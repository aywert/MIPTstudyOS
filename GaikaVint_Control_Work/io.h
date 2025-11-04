#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/sem.h>

int create_semaphore(const char* name, int flags);
void delete_semaphore(int semid);

void SetVal_safe(int semid, int num_in_sem, int value);
int GetVal_safe(int semid, int num_in_sem);
void RunOp_safe(int semid, struct sembuf *op, size_t nop);
pid_t fork_safe(void);