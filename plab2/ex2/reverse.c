#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define SIZE 25
#define READ_END 0
#define WRITE_END 1

int main()
{
  int fd[2];
  char wmsg[SIZE];
  char rmsg[SIZE];
  pid_t pid;
  if(pipe(fd) == -1)
  {
    printf("Pipe failed\n");
    return 1;
  }

  pid = fork();
  if(pid < 0)
  {
    printf("Fork failed\n");
    return 1;
  }

  if(pid > 0)
  {
    close(fd[READ_END]);
    printf("Input string: ");
    fgets(wmsg,SIZE,stdin);
    write(fd[WRITE_END],wmsg,strlen(wmsg)+1);
    close(fd[WRITE_END]);
    wait(NULL);
  }
  else
  {
    close(fd[WRITE_END]);
    read(fd[READ_END],rmsg,SIZE);
    close(fd[READ_END]);
    int size = strlen(rmsg);
    for(int i = 0;i < size;i++)
    {
      if(islower(rmsg[i]))
      {
        rmsg[i] = toupper(rmsg[i]);
      }
      else if(isupper(rmsg[i]))
      {
        rmsg[i] = tolower(rmsg[i]);
      }
    }
    printf("After reverse: %s",rmsg);
  }
  return 0;
}

