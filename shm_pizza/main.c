#include "io.h"
#include <sys/shm.h>
#include <wait.h>
#include <sys/sem.h>
#include <stdbool.h>

void Deliver(int id, int shmid, int semid, int N);
void CreateDelivery(int shmid, int semid, struct Argument* st);
void Kitchener(int id, int shmid, int semid, int N);
void CreateKitchener(int shmid, int semid, struct Argument* st);
void royal_print(int N, int* buffer);

int GetVal_safe(int semid, int num_in_sem);
int create_semaphore(const char* name, int flags, struct Argument* st);
void delete_semaphore(int semid);

struct sembuf wait_kitcheners[] = {{1, 0, 0}, {0, -1, 0}};     // waiting for entering

int main(int argc, char* argv[]) {

  struct Argument st;
  get_arg(argc, argv, &st);

  int shmid = shmget(ftok("main.c", 1), st.N*sizeof(int), IPC_CREAT | 0644);
  if (shmid < 0) {
    perror("shmget");
    exit(EXIT_FAILURE);
  }

  int semid = create_semaphore("main.c", IPC_CREAT|0644, &st);

  int *buffer = shmat(shmid, NULL, 0);
  if (buffer == (int *)-1) {
    perror("shmat");
    exit(1);
  }

  for (int i = 0; i < st.N; i++)
    buffer[i] = free_table;

  royal_print(st.N, buffer);

  CreateKitchener(shmid, semid, &st);
  CreateDelivery (shmid, semid, &st);

  printf("i am here\n");
  RunOp_safe(semid, wait_kitcheners, 2);
  //on this stage all kitcheners are ready to start their jobs
  
  for (int i = 0; i < st.K+st.D; i++)
    wait(NULL);

  del_shm(shmid);
  delete_semaphore(semid);

  return 0;

}


void CreateKitchener(int shmid, int semid, struct Argument* st) {
  for (int i = 0; i < st->K; i++) {
    pid_t pid = fork_safe();

    if (pid == -1) {
      fprintf(stderr, "fork: %s\n", strerror(errno));
      exit(1);
    }

    if (pid == 0) {
      Kitchener(i+1, shmid, semid, st->N);
      return;
    }
  } 
}

void CreateDelivery(int shmid, int semid, struct Argument* st) {
  for (int i = 0; i < st->D; i++) {
    pid_t pid = fork_safe();

    if (pid == -1) {
      fprintf(stderr, "fork: %s\n", strerror(errno));
      exit(1);
    }

    if (pid == 0) {
      Deliver(i+1, shmid, semid, st->N);
      return;
    }
  } 
}

void Deliver(int id, int shmid, int semid, int N)
{
  struct sembuf delivery_enter[] = {{2, -1, 0}};// decreasing sem with numbers of kitcheners
  struct sembuf wait_enter[]     = {{0, 0, 0}};     // waiting for entering
  struct sembuf cook_pizza_P[] = {{3, -1, 0}};
  struct sembuf cook_pizza_V[] = {{3, 1, 0}};
  struct sembuf pizza_is_ready[] = {{4, -1, 0}};
  
  int* buffer = (int*)shmat(shmid, NULL, 0);

  if (buffer == (int*)-1) {
    perror("shmat");
    exit(EXIT_FAILURE);
  }
  
  RunOp_safe(semid, delivery_enter, 1);
  RunOp_safe(semid, wait_enter, 1); // waiting for a signal to start
  printf("Deliver %d: Ready!\n", id);

  bool take_pizza = true;
  //bool flag = false;
  while(take_pizza) {
    RunOp_safe(semid, cook_pizza_P, 1);
    for (int i = 0; i < N; i++) {
      if (buffer[i] == pizza_ready) {
        RunOp_safe(semid, pizza_is_ready, 1);
        buffer[i] = free_table; //flag = true;
        printf("Deliver %d: I am taking pizza from table %d\n", id, i+1);
        royal_print(N, buffer);
        break;
      }
    }

    if (GetVal_safe(semid, 4) == 0) 
      take_pizza = false; //no more pizzas on the table
    
    RunOp_safe(semid, cook_pizza_V, 1);
  }

  printf("exit deliver\n");

  exit(0);
}

void Kitchener(int id, int shmid, int semid, int N)
{
  struct sembuf kitchener_enter[] = {{1, -1, 0}};
  struct sembuf wait_enter[] = {{0, 0, 0}};// waiting for entering
  struct sembuf cook_pizza_P[] = {{3, -1, 0}};
  struct sembuf pizza[] = {{4, 1, 0}};
  struct sembuf cook_pizza_V[] = {{3, 1, 0}};

  int* buffer = (int*)shmat(shmid, NULL, 0);

  if (buffer == (int*)-1) {
    perror("shmat");
    exit(EXIT_FAILURE);
  }
  
  //printf("Cooker %d: Buffer address: %p\n", id, buffer);
 
  RunOp_safe(semid, kitchener_enter, 1);
  RunOp_safe(semid, wait_enter, 1);  //Z()
  printf("Cooker %d: Ready!\n", id);

  int cooked_pizzas = 0;
  int table_index = 0;
  bool flag = false;
  while(cooked_pizzas != 5) {
    RunOp_safe(semid, cook_pizza_P, 1);
    for (int i = 0; i < N; i++) {
      if (buffer[i] == free_table) {
        RunOp_safe(semid, pizza, 1);
        table_index = i;
        buffer[i] = pizza_cooking;
        printf("Cooker %d: I am cooking %d on table %d\n", id, cooked_pizzas+1, i+1);
        royal_print(N, buffer);
        flag = true;
        break;
      }
    }
    
    RunOp_safe(semid, cook_pizza_V, 1);

    if (flag) {
      sleep(1); // LET HIM COOK!!!
      buffer[table_index] = pizza_ready; cooked_pizzas++;
      flag = false;
    }
  }

  printf("exit kitchen\n");
  
  exit(0);
}

int create_semaphore(const char* name, int flags, struct Argument* st) {
  // sem_array = | Able_to_enter | K | D | Able_to_make_pizza | pizzas | 
  int semid = semget(ftok(name, 1), 5, flags);
  if (semid == -1) {
    perror("unable to create semphore\n");
    exit(EXIT_FAILURE);
  }

  semctl(semid, 0, SETVAL, 1);
  semctl(semid, 1, SETVAL, st->K);
  semctl(semid, 2, SETVAL, st->D);
  semctl(semid, 3, SETVAL, 1);
  semctl(semid, 4, SETVAL, 0);
  
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

int GetVal_safe(int semid, int num_in_sem) {
  int return_value = semctl(semid, num_in_sem, GETVAL);
  if (semctl(semid, num_in_sem, GETVAL) < 0) {
    perror("semop GetVal error\n");
    exit(EXIT_FAILURE);
  }

  return return_value;
}
