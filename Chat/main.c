#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define MAX_USERS 128 
#define MSG_LENGTH 256

//int signal_1 = SIGRTMIN;

struct members {
  int semid;
  int chatters;
  int members[MAX_USERS];
  char msg[MAX_USERS][MSG_LENGTH];
};

int create_semaphore(const char* name, int flags); 
void delete_semaphore(int semid); 
void RunOp_safe(int semid, struct sembuf *op, size_t nop); 
struct sembuf P[] = {{0, -1, 0}};
struct sembuf V[] = {{0,  1, 0}};

void del_shm(int shmid);
void attach_to_shm(struct members** mem, int shmid);
void request_hdlr(int signal, siginfo_t* info, void *);
void dummy_hdlr(int signal, siginfo_t* info, void *);


int join(int argc, char* argv[], pid_t pid);
void Chat(void);
void send_msg(pid_t pid, struct members* mem, const char* msg);
void msg_hdlr(int signal, siginfo_t* info, void *); 

int shmid = 0;
int my_index = -1;
struct members* mem;

//bool talking = true;
int main(int argc, char* argv[]) {
  pid_t my_pid = getpid();
  printf("my pid = %d\n", my_pid);
  
  if (join(argc, argv, my_pid) < 0) {
    fprintf(stderr, "join couldnt be emplemented\n");
    return 0;
  }
 
  printf("numbers of chatters %d\n", mem->chatters);
  Chat();

  int semid = 0;
  RunOp_safe(mem->semid, P, 1);
  if (mem->chatters == 1) {
    semid = mem->semid;
    del_shm(shmid);
  }
  else mem->chatters--;
  RunOp_safe(mem->semid, V, 1); 

  del_shm(shmid);
  if (semid) delete_semaphore(semid); 
  return 0;
}

void Chat(void) {
  bool talking = true;
  char msg[32] = {};
  while(talking) {

    printf("MAX> ");
    fflush(stdout);

    scanf("%s", msg);
    if (!strncmp(msg, "bye", 3)) talking = false;

    else if (!strncmp(msg, "tell", 4)) {
      pid_t pid_to_tell = 0;
      char buffer[MSG_LENGTH] = {};
      scanf("%d %s", &pid_to_tell, buffer);
      if (pid_to_tell == 0) talking = false;

      else { 
        printf("it is tell and pid = %d\n", pid_to_tell);
        send_msg(pid_to_tell, mem, buffer);
      }
    }

    else printf("no such command. Try again.\n");
  }
}

int join(int argc, char* argv[], pid_t pid) {
  if (argc > 2) {
    printf("Incorrect input\n");
    return 0;
  }

  struct sigaction sg_msg = {
    .sa_sigaction = msg_hdlr,
    .sa_flags = SA_SIGINFO,
    };

  sigaction(SIGRTMIN+1, &sg_msg, NULL);
  //printf("msg_hdlr is installed\n");

  struct sigaction sg_request = {
    .sa_sigaction = request_hdlr,
    .sa_flags = SA_SIGINFO,
    };

  sigaction(SIGRTMIN, &sg_request, NULL);
  //printf("request_hdlr is installed\n");

  struct sigaction sg_accept = {
    .sa_sigaction = dummy_hdlr,
    .sa_flags = SA_SIGINFO,
    };

  sigaction(SIGRTMAX, &sg_accept, NULL);
  //printf("dummy_hdlr is installed\n");

  if (argc == 1) { //создаем разделяемую память
    shmid = shmget(ftok("main.c", 1), sizeof(struct members), IPC_CREAT | 0644);
    if (shmid < 0) {
      perror("shmget");
      exit(EXIT_FAILURE);
    }

    attach_to_shm(&mem, shmid);
    mem->members[0] = pid;
    my_index = 0;
    mem->semid = create_semaphore("main.c", IPC_CREAT);
  }

  else {
    const union sigval val = { .sival_int = 0};
    sigqueue(atoi(argv[1]), SIGRTMIN, val);

    siginfo_t info; sigset_t new_mask;
    sigemptyset(&new_mask);
    sigaddset(&new_mask, SIGRTMAX);

    sigwaitinfo(&new_mask, &info);

    shmid = info.si_value.sival_int;
    
    printf("got information from main\n");
    attach_to_shm(&mem, shmid);
    for (int i = 0; i < MAX_USERS; i++) {
      if (mem->members[i] == pid) {
        my_index = i;
        break;
      }
    }
    printf("members:\n");
    for (int i = 0; i < 10; i++) printf("%d\n", mem->members[i]);
  }

  printf("my shmid = %d\n", shmid);

  RunOp_safe(mem->semid, P, 1);
  mem->chatters++;
  RunOp_safe(mem->semid, V, 1); 

  return 0;
}


