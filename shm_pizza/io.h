#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/sem.h>

struct Argument {
  int N;
  int D;
  int K;
};

enum pizza_status
{
  pizza_ready,
  pizza_cooking,
  free_table,
};

int get_arg(int argc, char* argv[], struct Argument* st);
void RunOp_safe(int semid, struct sembuf *op, size_t nop);
void del_shm(int shmid);
void royal_print(int N, int* buffer);
pid_t fork_safe(void);