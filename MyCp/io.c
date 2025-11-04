#include "./io.h"

ssize_t Write(int fd, char buf[], ssize_t size)
{
  size_t n = write(fd, buf, size);

  if(n < 0)
  {
    fprintf(stderr, "MyCat: %s\n", strerror(errno));
    exit(-1);
  }

  return n;
}

ssize_t Read(int fd, char buf[], ssize_t size)
{
  int n = read(fd, buf, size);
  if(n < 0)
  {
    fprintf(stderr, "MyCat: %s\n", strerror(errno));
    exit(-1);
  }

  return n;
}

int Open(char * filename, int o_flags, ...)
{
  int fd = open(filename, o_flags);
  if (fd < 0)
  {
    fprintf(stderr, "MyCat: %s: %s\n", filename, strerror(errno));
    exit(-1);
  }

  return fd;
}

void CopyFile(int fd_read, int fd_write, char* buf, size_t size_buf)
{
    int n = 0;
    ssize_t written = 0;
  do
  {
    n = Read(fd_read, buf, size_buf);
    
    while(n != written)
    {
      ssize_t w = Write(fd_write, buf + written, n - written);
      written += w;
    }

    written = 0;
  } while(n != 0);

  return;
}