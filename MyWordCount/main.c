#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include "./io.h"
#include <string.h>

void get_prop(int fd_read, char* buf, size_t size_buf);

int main(int argc, char* argv[]) 
{
    struct timeval t1 = {};
    struct timeval t2 = {};
    void* timez = NULL;

  char buf[4096] = {0};
  int fds[2];

  if (pipe(fds) != 0)
  {
    fprintf(stderr, "MyWordCount: %s\n", strerror(errno));
    exit(-1);
  }

  gettimeofday(&t1, timez);
  pid_t pid = fork();
  if (pid == 0) {
    close(fds[0]); //close read end
    int new_fd = dup2(fds[1], 1); //sets write end to 1 fd
    printf("efverferf\n");
    
    execvp(argv[1], &argv[1]);
    close(new_fd);
    close(fds[1]);
    
    fprintf(stderr, "MyWordCount: %s\n", strerror(errno));
    return 0;
  }

  close(fds[1]); //close write end

  get_prop(fds[0], buf, 4096);

  int status = 0;
  wait(&status);

  gettimeofday(&t2, timez);

  double usec = (double)t2.tv_usec - (double)t1.tv_usec;
  double sec  = (double)t2.tv_sec - (double)t1.tv_sec;
  double time = 1e6 * sec + usec;  

  printf("Delta  = %lg (usec)\n", time);
  return 0;
}

void get_prop(int fd_read, char* buf, size_t size_buf)
{
  size_t bytes = 0;
  size_t words = 0;
  size_t lines = 0;
  bool is_word_new = false;
  bool is_word_old = false;
  int n = 0;

  do
  {
    n = Read(fd_read, buf, size_buf);
    bytes += n;

    for(size_t i = 0; i < n; i++)
    {
      switch(buf[i])
      {
        case '\n': lines++; break;
        case ' ':  is_word_new = false; break;
        default:   is_word_new = true; break;
      }
      
      if (!is_word_old && is_word_new)
        words++;
      is_word_old = is_word_new;

    }
    
  } while(n != 0);

  printf("bytes = %zu\n", bytes);
  printf("words = %zu\n", words); 
  printf("lines = %zu\n", lines);
}


