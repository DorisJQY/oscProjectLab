#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "sensor_db.h"
#include "logger.h"
#include <sys/wait.h>

#define SIZE 25
#define READ_END 0
#define WRITE_END 1

pid_t pid;
int fd[2];
char wmsg[SIZE];
char rmsg[SIZE];

FILE * open_db(char * filename, bool append)
{
  FILE *fp = NULL;
  static bool logger_initialize = false;
  if(!logger_initialize)
  {
    if(pipe(fd) == -1)
    {
      printf("pipe failed");
      return NULL;
    }
    pid = fork();
    if(pid < 0)
    {
      printf("fork failed");
      return NULL;
    }
    if(pid == 0)
    {
      create_log_process();
    }
    logger_initialize = true;
  }

  if(pid > 0)
  {
    close(fd[READ_END]);
    if(append)
    {
      fp = fopen(filename,"a");
    }
    else
    {
      fp = fopen(filename,"w");
    }
    snprintf(wmsg, SIZE, "Data file opened.");
    write(fd[WRITE_END],wmsg,strlen(wmsg)+1);
  }
  else
  {
    close(fd[WRITE_END]);
    read(fd[READ_END],rmsg,SIZE);
    write_to_log_process(rmsg);
  }
  return fp;
}

int insert_sensor(FILE * f, sensor_id_t id, sensor_value_t value, sensor_ts_t ts)
{
  int i;
  if(pid > 0)
  {
    if(f == NULL)
    {
      return -1;
    }
    i = fprintf(f,"%u, %.6f, %ld\n",id,value,ts);
    snprintf(wmsg, SIZE, "Data inserted.");
    write(fd[WRITE_END],wmsg,strlen(wmsg)+1);
  }
  else
  {
    read(fd[READ_END],rmsg,SIZE);
    write_to_log_process(rmsg);
  }
  return i;
}

int close_db(FILE * f)
{
  int i;
  if(pid > 0)
  {
    if(f==NULL)
    {
      return -1;
    }
    i = fclose(f);
    snprintf(wmsg, SIZE, "Data file closed.");
    write(fd[WRITE_END],wmsg,strlen(wmsg)+1);
    close(fd[WRITE_END]);
  }
  else
  {
    read(fd[READ_END],rmsg,SIZE);
    close(fd[READ_END]);
    write_to_log_process(rmsg);
    end_log_process();
  }
  waitpid(pid, NULL, 0);
  return i;
}
