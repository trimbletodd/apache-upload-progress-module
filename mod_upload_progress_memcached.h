/*
 * General implementation of the libmemcached library
 * for use in the mod_upload_progress apache plugin with ruby
 */
#include <stdio.h>
#include <stdlib.h>


/*
 * Takes the input file, which should define MEMCACHED_SERVERS,
 * and outputs a properly defined config string
 */
void set_memcache_config_string(char *file, char *config_string){
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
  sprintf(config_string, "%s", "--SERVER=localhost:11211");
  /* close */
  pclose(fp);
}

