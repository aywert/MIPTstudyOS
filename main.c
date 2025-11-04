#include "io.h"
#include <sys/wait.h>
#include <stdbool.h>

void create_workers(int semid);
void vint_worker(int semid, int num);
void gaika_worker(int semid, int num);
void replace_detail(int semid);

int details = 5;

// sem_array = | Able_to_enter | Vint = 2| Gaika = 1| Ready
int main(void)
{
  int semid = create_semaphore("main.c", IPC_CREAT|0644);
  
  create_workers(semid);

  for (size_t i = 0; i < 3; i++) wait(NULL);

  printf("all process are ended\n");
  delete_semaphore(semid);
  return 0;
}

void create_workers(int semid) {
  pid_t pid_1 = fork_safe();
  if (pid_1 == 0) {
    if (fork_safe()) vint_worker(semid, 1);
    else vint_worker(semid, 2);
  }

  else gaika_worker(semid, 1);
}

void vint_worker(int semid, int num) {
  struct sembuf vint_P[] = {{0, -1, 0}};
  struct sembuf vint_V[] = {{0, 1, 0}};
  struct sembuf screw_Vint[] = {{1, -1, 0}};
  bool need_to_work  = false;
  bool will_be_ready = false;

  int done = 0;

  while(done != details) {
    //sleep(1); // rest from hard work
    RunOp_safe(semid, vint_P, 1); //P()
    //printf("Vint worker %d: entered\n", num);

    int vint   = GetVal_safe(semid, 1);
    int gaika  = GetVal_safe(semid, 2);
    int status = GetVal_safe(semid, 3);
    //printf("| %d | %d | %d |\n", vint, gaika, status);

    if (vint != 0) {
      printf("Vint  worker %d: screwing %d vint\n", num, 3-vint);
      RunOp_safe(semid, screw_Vint, 1); // decreases number of vint yet to screw
      need_to_work = true;
      done++;
      if (vint-1 == 0 && gaika == 0) 
        will_be_ready = true;
    }
    else if (status) {
      printf("Vint  worker %d: replaced constructed detail\n", num);
      printf("-------Done: %d-------\n", done);
      replace_detail(semid); 
      RunOp_safe(semid, vint_V, 1); //V()
      continue;
    }

    RunOp_safe(semid, vint_V, 1); //V()

    if (need_to_work) {
      need_to_work = false;
      sleep(1); //simulating work

      if (will_be_ready) {
        will_be_ready = false;
        SetVal_safe(semid, 3, 1);
      }
    }
  }

  exit(EXIT_SUCCESS); 
}

void gaika_worker(int semid, int num) {
  struct sembuf gaika_P[] = {{0, -1, 0}};
  struct sembuf gaika_V[] = {{0, 1, 0}};
  struct sembuf screw_Gaika[] = {{2, -1, 0}};
  bool need_to_work  = false;
  bool will_be_ready = false;

  
  int done = 0;

  while(done != details) {
    //sleep(1); //rest from hard work
    RunOp_safe(semid, gaika_P, 1); //P()
    //printf("Gaika worker %d: entered\n", num);
    int vint   = GetVal_safe(semid, 1);
    int gaika  = GetVal_safe(semid, 2);
    int status = GetVal_safe(semid, 3);
    //printf("| %d | %d | %d |\n", vint, gaika, status);
    
    if (gaika != 0) {
      printf("Gaika worker %d: screwing %d gaika\n", num, 2-gaika);
      RunOp_safe(semid, screw_Gaika, 1); // decreases number of vint yet to screw
      need_to_work = true;
      done++;
      if (vint == 0 && gaika-1 == 0) 
        will_be_ready = true;
    }
    else if (status) {
      printf("Gaika worker %d: replaced constructed detail\n", num);
      printf("-------Done: %d-------\n", done);
      replace_detail(semid); 
      RunOp_safe(semid, gaika_V, 1); //V()
      continue;
    }

    RunOp_safe(semid, gaika_V, 1); //V()

    if (need_to_work) {
      need_to_work = false;
      sleep(1); //simulating work

      if (will_be_ready) {
        will_be_ready = false;
        SetVal_safe(semid, 3, 1);
      }
    }
  }
}

void replace_detail(int semid) {
  SetVal_safe(semid, 1, 2); //vint  = 2
  SetVal_safe(semid, 2, 1); //gaika = 1
  SetVal_safe(semid, 3, 0);  //Ready = 0
}