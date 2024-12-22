#include <stdlib.h>
#include <pthread.h>
#include "config.h"
#include "datamgr.h"
#include "sensor_db.h"
#include "sbuffer.h"
#include "connmgr.h"
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

FILE* storageFile;
FILE *fp_sensor_map;
sbuffer_t* sbuffer;

void *connection_thread(void *arg)
{
    conn_thread_args_t* args = (conn_thread_args_t*) arg;
    int port = args->port;
    int max_conn = args->max_conn;

    // 在这里调用 connmgr_run
    int ret = connmgr_run(port, max_conn);
    if (ret == 0) {
    printf("[connection_thread] connmgr_run finished successfully.\n");
    } else {
    printf("[connection_thread] connmgr_run returned error (%d).\n", ret);
    }
    return NULL;
}


void *data_thread(void *arg)
{
  datamgr_parse_sensor_files(fp_sensor_map);
  while(1)
  {
    //read from buffer
    sensor_data_t data;
    int result = sbuffer_remove(sbuffer, &data, false);
    if (result == SBUFFER_NO_DATA)
    {
      break;
    }
    else if (result == SBUFFER_FAILURE)
    {
      printf("Read from buffer failed.\n");
      break;
    }
    else if (result == SBUFFER_WAIT)
    {
      continue;
    }

    if (sensor_in_map(data.id) == true)
    {
      update_running_avg(&data);
    }
    else
    {
      printf("Received sensor data with invalid sensor node ID %d",data.id);
    }
  }
  print_dplist_contents();
  datamgr_free();
  return NULL;
}

void *storage_thread(void *arg)
{
  while(1)
  {
    if(storageFile == NULL) break;
    //read from buffer
    sensor_data_t data;
    int result = sbuffer_remove(sbuffer, &data, true);
    if (result == SBUFFER_NO_DATA)
    {
      break;
    }
    else if (result == SBUFFER_FAILURE)
    {
      printf("Read from buffer failed.\n");
      break;
    }
    else if (result == SBUFFER_WAIT)
    {
      continue;
    }

    //write to sensor_data_out.csv
    if(storageFile != NULL)
    {
      insert_sensor(storageFile, &data);
    }
  }
  return NULL;
}

int main(int argc, char *argv[])
{
  if(argc < 3) {
    	printf("Please provide the right arguments: first the port, then the max nb of clients");
    	return -1;
    }

    int MAX_CONN = atoi(argv[2]);
    int PORT = atoi(argv[1]);
    conn_thread_args_t c_args;
    c_args.port = PORT;
    c_args.max_conn = MAX_CONN;

  storageFile = open_db("data.csv",false);
  if(!storageFile)
  {
    perror("Failed to open data.csv");
    return -1;
  }
  
  fp_sensor_map = fopen("room_sensor.map", "r");

  if(sbuffer_init(&sbuffer) == -1)
  {
    printf("Create buffer failed.\n");
    return -1;
  }

  pthread_t writer, reader1, reader2;

  pthread_create(&writer, NULL, connection_thread, &c_args);
  pthread_create(&reader1, NULL, data_thread, NULL);
  pthread_create(&reader2, NULL, storage_thread, NULL);

  pthread_join(writer, NULL);
  pthread_join(reader1, NULL);
  pthread_join(reader2, NULL);

  close_db(storageFile);

  if(sbuffer_free(&sbuffer) == -1)
  {
    printf("Free buffer failed.\n");
    return -1;
  }

  printf("Program completed successfully.\n");
  return 0;
}
