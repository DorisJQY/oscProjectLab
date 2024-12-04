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
sem_t sem;
pthread_mutex_t bufferMut;
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
      fclose(binFile);
      binFile = NULL;
            
      sensor_data_t end_marker = {0, 0.0, 0}; 
      pthread_mutex_lock(&bufferMut);
      sbuffer_insert(sbuffer, &end_marker);
      sem_post(&sem); 
      pthread_mutex_unlock(&bufferMut);

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
    pthread_mutex_lock(&bufferMut);
    if(sbuffer_insert(sbuffer, &data) == -1)
    {
      printf("Write to buffer failed.\n");
      pthread_mutex_unlock(&bufferMut);
      break;
    }
    sem_post(&sem); 
    pthread_mutex_unlock(&bufferMut);

    usleep(10000); 
    }
    return NULL;
}

void *reader_thread(void *arg)
{
  while(1)
  {
    if(textFile == NULL)
    {
      printf("End marker detected by reader. Exiting.\n");
      break;
    }
        
    //read from buffer
    sem_wait(&sem);
    pthread_mutex_lock(&bufferMut);
    sensor_data_t data;
    if(sbuffer_remove(sbuffer, &data) == -1)
    {
      pthread_mutex_unlock(&bufferMut);
      continue;//continue waiting for data
    }
    pthread_mutex_unlock(&bufferMut);

    if(data.id == 0)
    {
      fclose(textFile);
      textFile = NULL;
      printf("End marker detected by reader. Exiting.\n");
      break;
    }

    //write to sensor_data_out.csv
    pthread_mutex_lock(&textFileMut);
    if(textFile != NULL)
    {
      fprintf(textFile, "%u, %.6f, %ld\n", data.id, data.value, data.ts);
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
  
  sem_init(&sem, 0, 0);
  pthread_mutex_init(&bufferMut, NULL);
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

  sem_destroy(&sem);
  pthread_mutex_destroy(&bufferMut);
  pthread_mutex_destroy(&textFileMut);

  if(sbuffer_free(&sbuffer) == -1)
  {
    printf("Free buffer failed.\n");
    return -1;
  }

  printf("Program completed successfully.\n");
  return 0;
}

