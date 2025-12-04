#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/msg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/sem.h>

enum message_type {
  TERMINATE_MSG = 1, 
  REQUEST_MSG = 2,
  ACCEPT_MSG = 3,
  REJECT_MSG = 4,
  COMMIT_MSG = 5,
  PHANTOM_MSG = 6,
  START_MSG  = 7,
  SING_MSG = 8,
  SUNG_MSG = 9,
};

enum letter_status {
  not_taken = 0,
  requested = 1,
  commited  = 2,
  accepted  = 3,
  rejected  = 4,
  sung      = 5,
};

struct msgbuf {
  long mtype;       /* message type, must be > 0 */
  int position;
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
  int warriror_status;
  size_t warriors_num;
  int song_length;
  char my_letter;
  int my_positions[64];
  size_t index_of_the_last_position;
  size_t active_warriors;
  int* queue_array;
  int queue_id;
  int semid;
  pid_t my_pid;
  bool got_permission;
  bool running;
  bool singing_song;
};

struct sembuf P[] = {{0, -1, SEM_UNDO}};
struct sembuf V[] = {{0,  1, SEM_UNDO}};

struct sembuf P_song[] = {{1, -1, SEM_UNDO}};
struct sembuf V_song[] = {{1,  1, SEM_UNDO}};

#define FAILURE_STATUS -1
#define SUCCESS_STATUS 1

void process_message(struct msgbuf* msg, struct warrior_state* st);
void warrior(int queue_id, int* queue_array, char* song, int semid, size_t warriors_num);
int create_queue(key_t key, int msgflg);
void init_warriors(int N, int* queue_array, int semid, char* song);
int count_letters(char* line);
int random_in_range(int min, int max);

bool is_letter_busy(struct warrior_state* st, char letter, size_t position);
void send_message(int queue_id_to, int queue_id_from, int msg_type, char letter, int position);
void broadcast(int* queue_array, size_t array_len, int msg_type, char letter, int queue_id, int semid, size_t position);

int create_semaphore(const char* name, int flags);
void delete_semaphore(int semid);
void RunOp_safe(int semid, struct sembuf *op, size_t nop);

int main(int argc, char* argv[]) {

  if (argc < 2) {
    perror("invalid input");
    return 0;
  }

  int letters = count_letters(argv[1]);
  printf("letter count = %d\n", letters);
  int* queue_array = (int*)calloc(letters, sizeof(int));
  int semid = create_semaphore("main.c", IPC_CREAT);
  memset(queue_array, -1, letters*sizeof(int));
  init_warriors(letters, queue_array, semid, argv[1]);

  printf("warrior_inited\n");
  sleep(1);
  broadcast(queue_array, letters, START_MSG, '\0', 0, semid, 0);

  for (int j = 0; j < letters; j++)
    wait(NULL);

  for (int i = 0; i < letters; i++) {
    if (msgctl(queue_array[i], IPC_RMID, NULL) < 0) {
      perror("mistake in closing messages queuess");
    }
  }

  delete_semaphore(semid);

  free(queue_array);
  return 0;
}

