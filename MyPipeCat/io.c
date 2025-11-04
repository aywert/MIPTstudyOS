#include "./io.h"


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

ssize_t Write(int fd, char buf[], ssize_t size)
{
  int n = write(fd, buf, size);

  if(n < 0)
  {
    fprintf(stderr, "MyCat: %s\n", strerror(errno));
    exit(-1);
  }

  return n;
}

int Open(char * filename)
{
  int fd = open(filename, O_RDONLY);
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
    written = 0;
    
    while(n != written)
    {
      ssize_t w = Write(fd_write, buf + written, n - written); //Write replace by write with checking
      written += w;
    }
  } while(n != 0);

  return;
}