void del_shm(int shmid) {
  if (shmctl(shmid, IPC_RMID, NULL) < 0) {
    perror("shmctl IPC_RMID");
    exit(EXIT_FAILURE);
  }
}

void attach_to_shm(struct members** mem, int shmid) {
  *mem = shmat(shmid, NULL, 0);
  if (mem == (struct members**)-1) {
    perror("shmat");
    exit(1);
  }
}

//---------------------------------------
//SECTION WITH COMMINICATION
//---------------------------------------

void request_hdlr(int signal, siginfo_t* info, void *) {

  RunOp_safe(mem->semid, P, 1);
  for (int i = 0; i < MAX_USERS; i++) {
    if (mem->members[i] == 0) {
      mem->members[i] = info->si_pid;
      break;
    }
  }
  RunOp_safe(mem->semid, V, 1);
  
  const union sigval val = { .sival_int = shmid };
  sigqueue(info->si_pid, SIGRTMAX, val);
}

void dummy_hdlr(int signal, siginfo_t* info, void *) {
  return;
}

void send_msg(pid_t pid, struct members* mem, const char* msg) {
  RunOp_safe(mem->semid, P, 1);

  int index = -1;
  for (int i = 0; i < MAX_USERS; i++) {
    if (mem->members[i] == pid) {
      index = i;
      break;
    }
  }

  if (index == -1) {
    printf("Couldn't find index by pid\n");
    return;
  } 

  strncpy(mem->msg[index], msg, MSG_LENGTH-1);
  printf("sending: '%s'\n", mem->msg[index]);
  const union sigval val = { .sival_int = 0};
  sigqueue(pid, SIGRTMIN+1, val);

  RunOp_safe(mem->semid, V, 1);
  return;
}

void msg_hdlr(int signal, siginfo_t* info, void *) {
  char buffer[2*MSG_LENGTH] = {};

  snprintf(buffer, 2*MSG_LENGTH, "New massage from %d: %s\n", info->si_pid, mem->msg[my_index]);
  write(1, buffer, MSG_LENGTH);
  return;
}

//---------------------------------------
//-------SECTION WITH SEMAPHORES---------
//---------------------------------------

int create_semaphore(const char* name, int flags) {
// sem_array = | able to enter |
  int semid = semget(ftok(name, 1), 1, flags | 0644);
  if (semid == -1) {
    perror("unable to create semphore");
    exit(EXIT_FAILURE);
  }

  semctl(semid, 0, SETVAL, 1);
  //printf("Semaphore is created\n");

  return semid;
}

void delete_semaphore(int semid) {
  if (semctl(semid, IPC_RMID, 0) < 0) {
    perror("unable to delete semaphore\n");
    exit(EXIT_FAILURE);
  }

  //printf("Semaphore is deleted\n");
}

void RunOp_safe(int semid, struct sembuf *op, size_t nop) {
  if (semop(semid, op, nop) < 0) {
    perror("semop RunOp error");
    exit(EXIT_FAILURE);
  }
}