void warrior(int queue_id, int* queue_array, char* song, int semid, size_t warriors_num) {
  
  struct warrior_state st;

  for (size_t i = 0; i < strlen(song); i++) {
    st.complex_song[i].position = i;
    st.complex_song[i].letter = song[i];
    st.complex_song[i].status = not_taken;
    st.complex_song[i].replies = 0;
  }
  st.index_of_the_last_position = 0;
  st.my_pid = getpid();
  st.my_letter = '\0';
  st.queue_array = queue_array;
  st.queue_id = queue_id;
  st.semid = semid;
  st.warriror_status = not_taken;
  st.got_permission = false;
  st.warriors_num   = warriors_num;
  st.song_length = strlen(song);
  st.active_warriors = warriors_num;
  st.running = true;
  st.singing_song = false;

  for (int i = 0; i < 64; i++)
    st.my_positions[i] = -1; 

  size_t index = 0;
  struct msgbuf msg = {};
  size_t num_of_commits = 0;
  
  while (st.running) {
    //printf("i am %d my status %d my letter %c\n", st.queue_id, st.warriror_status, st.my_letter);
    int ret = msgrcv(queue_id, &msg, sizeof(struct msgbuf) - sizeof(long), 0, IPC_NOWAIT);

    if (ret != -1) {
      if (msg.mtype == COMMIT_MSG) num_of_commits++;
      process_message(&msg, &st);
    }
    else if (errno != ENOMSG) {
      perror("msgrcv");
      st.running = 0;
    }
    else if (st.my_letter == '\0' && st.warriror_status != requested && st.got_permission) {
      printf("i am %d my status %d my letter %c\n", st.queue_id, st.warriror_status, st.my_letter);
      while (st.complex_song[index].status != not_taken && index < warriors_num) {
        index++; 
      }

      st.warriror_status = requested;
      st.complex_song[index].status = requested;
      broadcast(queue_array, warriors_num, REQUEST_MSG, st.complex_song[index].letter, queue_id, semid, index);
      
      if (index == warriors_num-1) 
        index = 0;
      else index++; 
      sleep(1);
    }

    
    // else if (st.active_warriors == 0 && st.singing_song == false) {
    //   // broadcast(st.queue_array, st.warriors_num, TERMINATE_MSG, '\0', st.queue_id, semid, -1);
    //   // st.running = false;
    //   //printf("Warriors are now going to sing a song for you. BEHOLD: \n");
    //   RunOp_safe(semid, P_song, 1);
    //   printf("i am a single proc\n");
    //   st.singing_song = true;
      
    //   RunOp_safe(semid, V_song, 1);
    // }

    if (st.active_warriors == 0 && !st.singing_song) {
      if (st.my_positions[0] == 0) {
        broadcast(st.queue_array, st.warriors_num, TERMINATE_MSG, '\0', st.queue_id, st.semid, -1);
        send_message(st.queue_id, st.queue_id, TERMINATE_MSG, '\0', -1);
        // st.singing_song = true;
        // putchar(st.my_letter); fflush(stdout);
        // st.complex_song[0].status = sung;
        // broadcast(st.queue_array, st.warriors_num, SUNG_MSG, st.complex_song[0].letter, st.queue_id, st.semid, 0);
        // broadcast(st.queue_array, st.warriors_num, SING_MSG, st.complex_song[1].letter, st.queue_id, semid, 1);
        // send_message(st.queue_id, st.queue_id, SING_MSG, st.complex_song[msg.position + 1].letter, msg.position + 1);
      }   
    }
  }

  printf("i am process %d exited\n my letter %c\n", getpid(), st.my_letter);

  return;
}

