#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

int open_safe(const char* name, int flags, int mode);
void ftruncate_safe(int fd, off_t size);
void* mmap_safe(void* addr, off_t size, int prot, int flags, int fd, off_t offset);
void munmap_safe(void* addr, off_t size);