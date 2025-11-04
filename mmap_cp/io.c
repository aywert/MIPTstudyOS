#include "io.h"

int open_safe(const char* name, int flags, int mode) {
    int fd = open(name, flags, mode);
    if (fd < 0) {
        fprintf(stderr, "error in opening %s: %s", name, strerror(errno));
        exit(EXIT_FAILURE);
    }

    return fd;
}

void ftruncate_safe(int fd, off_t size) {
    if (ftruncate(fd, size) != 0) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }
}

void* mmap_safe(void* addr, off_t size, int prot, int flags, int fd, off_t offset) {
    void* result = mmap(addr, size, prot, flags, fd, offset);
    if (result == MAP_FAILED) {
        perror("error in mmap");
        exit(EXIT_FAILURE);
    }
    return result;
}

void munmap_safe(void* addr, off_t size) {
    if (munmap(addr, size) != 0) {
        perror("munmap error");
        exit(EXIT_FAILURE);
    }
}