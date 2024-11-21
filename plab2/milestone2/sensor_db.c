#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "sensor_db.h"
#include "logger.h"
#include <sys/wait.h>

#define SIZE 250
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

    if (pid == 0) { // 子进程逻辑
            create_log_process();
            close(fd[WRITE_END]); // 子进程关闭写端

            while (1) {
                ssize_t num = read(fd[READ_END], rmsg, SIZE);
                if (num > 0) {
                    rmsg[num] = '\0'; // 确保字符串以 '\0' 结尾
                    printf("Received message: %s\n", rmsg);
                    write_to_log_process(rmsg);

                    if (strcmp(rmsg, "Data file closed.") == 0) {
                        printf("Debug: Received termination message. Exiting.\n");
                        break;
                    }
                } else if (num == 0) { // 管道关闭
                    printf("Debug: Pipe closed. Exiting.\n");
                    break;
                } else { // 读取错误
                    perror("Error reading from pipe");
                    break;
                }
            }

            close(fd[READ_END]); // 关闭读端
            end_log_process();
            exit(0); // 子进程正常退出
        }

        logger_initialize = true; // 确保初始化逻辑只执行一次
    }

    //if(pid == 0)
    //{
      //create_log_process();
      //close(fd[WRITE_END]);
      //bool end = false;
      //while(1)
      //{
        //ssize_t num = read(fd[READ_END],rmsg,SIZE);
        //if(num <= 0)
        //{
          //if(end)
          //{
            //close(fd[READ_END]);
            //end_log_process();
            //break;
          //}
        //}
        //rmsg[num] = '\0';
        //printf("Received message: %s\n", rmsg);
        //write_to_log_process(rmsg);
        //if(strcmp(rmsg, "Data file closed.") == 0)
        //{
          //end = true;
        //}
        //memset(rmsg, 0, SIZE);
      //}
    //}
    //logger_initialize = true;
  //}
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
    printf("Sent message: %s\n", wmsg);
    write(fd[WRITE_END],wmsg,strlen(wmsg)+1);
    memset(wmsg, 0, sizeof(wmsg));
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
      printf("file open failed");
      return -1;
    }
    i = fprintf(f,"%u, %.6f, %ld\n",id,value,ts);
    snprintf(wmsg, SIZE, "Data inserted.");
    printf("Sent message: %s\n", wmsg);
    write(fd[WRITE_END],wmsg,strlen(wmsg)+1);
    memset(wmsg, 0, sizeof(wmsg));
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
      printf("file open failed");
      return -1;
    }
    i = fclose(f);
    snprintf(wmsg, SIZE, "Data file closed.");
    printf("Sent message: %s\n", wmsg);
    write(fd[WRITE_END],wmsg,strlen(wmsg)+1);
    memset(wmsg, 0, sizeof(wmsg));
    close(fd[WRITE_END]);
  }
  waitpid(pid, NULL, 0);
  return i;
}
