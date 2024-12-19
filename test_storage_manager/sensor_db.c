#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "sensor_db.h"
#include "logger.h"
#include <sys/wait.h>

FILE * open_db(char * filename, bool append)
{
  FILE *fp = NULL;
  if (append)
  {
    fp = fopen(filename,"a");
  }
  else
  {
    fp = fopen(filename,"w");
  }
  return fp;
}

int insert_sensor(FILE * f, sensor_data_t *sensor_data)
{
  int i;
  if(f == NULL)
  {
    printf("file open failed");
    return -1;
  }
  i = fprintf(f,"%u, %.6f, %ld\n",sensor_data->id,sensor_data->value,sensor_data->ts);
  return i;
}

int close_db(FILE * f)
{
  int i;
  if(f==NULL)
    {
      printf("file open failed");
      return -1;
    }
  i = fclose(f);
  return i;
}
