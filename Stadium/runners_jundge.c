#include "runners_jundge.h"   
                                                        
mqd_t create_queue(const char *name, int msgflg, struct mq_attr* mq_attr) {
  mqd_t queue_id = mq_open(name, msgflg, 0644, mq_attr);
  if(queue_id == (mqd_t)-1) {
    fprintf(stderr, "Failed to create queue: %s\n", 
            strerror(errno));
    printf("i am deadhere\n");
    exit(FAILURE_STATUS); 
  }

  return queue_id; 
}

mqd_t* queues_for_runners(int msgflg, struct mq_attr* mq_attr) {
  const char* name[3] = {"/judge", "/runners", "/ready_queue"};
  mqd_t* queue_array = (mqd_t*)calloc(sizeof(mqd_t), 3);
  
  for (size_t i = 0; i < 3; i++) {
    printf("name[%ld] = %s\n", i, name[i]);
    mqd_t queue_id = create_queue(name[i], msgflg, mq_attr);
    queue_array[i] = queue_id;
  }

  return queue_array; 
}

void queues_for_runners_delete(mqd_t* queue_array) {
  const char* name[3] = {"/judge", "/runners", "/ready_queue"};
  for (size_t i = 0; i < 3; i++) {
    destruct_queue(name[i], queue_array[i]);
  }

  free(queue_array); queue_array = NULL;
}

void destruct_queue(const char* queue_name, mqd_t queue_id) {
  if (queue_id == (mqd_t)-1) {
    fprintf(stderr, "invalid file descriptor\n");
  }
  else if(mq_close(queue_id) == -1) {
    fprintf(stderr, "Failed to destruct queue: %s\n", 
            strerror(errno));
  }

  if(mq_unlink(queue_name) == -1) {
    if (errno != ENOENT) {
      fprintf(stderr, "Failed to destruct queue: %s\n", 
              strerror(errno));
      exit(FAILURE_STATUS); 
    }
    else {
      fprintf(stderr, "Queue is already destructed: %s\n", 
              strerror(errno));
    }
  }
  else  {
    printf("everything is OK\n");
  }
}

int judge(mqd_t* queue_array, int N) {
  //| comunication with judge| comunication with runners |ready status|
  char buffer[MSG_SIZE];
  char message[16];
  unsigned int msg_prio;

  for (int i = 0; i < N; i++) {
    if (mq_receive(queue_array[2], buffer, MSG_SIZE, &msg_prio)==-1) {
      fprintf(stderr, "Judge: failed to receive message from runner: %s\n", 
              strerror(errno));
      exit(FAILURE_STATUS);
    }

    printf("Judge: runner %s is ready!\n", buffer);
    memset(buffer,'\0', MSG_SIZE);
  }

  printf("Judge: Great! Everyone is ready! Start!\n");

  snprintf(message, sizeof(message), "1");
  printf("Judge: sent a start signal!\n");
  if (mq_send(queue_array[0], message, strlen(message)+1, 1) == -1) {
    perror("mq_send");
  }

  int status;
  for (long i = 0; i < N; i++)
    wait(&status);

  printf("Judge: race is over!\n");
  return 0;
}

int runner(int runner_n, mqd_t* queue_array, int N) {
  
  char message[16];
  char buffer[MSG_SIZE];
  unsigned int msg_prio;
  
  snprintf(message, sizeof(message), "%d", runner_n);
  printf("runner %d: Ready!\n", runner_n);
  if (mq_send(queue_array[2], message, strlen(message)+1, 1) == -1) {
    perror("mq_send");
  }

  bool run = true;
  while (run) {
    if (mq_receive(queue_array[0], buffer, MSG_SIZE, &msg_prio) == -1) {
      perror("runner");
      exit(1);
    }

    //printf("Runner %d: Got message %s Buffer %s\n", runner_n, message, buffer);
    if (atoi(buffer) == runner_n) {
      //printf("Runner %d: received message!\n", runner_n);
      if (runner_n < N) {
          printf("Runner %d: giving estapheta to the next runner\n", runner_n);
          snprintf(message, sizeof(message), "%d", runner_n + 1);
          mq_send(queue_array[0], message, strlen(message)+1, 2);
      } else {
          printf("Runner %d: i finished!\n", runner_n);
      }
      run = false;
    }
    else {
      //printf("Runner %d: Got message %s Buffer %s\n", runner_n, message, buffer);,
      snprintf(message, sizeof(message),"%s", buffer);
      if (mq_send(queue_array[0], message, strlen(message)+1, 1)==-1)
        perror("send issue");
      //sleep(1);
    }
  }

  for (int i = 0; i < 3; i++) {
    mq_close(queue_array[i]);
  }

  exit(SUCCESS_STATUS);
}

void init_runners(mqd_t* queue_array, int N) {
  for (int i = 0; i < N; i++) {
    pid_t pid = fork();

    if (pid == -1) {
      fprintf(stderr, "fork: %s\n", strerror(errno));
      exit(1);
    }

    if (pid == 0) {
      runner(i+1, queue_array, N);
      return;
    }
  } 
  
  return;
}
