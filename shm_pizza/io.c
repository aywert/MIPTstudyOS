#include "io.h"

int get_arg(int argc, char* argv[], struct Argument* st) {
  if (argc != 4) {
    fprintf(stderr, "Incorrect input data\n");
    return 0;
  }

  st->N = atoi(argv[1]);
  st->D = atoi(argv[2]);
  st->K = atoi(argv[3]);

  return 0;
}

void del_shm(int shmid)
{
  if (shmctl(shmid, IPC_RMID, NULL) < 0) {
    perror("shmctl IPC_RMID");
    exit(EXIT_FAILURE);
  }
}

pid_t fork_safe(void) {
  pid_t p = fork();
  if (p < 0) {
    perror("fork");
    exit(EXIT_FAILURE);
  }

  return p;
}

void RunOp_safe(int semid, struct sembuf *op, size_t nop) {
  if (semop(semid, op, nop) < 0) {
    perror("semop RunOp error");
    exit(EXIT_FAILURE);
  }
}

void royal_print(int N, int* buffer) {
  for (int i = 0; i < N; i++) {
    printf("|");
    switch(buffer[i])
    {
      case pizza_ready:   printf("#");  break;
      case pizza_cooking: printf("...");break;
      case free_table:    printf("free");break;
      default: printf("|");break;
    }
  }

  printf("|\n");
}