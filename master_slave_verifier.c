/*
 * Replica Set Verifier: check if your replica set is running
 *
 * SAMPLE BUILD:
 * gcc --std=c99 -I/usr/local/include -L/usr/local/lib -o master_slave_verifier master_slave_verifier.c -lmongoc
 *
 * REQUIREMENTS:
 * LIBBSON 0.4.0
 * MONGO-C-DRIVER 0.8.1
 * 
 * CONFIG FILE:
 * master_slave_verifier.conf 
 *
 * EXECUTE:
 * sudo ./master_slave_verifier
 * 
 * OUTPUT FILES:
 * master_slave_verifier (bin)
 *
 * OUTPUT FILES (when master_slave_verifier is excuted):
 * master_slave_verifier.log (/var/log)
 * replica_set_verfier.pid
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include "mongo.h"

#define MAXLEN 80
#define CONFIG_FILE "/etc/master_slave_verifier.conf"

/*
* create the struct which encapsulates each server config in replica set
*/
struct ms_server
{
  char address[MAXLEN];
  char port[MAXLEN];
}
  ms_server;

/*
 * initialize one instance of server with default values
 */
void
master_server (struct ms_server * server){
  strncpy (server->address, "127.0.0.1", MAXLEN);
  strncpy (server->port, "27017", MAXLEN);
}


/*
 * trim: get rid of trailing and leading whitespace...
 *       ...including the annoying "\n" from fgets()
 */
char *trim(char *str){
  char *end;

  // Trim leading space
  while(isspace(*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}

/*---------- Config Parser Funcitons ----------*/
/*
 *  get replica set configuration
 */
void
replica_config (struct ms_server * server, FILE *logger)
{
  char *s, buff[256];
  FILE *fp = fopen (CONFIG_FILE, "r");
  if (fp == NULL)
  {
    fprintf(logger, "/etc/master_slave_verifier.conf: no such file...");
    return;
  }
  while ((s = fgets (buff, sizeof buff, fp)) != NULL)
  {
    if (buff[0] == '\n' || buff[0] == '#')
      continue;
    char name[MAXLEN], value[MAXLEN];
    s = strtok (buff, "=");
    if (s==NULL)
      continue;
    else
      strncpy (name, s, MAXLEN);
    s = strtok (NULL, "=");
    if (s==NULL)
      continue;
    else
      strncpy (value, s, MAXLEN);
    trim (value);
    if (strcmp(name, "address" )==0){
      strncpy (server->address, value, MAXLEN);
    }else if (strcmp(name, "port" )==0){
      strncpy (server->port, value, MAXLEN);
    }
  }
  fclose (fp);
}


int main(int argc, char* argv[]){
    /*------------------- Daemonize -------------------*/
    FILE *fp= NULL;
    FILE *pid_file= NULL;
    FILE *token= NULL;
    pid_t process_id = 0;
    pid_t sid = 0;
    // Create child process
    process_id = fork();
    // Indication of fork() failure
    if (process_id < 0){
        printf("fork failed!\n");
        // Return failure in exit status
        exit(1);
    }
    // PARENT PROCESS. Need to kill it.
    if (process_id > 0){
        printf("process_id of child process %d \n", process_id); 
        // Create a file with pid into itself
        pid_file = fopen ("/tmp/master_slave_verifier.pid", "w+");
        fprintf(pid_file, "%d", process_id);
        fflush(pid_file);
        // return success in exit status
        exit(0);
    }
    //unmask the file mode
    umask(0);
    //set new session
    sid = setsid();
    if(sid < 0){
        // Return failure
        exit(1);
    }
    // Change the current working directory to root.
    chdir("/");
    // Close stdin. stdout and stderr
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    // Open a log file in write mode.
    fp = fopen ("/var/log/master_slave_verifier.log", "w+");

    /*------------------- Config Parser -------------------*/    
    struct ms_server server;

    replica_config (&server, fp);


    /*------------------- Mongo Connection -------------------*/    
    // Mongo connection
    mongo conn[1];
    int status;
    int timer = 0;
    int TIMEOUT = 5;


    // Trying to establish the connection
    status = mongo_client(conn,server.address, atoi(server.port));
    fprintf(fp, "%d\n", status);
    while(status != MONGO_OK){
      switch ( conn->err ) {
        case MONGO_CONN_NO_SOCKET: printf("NO SOCKET\n"); break;  /**< Could not create a socket. */
        case MONGO_CONN_FAIL: printf("CONN FAIL\n"); break;       /**< An error occured while calling connect(). */
        case MONGO_CONN_ADDR_FAIL: printf("CON ADDR FAIL\n"); break;  /**< An error occured while calling getaddrinfo(). */
        case MONGO_CONN_NOT_MASTER: printf("CONN NOT MASTER\n"); break; /**< Warning: connected to a non-master node (read-only). */
      }
      fflush(fp);
      fprintf(fp,"Checking the connection... status( %d )\n", status);
      sleep(TIMEOUT);  
      status = mongo_client(conn,server.address, atoi(server.port)); 
    }  
    fflush(fp);
    fprintf(fp, "connection OK!.\n" );
    fprintf(fp, "Setting token.\n" );
    // Open a token file in write mode.
    token = fopen ("/tmp/token.msv", "w+");
    fprintf(token, "true");
    fflush(token);
    mongo_destroy( conn );
    fprintf(fp, "ending...\n" );
    fclose(fp);
    fclose(token);
    return (0);
}