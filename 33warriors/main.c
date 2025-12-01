#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/msg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

enum message_type {
  NO_MSG = 0,
  TERMINATE_MSG = 1, 
  REQUEST_MSG = 2,
  ACCEPT_MSG = 3,
  REJECT_MSG = 4,
  COMMIT_MSG = 5,
};

struct msgbuf {
  long mtype;       /* message type, must be > 0 */
  size_t position;
  int  queue_id;
  char letter;    /* message data */
};

#define FAILURE_STATUS -1
#define SUCCESS_STATUS 1

void warrior(int queue_id, int* queue_array, char* song);
int create_queue(key_t key, int msgflg);
void init_warriors(int N, int* queue_array, char* song);
int count_letters(char* line);


void send_message(int queue_id, int msg_type, char letter);
void broadcast(int* queue_array, size_t array_len, int msg_type, char letter);

int main(int argc, char* argv[]) {

  if (argc < 2) {
    perror("invalid input");
    return 0;
  }

  int letters = count_letters(argv[1]);
  int* queue_array = (int*)calloc(letters, sizeof(int));
  init_warriors(letters, queue_array, argv[1]);

//   int queue_id = create_queue(IPC_CREAT|0666);
//   struct msqid_ds My_st = {};

//   init_warriors(queue_id, N);  //starts N processes
//   msgctl(queue_id, IPC_RMID, &My_st); //deleting the queue of messages

  for (int j = 0; j < letters; j++)
    wait(NULL);

  free(queue_array);
  return 0;
}

void warrior(int queue_id, int* queue_array, char* song) {
  char set[256] = {};
  size_t index = 0;

  int msg_status = NO_MSG;
  size_t count_replies = 0;

  char my_letter = '\0';
  size_t* my_positions[256] = {};
  size_t num_of_warriors = 0;
  struct msgbuf msg = {};
    
  bool running = true;
  while (running) {
    int ret = msgrcv(queue_id, &msg, sizeof(char), 0, IPC_NOWAIT);

    if (ret != -1) {
      process_message(&msg, &my_letter, &my_positions, &set, &index, &msg_status, &count_replies, num_of_warriors);

      if (msg.mtype == TERMINATE_MSG) {
        running = 0;
      }
    }
    else if (errno != ENOMSG) {
      perror("msgrcv");
      running = 0;
    }
  }

  return;
}

void process_message(struct msgbuf* msg, char* my_letter, size_t** my_position, 
                     char* set, size_t* index, int* status, size_t* count_replies, size_t num_of_warriors) {
  switch(msg->mtype) {
    case REQUEST_MSG: {
      if (*my_letter == msg->letter) {
        //set[*index++] = msg->letter;
        send_message(msg->queue_id, REJECT_MSG, msg->letter);
      }
      else send_message(msg->queue_id, ACCEPT_MSG, msg->letter);
      break;
    }
    case ACCEPT_MSG: {
      if (*status == REQUEST_MSG) {
        *count_replies++;
        if (count_replies == num_of_warriors-1) {
          *my_letter = msg->letter;
          send_message(msg->queue_id, COMMIT_MSG, msg->letter);
        }
      }
      break;
    }

    case REJECT_MSG: {
      //no do much
      break;
    }

    case COMMIT_MSG: {
      //update corrent state somehow
      break;
    }

    default: {
      printf("what is going on?\n");
      fflush(stdout);
      break;
    }
  }
}


int create_queue(key_t key, int msgflg) {
  int queue_id = msgget(key, msgflg);
  if(queue_id == -1) {
    fprintf(stderr, "creat_queue failed: %s\n", strerror(errno));
    exit(1); 
  }

  return queue_id; 
}

void init_warriors(int N, int* queue_array, char* song) {
  for (int i = 0; i < N; i++) {
    char ch = i;
    queue_array[i] = create_queue(ch, IPC_CREAT|0666);
    pid_t pid = fork();

    if (pid == -1) {
      fprintf(stderr, "fork: %s\n", strerror(errno));
      exit(1);
    }

    if (pid == 0) {
      warrior(queue_array[i], queue_array, song);
      printf("i am here\n");
      msgctl(queue_array[i], IPC_RMID, NULL); // deleting queue
      return;
    }
  } 
  
  return;
}

int count_letters(char* line) {
  int count = 0; 
  char set[256] = {}; 

  for (int i = 0; line[i] != '\0'; i++) {
    unsigned char c = (unsigned char)line[i];
    if (!set[c]) {
      set[c] = 1;
      count++;
    }
  }

  return count;
}

void broadcast(int* queue_array, size_t array_len, int msg_type, char letter) {
  for (size_t i = 0; i < array_len; i++) {
    if (queue_array[i] != 0)
      send_message(queue_array[i], msg_type, letter);
  }
}

void send_message(int queue_id, int msg_type, char letter)
{
  struct msgbuf buf = {.mtype = msg_type, .letter = letter, .queue_id = queue_id};

  if (msgsnd(queue_id, &buf, sizeof(char), 0) == -1) {
    fprintf(stderr, "Failed to send message: %s\n", strerror(errno));
    exit(FAILURE_STATUS); 
  }

  return;
}

