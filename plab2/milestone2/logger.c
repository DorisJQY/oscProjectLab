#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int sequence_number = 0;
FILE* fp = NULL;

int write_to_log_process(char *msg)
{
  time_t now;
  struct tm *local;
  char time_string[125];

  time(&now);
  local = localtime(&now);
  strftime(time_string, sizeof(time), "%a %b %d %H:%M:%S %Y", local);

  fprintf(fp, "%d - %s - %s\n", sequence_number, time_string, msg);
  fflush(fp);
  sequence_number++;
  return 0;
}

int create_log_process()
{
  fp = fopen("gateway.log","a");
  sequence_number = 0;
  return 0;
}

int end_log_process()
{
  fclose(fp);
  sequence_number = 0;
  return 0;
}
