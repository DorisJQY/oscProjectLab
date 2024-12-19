#include <stdlib.h>
#include <pthread.h>
#include "config.h"
#include "sbuffer.h"
#include <semaphore.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

FILE* binFile;
FILE* dataFile;
FILE* storageFile;
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
  while(1)
  {
    if(dataFile == NULL) break;
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

    //write to sensor_data_out.csv
    if(dataFile != NULL)
    {
      fprintf(dataFile, "%u, %.6f, %ld\n", data.id, data.value, data.ts);
      fflush(dataFile);
    }

  }
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
      fprintf(storageFile, "%u, %.6f, %ld\n", data.id, data.value, data.ts);
      fflush(storageFile);
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

  dataFile = fopen("data_out.csv", "a");
  if(!dataFile)
  {
    perror("Failed to open data_out.csv");
    fclose(binFile);
    return -1;
  }

  storageFile = fopen("storage_out.csv", "a");
  if(!storageFile)
  {
    perror("Failed to open storage_out.csv");
    fclose(binFile);
    fclose(dataFile);
    return -1;
  }

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
  fclose(dataFile);
  dataFile = NULL;
  fclose(storageFile);
  storageFile = NULL;

  if(sbuffer_free(&sbuffer) == -1)
  {
    printf("Free buffer failed.\n");
    return -1;
  }

  printf("Program completed successfully.\n");
  return 0;
}
