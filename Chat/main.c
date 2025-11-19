#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/shm.h>

#define MAX_USERS 128 

struct members {
  int members[MAX_USERS];
};

//int SIGNAL = SIGRTMIN;

void del_shm(int shmid);
void attach_to_shm(struct members** mem, int shmid);
void request_hdlr(int signal, siginfo_t* info, void *);
void dummy_hdlr(int signal, siginfo_t* info, void *);
void Chat(void);

int shmid = 0;
struct members* mem;

//bool talking = true;
int main(int argc, char* argv[]) {
  pid_t my_pid = getpid();
  printf("my pid = %d\n", my_pid);
  
  if (argc > 2) return 0;
  if (argc == 1) { //если не указан shmid, то создаем разделяемую память
    shmid = shmget(ftok("main.c", 1), 128*sizeof(int), IPC_CREAT | 0644);
    if (shmid < 0) {
      perror("shmget");
      exit(EXIT_FAILURE);
    }

    struct sigaction sg_new = {
    .sa_sigaction = request_hdlr,
    .sa_flags = SA_SIGINFO,
    };

    struct sigaction sg_old = {};
    sigaction(SIGRTMIN, &sg_new, &sg_old);
    printf("handler is installed\n");
  }
  else {
    struct sigaction sg_new = {
    .sa_sigaction = dummy_hdlr,
    .sa_flags = SA_SIGINFO,
    };

    struct sigaction sg_old = {};
    sigaction(SIGRTMAX, &sg_new, &sg_old);

    const union sigval val = { .sival_int = 0};
    sigqueue(atoi(argv[1]), SIGRTMIN, val);
    siginfo_t info;
    sigset_t new_mask;
    sigemptyset(&new_mask);
    sigaddset(&new_mask, SIGRTMAX);
    
    sigwaitinfo(&new_mask, &info);
    shmid = info.si_value.sival_int;
    printf("got information from main\n");
  }

  printf("my shmid = %d\n", shmid);

  attach_to_shm(&mem, shmid);
 
  Chat();
  //while(talking) sleep(1);

  del_shm(shmid);  

  return 0;
}

void Chat(void) {
  bool talking = true;
  char msg[32] = {'\0'};
  while(talking) {
    printf("MAX: ");
    scanf("%s", msg);
    if (!strncmp(msg, "bye", 3)) talking = false;
    else if (!strncmp(msg, "tell", 4)) {
      pid_t pid_to_tell = 0;
      scanf("%d", &pid_to_tell);
      if (pid_to_tell == 0) talking = false;
      printf("it is tell and pid = %d\n", pid_to_tell);
    }

    else printf("%s\n", msg);
    //else printf("%s\n", msg);

  }
}

// void tell_pid(pid_t pid) {

// }

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
//SECTION WITH INITIALISING COMMINICATION
//---------------------------------------

void request_hdlr(int signal, siginfo_t* info, void *) {
  const union sigval val = { .sival_int = shmid };
  
  for (int i = 0; i < MAX_USERS; i++)
    if (mem->members[i] == 0) mem->members[i] = info->si_pid;

  sigqueue(info->si_pid, SIGRTMAX, val);
}

void dummy_hdlr(int signal, siginfo_t* info, void *) {
  return;
}

