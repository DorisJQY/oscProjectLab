#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "sensor_db.h"
#include "lib/dplist.h"
#include "config.h"

FILE * open_db(char * filename, bool append) {
  FILE *fp = NULL;
  if (append)
    fp = fopen(filename,"a");
  else
    fp = fopen(filename,"w");
  return fp;
}

int insert_sensor(FILE * f, sensor_data_t *sensor_data) {
  int i;
  if (f == NULL) {
    printf("file open failed");
    return -1;
  }
  i = fprintf(f,"%hu, %.6f, %ld\n",sensor_data->id,sensor_data->value,sensor_data->ts);
  fflush(f);
  char msg[SIZE];
  snprintf(msg, SIZE, "Data insertion from sensor %hu succeeded.", sensor_data->id);
  write_to_pipe(msg);
  return i;
}

int close_db(FILE * f) {
  int i;
  if (f==NULL) {
    printf("file open failed");
    return -1;
  }
  i = fclose(f);
  f = NULL;
  return i;
}
