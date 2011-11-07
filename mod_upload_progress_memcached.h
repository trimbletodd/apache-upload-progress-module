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

static memcached_st *init_memcache(char *file);
static void set_memcache_conn_string(char *file, char *config_string);
static bool file_exists(const char *filename);

/* 
 * If the key isn't initialized, create the key, otherwise,
 * update the key with the new values of node
 */

/* static void init_memcache_and_update_key(char *id, char *val, char *file){ */
/*   memcached_return_t rc; */
/*   memcache_st *memc = init_memcache(file); */
/*   uint32_t flags=0; */
/*   char *ret_str; */
/*   const char *rc_str; */

/*   memc=init_memcache(file); */

/*   rc = memcached_set(memc, id, strlen(id), val, strlen(val), expiration, flags); */
 
/*   rc = memcached_exist(memc, id, (size_t) strlen(id)); */
/*   memcached_free(memc); */
/* } */


/*
 * Initializes the memcache connection
 *
 * Todo: if file doesn't exist/is not readable, throw error?
 */
static memcached_st *init_memcache(char *file){
  char conn_string[1024];
  set_memcache_conn_string(file, conn_string);
  memcached_st *memc= memcached(conn_string, strlen(conn_string));
  memcached_return_t rc= memcached_version(memc);
  if (rc != MEMCACHED_SUCCESS){
    printf("ERROR: Unable to initiate connection to memcached using connection string %s\n", conn_string);
  }
  return(memc);
}

/*
 * Return node data in JSON
 */
/* static char * node_to_JSON(upload_progress_node_s node){ */
/*   if (node == NULL) { */
/*     return "node undefined in node_to_JSON"; */
/*   }else{ */
    
/*   } */
/* } */

/*
 * Takes a ruby input file, which should define MEMCACHED_SERVERS,
 * and outputs a properly defined config string.
 *
 * Note the dependency on ruby being defined in $PATH.
 */
static void set_memcache_conn_string(char *file, char *config_string){
  if (!file_exists(file)){
    sprintf(config_string, "%s", "--SERVER=localhost:11211");
    return;
  }

  char command[1024];
  FILE *fp;
  int status;
  char path[1035];

  /* Build the command to parse the file */
  sprintf(command, "ruby -e \'require \"%s\"; puts MEMCACHED_SERVERS.map{|h| \"--SERVER=#{h}\"}.join(\" \")\' 2> /dev/null", file);

  /* Open the command for reading. */
  fp = popen(command, "r");
  if (fp == NULL) {
    printf("Failed to run command\n" );
    exit;
  }

  /* Read the output a line at a time - output it. */
  while (fgets(path, sizeof(path)-1, fp) != NULL) {
    sprintf(config_string, "%s", path);
  }
  /* close */
  pclose(fp);
}

static bool file_exists(const char *filename){    
  FILE *file;
  if (file = fopen(filename, "r")){
    fclose(file);        
    return true;    
  }
  return false;
}

