#include "io.h"


pid_t fork_safe(void) {
  pid_t p = fork();
  if (p < 0) {
    perror("fork");
    exit(EXIT_FAILURE);
  }

  return p;
}

int GetVal_safe(int semid, int num_in_sem) {
  int return_value = semctl(semid, num_in_sem, GETVAL);
  if (semctl(semid, num_in_sem, GETVAL) < 0) {
    perror("semop GetVal error\n");
    exit(EXIT_FAILURE);
  }

  return return_value;
}

void SetVal_safe(int semid, int num_in_sem, int value) {
  if (semctl(semid, num_in_sem, SETVAL, value) < 0) {
      perror("semop SetVal error\n");
      exit(EXIT_FAILURE);
  }
}

void RunOp_safe(int semid, struct sembuf *op, size_t nop) {
  if (semop(semid, op, nop) < 0) {
    perror("semop RunOp error");
    exit(EXIT_FAILURE);
  }
}

int create_semaphore(const char* name, int flags) {
  // sem_array = | Able_to_enter | Vint | Gaika | Ready | 
  int semid = semget(ftok(name, 1), 4, flags);
  if (semid == -1) {
    perror("unable to create semphore\n");
    exit(EXIT_FAILURE);
  }

  semctl(semid, 0, SETVAL, 1);
  semctl(semid, 1, SETVAL, 2);
  semctl(semid, 2, SETVAL, 1);
  semctl(semid, 3, SETVAL, 0);
  
  printf("Semaphore is created\n");

  return semid;
}

void delete_semaphore(int semid) {
  if (semctl(semid, IPC_RMID, 0) < 0) {
    perror("unable to delete semaphore\n");
    exit(EXIT_FAILURE);
  }

  printf("Semaphore is deleted\n");
}