void process_message(struct msgbuf* msg, struct warrior_state* st) {
  switch(msg->mtype) {
    case REQUEST_MSG: {
      if (st->complex_song[msg->position].status == commited || is_letter_busy(st, msg->letter, msg->position)) {
        if (st->complex_song[msg->position].status != requested) {
          st->complex_song[msg->position].status = rejected;
        }
        send_message(msg->queue_id, st->queue_id, REJECT_MSG, msg->letter, msg->position);
      }
        
      else if (st->my_letter == msg->letter) {
         //(st->my_positions[0]!=-1? (st->complex_song[st->my_positions[0]].status != requested) : false)*/ {
        if (st->complex_song[msg->position].status != requested) {
          st->complex_song[msg->position].status = rejected;
        }
        send_message(msg->queue_id, st->queue_id, REJECT_MSG, msg->letter, msg->position);
      }
      else if (st->complex_song[msg->position].status == requested) {
        if (msg->queue_id < st->queue_id) {
          send_message(msg->queue_id, st->queue_id, ACCEPT_MSG, msg->letter, msg->position);
        }

        else {
          send_message(msg->queue_id, st->queue_id, REJECT_MSG, msg->letter, msg->position);
        }
      }
      else {
        if (st->complex_song[msg->position].status != requested) {
          st->complex_song[msg->position].status = accepted;
        }
        send_message(msg->queue_id, st->queue_id, ACCEPT_MSG, msg->letter, msg->position);
      }
      break;
    }
    case ACCEPT_MSG: {
      //printf("ACCEPT: status = %d\n", st->complex_song[msg->position].status);
      if (st->complex_song[msg->position].status == requested || 
          st->complex_song[msg->position].status == rejected ||
          st->complex_song[msg->position].status == accepted) {
        st->complex_song[msg->position].replies++;
        if (st->complex_song[msg->position].replies == st->warriors_num-1) {
          if (st->complex_song[msg->position].status == rejected) {
            st->warriror_status = rejected;
            break;
          }  
          st->complex_song[msg->position].replies = 0;
          st->complex_song[msg->position].status = commited;
          st->active_warriors--;
          st->my_letter = msg->letter;
          st->warriror_status = accepted;
          st->my_positions[st->index_of_the_last_position++] = msg->position;
          printf("Warrior %d: my letter %c on positions: %d\n", st->queue_id, st->my_letter, st->my_positions[0]);
          printf("Active warriors %zu\n", st->active_warriors);
          // if (st->active_warriors == 0) {
          //   broadcast(st->queue_array, st->warriors_num, TERMINATE_MSG, '\0', st->queue_id, -1);
          //   send_message(st->queue_id, st->queue_id, TERMINATE_MSG, '\0', -1);
          // }
          // else {
          broadcast(st->queue_array, st->warriors_num, COMMIT_MSG, msg->letter, st->queue_id, st->semid,  msg->position);
          // }
          
        }
      }

      else if (st->complex_song[msg->position].status == commited) { 
        st->complex_song[msg->position].replies = 0;
        st->warriror_status = rejected;
        printf("WENT WRONG Msg letter = %c but my_letter = %c position = %d\n", msg->letter, st->my_letter, msg->position);
      }
      
      else {
        printf("AAAAAAAA we are dying\n Non-requested letter got message accepted\n");
      }
      break;
    }

    case REJECT_MSG: {
      //printf("REJECT: status = %d\n", st->complex_song[msg->position].status);
      if (st->complex_song[msg->position].status == requested || 
          st->complex_song[msg->position].status == rejected || 
          st->complex_song[msg->position].status == accepted) {
        st->complex_song[msg->position].status = rejected;
        st->complex_song[msg->position].replies++;
        if (st->complex_song[msg->position].replies == st->warriors_num-1) {
          st->warriror_status = rejected;
          st->complex_song[msg->position].replies = 0;
        }
      }

      else if (st->complex_song[msg->position].status == commited) { 
        //ignoring forgetted requests
        st->complex_song[msg->position].replies = 0;
        st->warriror_status = rejected;
        printf("WENT WRONG Msg letter = %c but my_letter = %c position = %d\n", msg->letter, st->my_letter, msg->position);
      }

       else {
        printf("Warrior %d: AAAAAAAA we are dying\n Non-requested letter got message rejected\n", st->queue_id);
      }
      break;
    }

    case COMMIT_MSG: {
      if (st->my_letter == msg->letter) {
        break;
      }
      
      if (st->complex_song[msg->position].status == commited) {
        send_message(msg->queue_id, st->queue_id, PHANTOM_MSG, msg->letter, msg->position);
      }

      st->complex_song[msg->position].letter = msg->letter;
      st->complex_song[msg->position].status = commited;
      st->active_warriors--;
      break;
    }

    case SING_MSG: {
      if (st->singing_song == false)
        st->singing_song = true;

      if (msg->position == st->song_length && st->complex_song[msg->position-1].status == sung) {
        broadcast(st->queue_array, st->warriors_num, TERMINATE_MSG, '\0', st->queue_id, st->semid, -1);
        send_message(st->queue_id, st->queue_id, TERMINATE_MSG, '\0', -1);
      }

      else if (msg->position == st->my_positions[0] || msg->letter == st->my_letter) {
        st->complex_song[msg->position].status = sung;
        putchar(st->my_letter); fflush(stdout);

        broadcast(st->queue_array, st->warriors_num, SUNG_MSG, st->complex_song[msg->position].letter, st->queue_id, st->semid, msg->position);
        broadcast(st->queue_array, st->warriors_num, SING_MSG, st->complex_song[msg->position + 1].letter, st->queue_id, st->semid, msg->position + 1);
        send_message(st->queue_id, st->queue_id, SING_MSG, st->complex_song[msg->position + 1].letter, msg->position + 1);
      }
       
      break;
    }

    case PHANTOM_MSG: {
      st->complex_song[msg->position].replies++;
      if (st->complex_song[msg->position].replies == st->warriors_num-1) { 
        st->complex_song[msg->position].replies = 0;
        st->active_warriors++;
        st->my_letter = '\0';
        st->my_positions[0] = -1;
        st->index_of_the_last_position--; 
        st->warriror_status = rejected;
      }

      break;
    }

    case SUNG_MSG: {
      st->complex_song[msg->position].status = sung;
      break;
    }

    case START_MSG: {
      st->got_permission = true;
      break;
    }

    case TERMINATE_MSG: {
      st->running = false;
      st->got_permission = false;
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

void init_warriors(int N, int* queue_array, int semid, char* song) {
  for (int i = 0; i < N; i++) {
    queue_array[i] = create_queue(ftok("main.c", i), IPC_CREAT|0666);
  }

  for (int i = 0; i < N; i++) {
    pid_t pid = fork();

    if (pid == -1) {
      fprintf(stderr, "fork: %s\n", strerror(errno));
      exit(1);
    }

    if (pid == 0) {
      warrior(queue_array[i], queue_array, song, semid, N);
      exit(0);
      break;
    }
  } 

  sleep(1);
  
  return;
}

int count_letters(char* line) {
  if (line == NULL || *line == '\0') {
    return 0;
  }
  
  int count = 0;
  int length = strlen(line);
  
  for (int i = 0; i < length; i++) {
    int found = 0;
    for (int j = 0; j < i; j++) {
      if (line[i] == line[j]) {
        found = 1;
        break;
      }
    }
    if (!found) {
      count++;
    }
  }
  
  return count;
}

void broadcast(int* queue_array, size_t array_len, int msg_type, char letter, int queue_id, int semid, size_t position) {
  RunOp_safe(semid, P, 1);
  for (size_t i = 0; i < array_len; i++) {
    if (queue_array[i] != -1 && queue_array[i] != queue_id)
      send_message(queue_array[i], queue_id, msg_type, letter, position);
  }
  RunOp_safe(semid, V, 1);
}

void send_message(int queue_id_to, int queue_id_from, int msg_type, char letter, int position) {
  struct msgbuf buf = {.mtype = msg_type, .letter = letter, .queue_id = queue_id_from, .position = position};
  // printf("Message:\nmtype = %d\n char = %c\n position = %d\n queue_id_from = %d\n queue_id_to = %d\n---------\n", 
  //         msg_type, letter, position, queue_id_from,  queue_id_to);
  // fflush(stdout);
  if (msgsnd(queue_id_to, &buf, sizeof(struct msgbuf) - sizeof(long), 0) == -1) {
    fprintf(stderr, "Failed to send message: %s\n", strerror(errno));
    exit(FAILURE_STATUS); 
  }

  return;
}

bool is_letter_busy(struct warrior_state* st, char letter, size_t position) {
  bool is_busy = false;
  for (size_t i = 0; i < st->warriors_num; i++) {
    if (i != position && st->complex_song[i].letter == letter && (st->complex_song[i].status == commited ||
                                                 st->complex_song[i].status == requested )) {
      is_busy = true;
    }
  }

  return is_busy;
}

void delete_semaphore(int semid) {
  if (semctl(semid, IPC_RMID, 0) < 0) {
    perror("unable to delete semaphore\n");
    exit(EXIT_FAILURE);
  }

  printf("Semaphore is deleted\n");
}

int create_semaphore(const char* name, int flags) {
  //|able_to_broadcast|song|
  int semid = semget(ftok(name, 1), 2, flags | 0644);
  if (semid == -1) {
    perror("unable to create semphore\n");
    exit(EXIT_FAILURE);
  }

  semctl(semid, 0, SETVAL, 1);
  semctl(semid, 1, SETVAL, 1); 
  
  printf("Semaphore is created\n");

  return semid;
}


void RunOp_safe(int semid, struct sembuf *op, size_t nop) {
  if (semop(semid, op, nop) < 0) {
    perror("semop RunOp error");
    exit(EXIT_FAILURE);
  }
}

int random_in_range(int min, int max) {
    return min + rand() % (max - min + 1);
}