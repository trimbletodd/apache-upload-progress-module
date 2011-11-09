 /*
 * General implementation of the libmemcached library
 * for use in the mod_upload_progress apache plugin with ruby
 */
#include <stdio.h>
#include <stdlib.h>

// The static connection to memcache
/* static const memcached_st *memc; */
// The expiration time of the keys in seconds
static const uint32_t expiration = 28800;
static memcached_st *memcache_inst;

static void memcache_node_to_JSON(upload_progress_node_t *node, char *str);
static memcached_st *memcache_init(char *file);
static void memcache_cleanup();
static void memcache_get_conn_string(char *file, char *config_string, size_t config_str_len);
static bool file_exists(const char *filename);
static void memcache_print_val(char *key, size_t length, char *namespace);
static void memcache_update_progress(char *key, upload_progress_node_t *node, char *namespace);

/*
 * This sets the directory config for using memcache to track uploads.
 * Variable: UploadProgressUseMemcache
 */
static const char *memcache_track_upload_progress_cmd(cmd_parms *cmd, void *dummy, int arg){
    ServerConfig *config = (ServerConfig*)ap_get_module_config(cmd->server->module_config, &upload_progress_module);
    config->memcache_enabled = arg;
    return NULL;
}

/*
 * This sets the file where MEMCACHE_SERVERS is defined.
 * 
 * Variable: UploadProgressMemcacheFile
 */
static const char* memcache_server_file_cmd(cmd_parms *cmd, void *dummy, char *arg) {
    ServerConfig *config = (ServerConfig*)ap_get_module_config(cmd->server->module_config, &upload_progress_module);
    config->memcache_server_file = arg;
    return NULL;
}

/*
 * This sets the memcache namespace to use
 * 
 * Variable: UploadProgressMemcacheNamespace
 */
static const char* memcache_namespace_cmd(cmd_parms *cmd, void *dummy, char *arg) {
    ServerConfig *config = (ServerConfig*)ap_get_module_config(cmd->server->module_config, &upload_progress_module);
    config->memcache_namespace = arg;
    return NULL;
}

/*
 * Initializes the memcache connection
 *
 * Todo: if file doesn't exist/is not readable, throw error
 */
static memcached_st *memcache_init(char *file){
  char conn_string[1024];
  memcache_get_conn_string(file, conn_string, sizeof(conn_string));
  memcache_inst = memcached(conn_string, strlen(conn_string));
  if(memcache_inst == NULL){
    printf("ERROR: Unable to initiate connection to memcached using connection string %s\n", conn_string);
    exit(1);
  }
  memcached_return_t rc= memcached_version(memcache_inst);
  if (rc != MEMCACHED_SUCCESS){
    printf("ERROR: Unable to initiate connection to memcached using connection string %s\n", conn_string);
  }
  return(memcache_inst);
}

/*
 * Primarily used for debugging
 */
static void memcache_print_val(char *key, size_t length, char *namespace){
  size_t return_value_length;
  char *ret_str;
  uint32_t flags=0;
  memcached_return_t rc;
  char key_w_ns[1024];
  sprintf(key_w_ns, "%s%s", namespace,key);

  ret_str = memcached_get(memcache_inst, key_w_ns, strlen(key_w_ns), &return_value_length, &flags, &rc);
  if (rc != MEMCACHED_SUCCESS){printf("ERROR: ");}
  printf("Get(%s) => %s (rc: %s)\n", key_w_ns, ret_str, memcached_strerror(memcache_inst, rc));

}

/*
 * Terminates the memcache connection
 */
static void memcache_cleanup(){
  memcached_free(memcache_inst);
}

/*
 * updates memcache key with the JSON from the node
 *
 * Todo: make this use apr_pcalloc instead of malloc
 */
static void memcache_update_progress(char *key, upload_progress_node_t *node, char *namespace){
  memcached_return_t rc;
  uint32_t flags=0;
  char *json_str = (char *) malloc(1024);
  char key_w_ns[1024];
  sprintf(key_w_ns, "%s%s", namespace,key);

  // Caution, connection may have timed out by the time this is called.
  memcache_node_to_JSON(node, json_str);
  /* printf("[%s] => %s\n", key_w_ns,json_str); */
  rc = memcached_set(memcache_inst, key_w_ns, strlen(key_w_ns), json_str, strlen(json_str), expiration, flags);
  if (rc != MEMCACHED_SUCCESS){
    printf("ERROR: key=%s %s", key_w_ns, memcached_strerror(memcache_inst, rc));
  }
}

/*
 * Return node data in JSON
 */
static void memcache_node_to_JSON(upload_progress_node_t *node, char *str){
  if (node == NULL) {
    sprintf(str, "node undefined in node_to_JSON");
  }else{
    sprintf(str, "{\"%s\": \"%i\",\"%s\": \"%i\",\"%s\": \"%i\",\"%s\": \"%i\",\"%s\": \"%i\"}",
            "state", node->done,
            "size", node->length,
            "received", node->received,
            "speed", node->speed,
            "started_at", node->started_at);
  }
}

/*
 * Takes a ruby input file, which should define MEMCACHED_SERVERS,
 * and outputs a properly defined config string.
 *
 * Note the dependency on ruby being defined in $PATH.
 */
static void memcache_get_conn_string(char *file, char *config_string, size_t config_str_len){
  if (!file_exists(file)){
    sprintf(config_string, "%s", "--SERVER=localhost:11211");
    return;
  }

  char command[1024];
  FILE *fp;
  char path[100];
  /* Build the command to parse the file */
  sprintf(command, "ruby -e \'require \"%s\"; puts MEMCACHED_SERVERS.map{|h| \"--SERVER=#{h}\"}.join(\" \")\' 2> /dev/null", file);

  /* Open the command for reading. */
  fp = popen(command, "r");
  if (fp == NULL) {
    printf("Unable to retreive MEMCACHED_SERVERS variable from file %s (CMD: %s)", file, command);
    exit(1);
  }

  fgets(config_string, config_str_len-1, fp);
  int i=0;
  while(i<config_str_len){
    if (config_string[i] == '\n') {
      config_string[i] = '\0';
      break;
    }
    i++;
  }      

  /* close */
  if (pclose(fp) != 0){
    printf("Unable to retreive MEMCACHED_SERVERS variable from file %s (CMD: %s)", file, command);
    exit(1);
  }
}

static bool file_exists(const char *filename){    
  FILE *file;
  if (file = fopen(filename, "r")){
    fclose(file);        
    return true;    
  }
  return false;
}

