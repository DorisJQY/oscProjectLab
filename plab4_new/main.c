#include <stdlib.h>
#include <pthread.h>
#include "config.h"
#include <semaphore.h>
#include "sbuffer.h"
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

FILE* binFile;
FILE* textFile;
sbuffer_t* sbuffer;
pthread_mutex_t textFileMut;

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

    usleep(10000);
  }
  return NULL;
}

void *reader_thread(void *arg)
{
  while(1)
  {
    if(textFile == NULL) break;
    //read from buffer
    sensor_data_t data;
    int result = sbuffer_remove(sbuffer, &data);
    if (result == SBUFFER_NO_DATA)
    {
      break;
    }
    else if (result == SBUFFER_FAILURE)
    {
      printf("Read from buffer failed.\n");
      break;
    }

    //write to sensor_data_out.csv
    pthread_mutex_lock(&textFileMut);
    if(textFile != NULL)
    {
      fprintf(textFile, "%u, %.6f, %ld\n", data.id, data.value, data.ts);
      fflush(textFile);
    }
    pthread_mutex_unlock(&textFileMut);

    usleep(25000);
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

  textFile = fopen("sensor_data_out.csv", "a");
  if(!textFile)
  {
    perror("Failed to open sensor_data_out.csv");
    fclose(binFile);
    return -1;
  }

  pthread_mutex_init(&textFileMut, NULL);

  if(sbuffer_init(&sbuffer) == -1)
  {
    printf("Create buffer failed.\n");
    return -1;
  }

  pthread_t writer, reader1, reader2;

  pthread_create(&writer, NULL, writer_thread, NULL);
  pthread_create(&reader1, NULL, reader_thread, NULL);
  pthread_create(&reader2, NULL, reader_thread, NULL);

  pthread_join(writer, NULL);
  pthread_join(reader1, NULL);
  pthread_join(reader2, NULL);

  fclose(binFile);
  binFile = NULL;
  fclose(textFile);
  textFile = NULL;

  pthread_mutex_destroy(&textFileMut);

  if(sbuffer_free(&sbuffer) == -1)
  {
    printf("Free buffer failed.\n");
    return -1;
  }

  printf("Program completed successfully.\n");
  return 0;
}
