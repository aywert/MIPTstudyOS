#include "io.h"


int main(int argc, char* argv[]) {
    if (argc != 3) {
      fprintf(stderr, "Invalid input");
      return EXIT_FAILURE;
    }

    int fd_1 = open_safe(argv[1], O_RDONLY, 0644);
    int fd_2 = open_safe(argv[2], O_RDWR | O_CREAT | O_TRUNC, 0644);

    struct stat file_st;
    if (fstat(fd_1, &file_st) == -1) {
        perror("stat error");
        exit(EXIT_FAILURE);
    }
    
    off_t shared_mem_size = (off_t)file_st.st_size;

    ftruncate_safe(fd_2, shared_mem_size); // truncate file given by fd to size 

    void* mapped_1 = mmap_safe(NULL, shared_mem_size, PROT_READ, MAP_PRIVATE, fd_1, 0);
    void* mapped_2   = mmap_safe(NULL, shared_mem_size, PROT_WRITE, MAP_SHARED, fd_2, 0);

    memcpy(mapped_2, mapped_1, shared_mem_size);

    munmap_safe(mapped_1, shared_mem_size);
    munmap_safe(mapped_2, shared_mem_size);

    close(fd_1);
    close(fd_2);

    return EXIT_SUCCESS;

}