#include "shower.h"

static int GetVal_safe(int semid, int num_in_sem);
static void SetVal_safe(int semid, int num_in_sem, int value);
static void RunOp_safe(int semid, struct sembuf *op, size_t nop); 

int main(int argc, char* argv[]) {
  //order N M W
  if (argc != 4) {
    fprintf(stderr, "Incorrect input data\n");
    return 0;
  }

  struct Shower_enjoers st = {
    atoi(argv[1]),
    atoi(argv[2]),
    atoi(argv[3])
  };
 

  int semid = create_semaphore("main.c", IPC_CREAT, &st);
   // sem_array = | M/W? | Able_to_enter | N | M | W |

  init_processes(&st, semid);

  printf("Shower is open!\n");
  SetVal_safe(semid, 1, 0); //signal for all processes that shower is open

  int status;
  for (size_t i = 0; i < st.M + st.W; i++) {
    wait(&status);
  }

  delete_semaphore(semid);
  return 0;
}

void init_processes(struct Shower_enjoers* st, int semid) {
  for (size_t i = 0; i < st->M + st->W; i++) {
    pid_t pid = fork();

    if (pid == -1) {
      fprintf(stderr, "fork: %s\n", strerror(errno));
      exit(FAILURE);
    }

    if (pid == 0) {
      if (i >= st->M)
        shower_visitor(Woman, semid, (int)i-st->M+1);
      else
        shower_visitor(Man, semid, (int)(i+1));
      return;
    }
  } 
  
  return;
}

void delete_semaphore(int semid) {
  if (semctl(semid, IPC_RMID, 0) < 0) {
    perror("unable to delete semaphore\n");
    exit(EXIT_FAILURE);
  }

  printf("Semaphore is deleted\n");
}

int create_semaphore(const char* name, int flags, struct Shower_enjoers* st) {
  // sem_array = | M/W? | Able_to_enter | N | M | W |
  int semid = semget(ftok(name, 1), 5, flags | 0644);
  if (semid == -1) {
    perror("unable to create semphore\n");
    exit(EXIT_FAILURE);
  }

  semctl(semid, 0, SETVAL, 0);
  semctl(semid, 1, SETVAL, 1);
  semctl(semid, 2, SETVAL, st->N);
  semctl(semid, 3, SETVAL, st->M);
  semctl(semid, 4, SETVAL, st->W);
  
  printf("Semaphore is created\n");

  return semid;
}

void shower_visitor(int gender, int semid, int index) {
  if(gender == Man) {
    printf("Man %d: need to shower\n", index);
  } else {
    printf("Woman %d: need to shower\n", index);
  }
  struct sembuf inc[] = {{2, 1, 0}};// incrementing number of visitors
  struct sembuf dec[] = {{2, -1, 0}};// decrementing number of visitors
  struct sembuf dec_M[] = {{Man, -1, 0}};// decrementing number of visitors
  struct sembuf dec_W[] = {{Woman, -1, 0}};// incrementing number of visitors
  struct sembuf wait_enter[] = {{1, 0, 0}, {1, 1, 0}};// waiting for entering

  //Z()&V()
  RunOp_safe(semid, wait_enter, 2);
  
  int shower_gender;
  do {
    shower_gender = GetVal_safe(semid, 0);
  } while(!(GetVal_safe(semid, 2) > 0 && (shower_gender == gender || 
                                    shower_gender == NotSet)));
  if(gender == Man) {
    printf("Man %d: in\n", index);
  } else {
    printf("Woman %d: in\n", index);
  }
  // int n1 = GetVal_safe(semid, 0);
  // int n2 = GetVal_safe(semid, 1);
  // int n3 = GetVal_safe(semid, 2);
  // int n4 = GetVal_safe(semid, 3);
  // int n5 = GetVal_safe(semid, 4);
  // printf("| %d | %d | %d | %d | %d |\n", n1, n2, n3, n4, n5);

  if (shower_gender == NotSet) {
    SetVal_safe(semid, 0, gender);
  }

  RunOp_safe(semid, dec, 1); //somebody took a place in shower

  //P()
  SetVal_safe(semid, 1, 0); //allowing somebody else to enter shower
    
  sleep(1); //taking shower
  
  if (gender == Man)
    RunOp_safe(semid, dec_M, 1); //somebody took a place in shower
  else
    RunOp_safe(semid, dec_W, 1); //somebody took a place in shower

  if(gender == Man) {
    printf("Man %d: out\n", index);
  } else {
    printf("Woman %d: out\n", index);
  }

  if (GetVal_safe(semid, Man) == 0)
    SetVal_safe(semid, 0, Woman);

  if (GetVal_safe(semid, Woman) == 0)
    SetVal_safe(semid, 0, Man);
  
  RunOp_safe(semid, inc, 1); //somebody spare a place in shower

  exit(SUCCESS);

  //could't enter shower
  //P()
  SetVal_safe(semid, 1, 0); //allowing somebody else to enter shower
  exit(SUCCESS);
}

void SetVal_safe(int semid, int num_in_sem, int value) {
  if (semctl(semid, num_in_sem, SETVAL, value) < 0) {
      perror("semop SetVal error\n");
      exit(EXIT_FAILURE);
  }
}

int GetVal_safe(int semid, int num_in_sem) {
  int return_value = semctl(semid, num_in_sem, GETVAL);
  if (semctl(semid, num_in_sem, GETVAL) < 0) {
    perror("semop GetVal error\n");
    exit(EXIT_FAILURE);
  }

  return return_value;
}

void RunOp_safe(int semid, struct sembuf *op, size_t nop) {
  if (semop(semid, op, nop) < 0) {
    perror("semop RunOp error\n");
    exit(EXIT_FAILURE);
  }
}
