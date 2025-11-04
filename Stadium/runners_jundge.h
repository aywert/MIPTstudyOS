#ifndef RUNNERS
#define RUNNERS

#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/msg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <sys/wait.h>

struct message_st {
  int direction;
  char data[16];
};

#define FAILURE_STATUS -1
#define SUCCESS_STATUS 1

static const long MSG_MAX = 10;
static const long MSG_SIZE = 8192;

int  runner(int runner_n, mqd_t* queue_array, int N);
int  judge(mqd_t* queue_array, int N);
void init_runners(mqd_t* queue_array, int N);

void   queues_for_runners_delete(mqd_t* queue_array);
mqd_t* queues_for_runners(int msgflg, struct mq_attr* mq_attr);
void   destruct_queue(const char* queue_name, mqd_t queue_id); 
mqd_t  create_queue(const char *name, int msgflg, struct mq_attr* mq_attr);

#endif