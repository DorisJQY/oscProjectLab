#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>
#include "config.h"
#include "datamgr.h"
#include "sensor_db.h"
#include "sbuffer.h"
#include "connmgr.h"

#define SIZE 250
#define READ_END 0
#define WRITE_END 1

pid_t pid;
int fd[2];
char rmsg[SIZE];
FILE* fp_data_csv;
FILE* fp_sensor_map;
FILE* fp_gateway_log;
sbuffer_t* sbuffer;
int sequence_number;
pthread_mutex_t logMut;


void *connection_thread(void *arg) {
  connmgr_args_t* args = (connmgr_args_t*) arg;
  int port = args->port;
  int max_conn = args->max_conn;

  int result = connmgr_run(port, max_conn);
  if (result == 0)
    printf("Connection manager finished successfully.\n");
  else
    printf("Connection manager returned error (%d).\n", result);

  return NULL;
}


void *data_thread(void *arg) {
  datamgr_parse_sensor_files(fp_sensor_map);
  fclose(fp_sensor_map);
  fp_sensor_map = NULL;
  while(1) {
    //read from buffer
    sensor_data_t data;
    int result = sbuffer_remove(sbuffer, &data, false);
    if (result == SBUFFER_NO_DATA)
      break;
    else if (result == SBUFFER_FAILURE){
      printf("Read from buffer failed.\n");
      break;
    }
    else if (result == SBUFFER_WAIT)
      continue;

    update_running_avg(&data);
  }
  print_datamgr_contents();
  datamgr_free();
  return NULL;
}

void *storage_thread(void *arg) {
  while(1) {
    //read from buffer
    sensor_data_t data;
    int result = sbuffer_remove(sbuffer, &data, true);
    if (result == SBUFFER_NO_DATA)
      break;
    else if (result == SBUFFER_FAILURE) {
      printf("Read from buffer failed.\n");
      break;
    }
    else if (result == SBUFFER_WAIT)
      continue;

    //write to data.csv
    if(fp_data_csv != NULL)
      insert_sensor(fp_data_csv, &data);
  }
  if (close_db(fp_data_csv) == 0)
    write_to_pipe("The data.csv file has been closed.");
  else
    fprintf(stderr, "Failed to close data.csv");
  return NULL;
}

int write_to_log_file(char *msg) {

  time_t now;
  struct tm *local;
  char time_string[125];

  time(&now);
  local = localtime(&now);
  strftime(time_string, sizeof(time_string), "%a %b %d %H:%M:%S %Y", local);

  const char *current_string = msg;
  while (*current_string != '\0') {
    if(strcmp(current_string, "End.") == 0) return -1;
    size_t length = strlen(current_string);
    fprintf(fp_gateway_log, "%d - %s - %.*s\n", sequence_number++, time_string, (int)length, current_string);
    fflush(fp_gateway_log);
    current_string += length + 1;
  }
  return 0;
}

int write_to_pipe(const char *msg) {
  pthread_mutex_lock(&logMut);
  ssize_t num = write(fd[WRITE_END], msg, strlen(msg) + 1);
  pthread_mutex_unlock(&logMut);

  if (num == -1) {
    fprintf(stderr, "Failed to write to pipe");
    return -1;
  }
  return 0;
}

int main(int argc, char *argv[]) {

  if (argc < 3) {
    printf("Please provide the right arguments: first the port, then the max number of clients");
    return -1;
  }

  int port = atoi(argv[1]);
  int max_conn = atoi(argv[2]);
  connmgr_args_t args;
  args.port = port;
  args.max_conn = max_conn;
  
  if (pipe(fd) == -1) {
    printf("pipe failed");
    return -1;
  }
  pid = fork();
  if (pid < 0) {
    printf("fork failed");
    return -1;
  }
  
  // log process
  if (pid == 0) {
    sequence_number = 0;
    close(fd[WRITE_END]);
    fp_gateway_log = fopen("gateway.log", "w");
    if (!fp_gateway_log) {
      fprintf(stderr, "Failed to open gateway.log");
      exit(-1);
    }
    
    while(1) {
      ssize_t num = read(fd[READ_END], rmsg, SIZE);
      if (num > 0) {
        rmsg[num] = '\0';
        int result = write_to_log_file(rmsg);
        if (result == -1) break;
      }
      else
        break;
    }
    close(fd[READ_END]);
    fclose(fp_gateway_log);
    fp_gateway_log = NULL;
    sequence_number = 0;
    exit(0);
  }
  // main process
  else if (pid > 0) {
    close(fd[READ_END]);
    fp_data_csv = open_db("data.csv",false);
    if (!fp_data_csv) {
      fprintf(stderr, "Failed to open data.csv");
      return -1;
    }
    char msg[SIZE];
    snprintf(msg, SIZE, "A new data.csv file has been created.");
    write_to_pipe(msg);
  
    fp_sensor_map = fopen("room_sensor.map", "r");
    if (!fp_sensor_map) {
      fprintf(stderr, "Failed to open room_sensor.map");
      return -1;
    }

    if (sbuffer_init(&sbuffer) == -1) {
      printf("Create buffer failed.\n");
      return -1;
    }
    
    if (pthread_mutex_init(&logMut, NULL) != 0) return -1;

    pthread_t connmgr, datamgr, sensor_db;

    pthread_create(&connmgr, NULL, connection_thread, &args);
    pthread_create(&datamgr, NULL, data_thread, NULL);
    pthread_create(&sensor_db, NULL, storage_thread, NULL);

    pthread_join(connmgr, NULL);
    pthread_join(datamgr, NULL);
    pthread_join(sensor_db, NULL);
    
    write_to_pipe("End.");
    close(fd[WRITE_END]);
    waitpid(pid, NULL, 0);
    
    if (pthread_mutex_destroy(&logMut) != 0) return -1;

    if(sbuffer_free(&sbuffer) == -1) {
      printf("Free buffer failed.\n");
      return -1;
    }

    printf("Program completed successfully.\n");
    return 0;
  }
}
