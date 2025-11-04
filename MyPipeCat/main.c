#include <sys/wait.h>
#include "./io.h"

static size_t size_buf = 4096;

int main(int argc, char* argv[])
{
  ssize_t written = 0;
  char buf[4096] = {0};
  int fds[2];

  if (pipe(fds) != 0)
  {
    fprintf(stderr, "Mypcat: %s\n", strerror(errno));
    exit(-1);
  }

  pid_t pid = fork();
  if (pid == -1)
  {
    fprintf(stderr, "fork: %s\n", strerror(errno));
    exit(-1);
  }

  if (pid == 0)
  {
    close(fds[1]); // closes writing
    CopyFile(fds[0], 1, buf, size_buf);  
    close(fds[0]);
    exit(1);
  }

  close(fds[0]);
  
  if (argc > 1)
  {
    for(int i = 1; i < argc; i++)
    {
      int fd = Open(argv[i]);
      CopyFile(fd, fds[1], buf, size_buf);
      close(fd);
    }
  }

  else
  {
    CopyFile(0, fds[1], buf, size_buf); //stdin -> pipe
    close(fds[1]);
  }

  int status;
  wait(&status);

  return 0;
}