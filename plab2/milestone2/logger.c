#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

int sequence_number = 0;
FILE* fp = NULL;

int write_to_log_process(char *msg)
{
  time_t now;
  struct tm *local;
  char time_string[125];

  time(&now);
  local = localtime(&now);
  strftime(time_string, sizeof(time_string), "%a %b %d %H:%M:%S %Y", local);

  const char *current_string = msg;
  while(*current_string != '\0')
  {
    size_t length = strlen(current_string);
    fprintf(fp, "%d - %s - %.*s\n", sequence_number++, time_string, (int)length, current_string);
    fflush(fp);
    current_string += length + 1;
  }
  return 0;
}

int create_log_process()
{
  fp = fopen("gateway.log","a");
  if(fp == NULL)
  {
    printf("file open failed");
    return -1;
  }
  sequence_number = 0;
  return 0;
}

int end_log_process()
{
  fclose(fp);
  sequence_number = 0;
  return 0;
}
