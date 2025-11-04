#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/msg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

enum message_type {
  NO_MSG = 0,
  START_MSG = -1, 
  READY_MSG = -2,
};

struct msgbuf {
  long mtype;       /* message type, must be > 0 */
  int  msg;    /* message data */
};

#define FAILURE_STATUS -1
#define SUCCESS_STATUS 1

int runner(int runner_n, int id, int N);
int judge(int queue_id, int N);
int create_queue(int msgflg);
void init_runners(int queue_id, int N);