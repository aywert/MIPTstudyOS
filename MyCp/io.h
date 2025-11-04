#ifndef IO_FUNC
#define IO_FUNC

#include <unistd.h> //for write/read
#include <stdio.h>  // for fprintf
#include <errno.h>  // for errno var
#include <fcntl.h>  // for open
#include <string.h> // for strerr
#include <stdlib.h> // for exit

ssize_t Write(int fd, char buf[], ssize_t size);
ssize_t Read(int fd, char buf[], ssize_t size);
int Open(char * filename, int o_flags, ...);
void CopyFile(int fd_read, int fd_write, char* buf, size_t size_buf);

#endif