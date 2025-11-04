#ifndef IO_FUNC
#define IO_FUNC

#include <unistd.h> //for write/read
#include <stdio.h>  // for fprintf
#include <errno.h>  // for errno var
#include <fcntl.h>  // for open
#include <string.h> // for strerr
#include <stdlib.h> // for exit
#include <stdbool.h>

ssize_t MyWrite(int fd, char buf[], ssize_t size);
ssize_t MyRead(int fd, char buf[], ssize_t size);
int MyOpen(char * filename, int flags);
void CopyFile(int fd_read, int fd_write);
int CountWords(char* buf, size_t size_buf);

#endif