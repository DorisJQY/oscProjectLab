#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include "config.h"
#include "datamgr.h"
#include "sensor_db.h"
#include "sbuffer.h"
#include "connmgr.h"

FILE* fp_data_csv;
FILE* fp_sensor_map;
sbuffer_t* sbuffer;

void *connection_thread(void *arg) {
  connmgr_args_t* args = (connmgr_args_t*) arg;
  int port = args->port;
  int max_conn = args->max_conn;

  int result = connmgr_run(port, max_conn);
  if (result == 0)
    printf("[connection_thread] connmgr_run finished successfully.\n");
  else
    printf("[connection_thread] connmgr_run returned error (%d).\n", result);

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

    if (sensor_in_map(data.id) == true)
      update_running_avg(&data);
    else
      printf("Received sensor data with invalid sensor node ID %d",data.id);
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
  close_db(fp_data_csv);
  return NULL;
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

  fp_data_csv = open_db("data.csv",false);
  if (!fp_data_csv) {
    perror("Failed to open data.csv");
    return -1;
  }
  
  fp_sensor_map = fopen("room_sensor.map", "r");
  if (!fp_sensor_map) {
    perror("Failed to open room_sensor.map");
    return -1;
  }

  if (sbuffer_init(&sbuffer) == -1) {
    printf("Create buffer failed.\n");
    return -1;
  }

  pthread_t connmgr, datamgr, sensor_db;

  pthread_create(&connmgr, NULL, connection_thread, &args);
  pthread_create(&datamgr, NULL, data_thread, NULL);
  pthread_create(&sensor_db, NULL, storage_thread, NULL);

  pthread_join(connmgr, NULL);
  pthread_join(datamgr, NULL);
  pthread_join(sensor_db, NULL);

  close_db(fp_data_csv);

  if(sbuffer_free(&sbuffer) == -1)
  {
    printf("Free buffer failed.\n");
    return -1;
  }

  printf("Program completed successfully.\n");
  return 0;
}
