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

enum letter_status {
  not_taken = 0,
  requested = 1,
  commited  = 2,
  accepted  = 3,
  rejected  = 4,
};

struct msgbuf {
  long mtype;       /* message type, must be > 0 */
  size_t position;
  int  queue_id;
  char letter;    /* message data */
};

struct letter_state {
  char letter;
  size_t position;
  enum letter_status status;
  size_t replies; 
};

struct warrior_state {
  struct letter_state complex_song[128];
  size_t warriors_num;
  char my_letter;
  int my_positions[64];
  size_t index_of_the_last_position;
  pid_t my_pid;
};

#define FAILURE_STATUS -1
#define SUCCESS_STATUS 1

void process_message(struct msgbuf* msg, struct warrior_state* st);
void warrior(int queue_id, int* queue_array, char* song, size_t warriors_num);
int create_queue(key_t key, int msgflg);
void init_warriors(int N, int* queue_array, char* song);
int count_letters(char* line);


void send_message(int queue_id, int msg_type, char letter, size_t position);
void broadcast(int* queue_array, size_t array_len, int msg_type, char letter, int queue_id, size_t position);

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

void warrior(int queue_id, int* queue_array, char* song, size_t warriors_num) {
  
  struct warrior_state st;

  for (size_t i = 0; i < warriors_num; i++) {
    st.complex_song[i].letter = song[i];
    st.complex_song[i].status = not_taken;
  }
  st.index_of_the_last_position = 0;
  st.my_pid = getpid();
  st.my_letter = '\0';
  st.warriors_num = warriors_num;

  for (int i = 0; i < 64; i++)
    st.my_positions[i] = -1; 

  size_t index = 0;
  struct msgbuf msg = {};
    
  bool running = true;
  while (running) {
    int ret = msgrcv(queue_id, &msg, sizeof(struct msgbuf) - sizeof(long), 0, IPC_NOWAIT);

    if (ret != -1) {
      process_message(&msg, &st);

      if (msg.mtype == TERMINATE_MSG) {
        running = 0;
      }
    }
    else if (errno != ENOMSG) {
      perror("msgrcv");
      running = 0;
    }
    else {
      while (st.complex_song[index].status != not_taken) {
        index++; 
        if (index == warriors_num && st.my_letter == '\0') index = 0; 
      }
      broadcast(queue_array, warriors_num, REQUEST_MSG, st.complex_song[index].letter, queue_id, index);
      index++;
      sleep(1);
    }
  }

  return;
}

void process_message(struct msgbuf* msg, struct warrior_state* st) {
  switch(msg->mtype) {
    case REQUEST_MSG: {
      if (st->my_letter == msg->letter || (st->complex_song[msg->position].status == commited) 
                                       || (st->complex_song[msg->position].status == accepted)) {
        st->complex_song[msg->position].status = rejected;
        send_message(msg->queue_id, REJECT_MSG, msg->letter, msg->position);
      }
      else {
        st->complex_song[msg->position].status = accepted;
        send_message(msg->queue_id, ACCEPT_MSG, msg->letter, msg->position);
      }
      break;
    }
    case ACCEPT_MSG: {
      if (st->complex_song[msg->position].status == requested || 
         (st->complex_song[msg->position].status == rejected) ) {
        st->complex_song[msg->position].replies++;
        if (st->complex_song[msg->position].replies == st->warriors_num-1) {
          st->complex_song[msg->position].replies = 0;
          st->complex_song[msg->position].status = commited;
          st->my_letter = msg->letter;
          st->my_positions[st->index_of_the_last_position++] = msg->position;
          send_message(msg->queue_id, COMMIT_MSG, msg->letter, msg->position);
        }
      }
      
      else {
        printf("AAAAAAAA we are dying\n Non-requested letter got message accepted\n");
      }
      break;
    }

    case REJECT_MSG: {
      if (st->complex_song[msg->position].status == requested || 
         (st->complex_song[msg->position].status == rejected)) {
        st->complex_song[msg->position].replies++;
        if (st->complex_song[msg->position].replies == st->warriors_num-1) {
          st->complex_song[msg->position].status = rejected;
          st->complex_song[msg->position].replies = 0;
        }
      }

       else {
        printf("AAAAAAAA we are dying\n Non-requested letter got message rejected\n");
      }
      break;
    }

    case COMMIT_MSG: {
      printf("Warrior %d: my letter %c on positions: %d\n", getpid(), st->my_letter, st->my_positions[0]);
      st->complex_song[msg->position].letter = msg->letter;
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
    queue_array[i] = create_queue(ftok("A", i), IPC_CREAT|0666);
    pid_t pid = fork();

    if (pid == -1) {
      fprintf(stderr, "fork: %s\n", strerror(errno));
      exit(1);
    }

    if (pid == 0) {
      warrior(queue_array[i], queue_array, song, N);
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

void broadcast(int* queue_array, size_t array_len, int msg_type, char letter, int queue_id, size_t position) {
  for (size_t i = 0; i < array_len; i++) {
    if (queue_array[i] != 0 && queue_array[i] != queue_id)
      send_message(queue_array[i], msg_type, letter, position);
  }
}

void send_message(int queue_id, int msg_type, char letter, size_t position)
{
  struct msgbuf buf = {.mtype = msg_type, .letter = letter, .queue_id = queue_id, .position = position};

  if (msgsnd(queue_id, &buf, sizeof(char), 0) == -1) {
    fprintf(stderr, "Failed to send message: %s\n", strerror(errno));
    exit(FAILURE_STATUS); 
  }

  return;
}

