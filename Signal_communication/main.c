#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/wait.h>



char received_msg[128] = {0};
pid_t parent_pid;
volatile sig_atomic_t flag = 0;
volatile sig_atomic_t syllable = 0;
volatile sig_atomic_t pos = 0;

void bit_hdlr(int signal);
void send_message(const char* msg, pid_t pid, sigset_t* mask);
void add_bit(bool bit);
void send_bit(pid_t pid, bool bit);
void handler(int signal);
void last_word_hdlr(int signal); 

int main() {
  pid_t pid = fork();
  if (pid < 0) perror("fork");
  if (pid == 0) {
    parent_pid = getppid();

    // setting mask to prevent signals from colision
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1); 
    sigaddset(&mask, SIGUSR2); 

    struct sigaction sg_new = {
      .sa_handler = bit_hdlr,
      .sa_mask = mask,
      .sa_flags = 0,
    };

    struct sigaction sg_old;

    sigaction(SIGUSR1, &sg_new, &sg_old);
    sigaction(SIGUSR2, &sg_new, &sg_old);
    //------------------------------------

    sigset_t mask_end;
    sigemptyset(&mask_end);
    sigaddset(&mask_end, SIGTERM);

    struct sigaction sg_new_end = {
      .sa_handler = last_word_hdlr,
      .sa_mask = mask_end,
      .sa_flags = 0,
    };

    struct sigaction sg_old_end;
    sigaction(SIGTERM, &sg_new_end, &sg_old_end);

    while(1) sleep(1);

    exit(1);
  }

  sleep(1);

  sigset_t new_mask;
  sigemptyset(&new_mask);
  sigaddset(&new_mask, SIGUSR1);

  struct sigaction sg_new = {
    .sa_handler = handler,
    .sa_mask = new_mask,
    .sa_flags = 0,
  };

  struct sigaction sg_old;
  sigaction(SIGUSR1, &sg_new, &sg_old);

  sigfillset(&new_mask);
  sigdelset(&new_mask, SIGUSR1);
  sigdelset(&new_mask, SIGUSR2); 

  const char * msg = "Hello, world!\n";
  
  send_message(msg, pid, &new_mask); // sending messages to the child

  kill(pid, SIGTERM);
  wait(NULL);
  printf("Communication is ended\n");
  
  return 0;
}

void bit_hdlr(int signal) {
  // const char* buf = "bit_hdlr\n";
  // write(1, buf, strlen(buf));
  if (signal == SIGUSR1)
    add_bit(1);
  if (signal == SIGUSR2)
    add_bit(0);

  // write(1, received_msg, strlen(received_msg));
  // const char* bye = "sending signal to parent\n";
  // write(1, bye, strlen(bye));
  kill(parent_pid, SIGUSR1);
}

void handler(int signal) {
  // const char* buf = "handler\n";
  // write(1, buf, strlen(buf));
  if (signal == SIGUSR1) {
    flag = 1;
  }
}

void send_bit(pid_t pid, bool bit) {
  if (bit)
    kill(pid, SIGUSR1);
  else
    kill(pid, SIGUSR2);
}

void add_bit(bool bit) {
  received_msg[syllable] = received_msg[syllable] | (bit << pos);
  pos+=1;
  if (pos == 8) {
    syllable+=1; pos = 0;
  }
}

void send_message(const char* msg, pid_t pid, sigset_t* mask) {
  for (size_t i = 0; i < strlen(msg); i++) {
    for (size_t j = 0; j < 8; j++) {
      flag = 0;
      //printf("msg[%zu]&(1<<%zu) = %d\n", i , j, msg[i]&(1<<j));
      send_bit(pid, msg[i]&(1<<j));
      while (!flag) {
        sigsuspend(mask);
      }
    }
  }
}

void last_word_hdlr(int signal) {
  write(1, received_msg, strlen(received_msg));
  _exit(0);
}
