#include <stdio.h>
#include "sensor_db.h"
#include <stdbool.h>

FILE * open_db(char * filename, bool append)
{
  FILE *fp;
  if(append)
  {
    fp = fopen(filename,"a");
  }
  else
  {
    fp = fopen(filename,"w");
  }
  if(fp == NULL)
  {
    perror("Failed to open file");
    return NULL;
  }
  return fp;
}

int insert_sensor(FILE * f, sensor_id_t id, sensor_value_t value, sensor_ts_t ts)
{
  if(f==NULL)
  {
    return -1;
  }
  int i = fprintf(f,"%u, %.6f, %ld\n",id,value,ts);
  return i;
}

int close_db(FILE * f)
{
  if(f==NULL)
  {
    return -1;
  }
  int i = fclose(f);
  return i;
}


