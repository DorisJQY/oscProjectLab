#include <stdlib.h>
#include <pthread.h>
#include "config.h"
#include "datamgr.h"
#include "sensor_db.h"
#include "sbuffer.h"
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

FILE* binFile;
//FILE* dataFile;
FILE* storageFile;
FILE *fp_sensor_map;
sbuffer_t* sbuffer;

void *writer_thread(void *arg)
{
  while(1)
  {
    if(binFile == NULL) break;

    //read from sensor_data single value by single value
    uint16_t sensor_id;
    double temperature;
    time_t timestamp;

    if(fread(&sensor_id, sizeof(uint16_t), 1, binFile) != 1)
    {
      if(feof(binFile))
      {
        sensor_id = 0;
      }
      else
      {
        printf("Error reading sensor_id.\n");
        break;
      }
    }

    if(sensor_id == 0)
    {
      sensor_data_t end_marker = {0, 0.0, 0};
      sbuffer_insert(sbuffer, &end_marker);

      printf("End marker created by writer.\n");
      break;
    }

    if(fread(&temperature, sizeof(double), 1, binFile) != 1)
    {
      printf("Error reading temperature.\n");
      break;
    }

    if(fread(&timestamp, sizeof(time_t), 1, binFile) != 1)
    {
      printf("Error reading timestamp.\n");
      break;
    }

    sensor_data_t data;
    data.id = sensor_id;
    data.value = temperature;
    data.ts = timestamp;

    //write to buffer
    if(sbuffer_insert(sbuffer, &data) == SBUFFER_FAILURE)
    {
      printf("Write to buffer failed.\n");
      break;
    }

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

int main()
{
  binFile = fopen("sensor_data", "rb");
  if(!binFile)
  {
    perror("Failed to open sensor_data");
    return -1;
  }

  //dataFile = fopen("data_out.csv", "a");
  //if(!dataFile)
  //{
    //perror("Failed to open data_out.csv");
    //fclose(binFile);
    //return -1;
  //}

  storageFile = open_db("data.csv",false);
  if(!storageFile)
  {
    perror("Failed to open data.csv");
    fclose(binFile);
    //fclose(dataFile);
    return -1;
  }
  
  fp_sensor_map = fopen("room_sensor.map", "r");

  if(sbuffer_init(&sbuffer) == -1)
  {
    printf("Create buffer failed.\n");
    return -1;
  }

  pthread_t writer, reader1, reader2;

  pthread_create(&writer, NULL, writer_thread, NULL);
  pthread_create(&reader1, NULL, data_thread, NULL);
  pthread_create(&reader2, NULL, storage_thread, NULL);

  pthread_join(writer, NULL);
  pthread_join(reader1, NULL);
  pthread_join(reader2, NULL);

  fclose(binFile);
  binFile = NULL;
  //fclose(dataFile);
  //dataFile = NULL;
  close_db(storageFile);

  if(sbuffer_free(&sbuffer) == -1)
  {
    printf("Free buffer failed.\n");
    return -1;
  }

  printf("Program completed successfully.\n");
  return 0;
}
