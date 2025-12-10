#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <assert.h>
#include <stdbool.h>

struct monitor_t {
  pthread_mutex_t mtx;
  pthread_cond_t cond; //сигнализиурет о том, что начался новый день
  pthread_cond_t cond_replied;

  int time_of_the_day;
  int num_of_conibals;
  int* hunters_status;
  int num_of_replies;

  int food_pieces; // исмеряем количество еды в кусках
};

enum DAY_STATUS {
  IS_DAY  = 1, 
  IS_NIGHT = 2,
};

enum HUNTER_STATUS {
  HUNT_SUC  = 1, 
  HUNT_FAIL = 2,
  HUNT_NOEAT = 3,
  HUNT_EAT  = 4,
  HUNT_DEAD = 5,
};

struct argument {
  struct monitor_t* monitor;
  int index;
  pthread_t* pthreads;
};

void* canibals(void* arg);
void* cooker(void* arg);
int get_food(int hunter_status);
int init_canibals(int num_of_canibals, struct monitor_t* monitor, pthread_t* threads);

void monitor_init(struct monitor_t* monitor, int num_of_conibals);
void monitor_destroy(struct monitor_t* monitor);

int main (int argc, char* argv[]) {
  if (argc < 2) {
    perror("not enough arguments");
  }

  int N = atoi(argv[1]);

  struct monitor_t monitor;
  monitor_init(&monitor, N);

  pthread_t threads[N+1];
  //запускаем конибалов (охотников и повора)
  init_canibals(N, &monitor, threads);
 
  //ждем когда все потоки выйдут
  for (int i = 0; i < N+1; i++) {
    pthread_join(threads[i], NULL);
  }
  printf("hello\n");
  monitor_destroy(&monitor);

  return 0;
}

void* canibals(void* arg) {
  printf("canibals\n");
  struct argument* st = (struct argument*) arg;
  struct monitor_t* monitor = st->monitor;
  int index_of_canibal = st->index;

  while(1) {
    // Ждем когда будет день
    pthread_mutex_lock(&(monitor->mtx)); 
    while (monitor->time_of_the_day != IS_DAY) {
      pthread_cond_wait(&(monitor->cond), &(monitor->mtx));
    }
    pthread_mutex_unlock(&(monitor->mtx));

    if (monitor->hunters_status[st->index]) {
      pthread_exit(0);
    }

    pthread_mutex_lock(&(monitor->mtx)); 
    if (get_food(monitor->hunters_status[index_of_canibal])) {
      monitor->hunters_status[index_of_canibal] = HUNT_SUC;
      printf("Conibal %d: found meat\n", index_of_canibal);
      monitor->food_pieces++; 
    }

    else {
      monitor->hunters_status[index_of_canibal] = HUNT_FAIL;
      printf("Conibal %d: didn`t find meat\n", index_of_canibal);
    }
    pthread_mutex_unlock(&(monitor->mtx)); 

  }

  printf("i am conibal\n");
  return NULL;
}


void* cooker(void* arg) {
  //printf("cooker\n");
  struct argument* st = (struct argument*) arg;
  pthread_t* threads = st->pthreads;
  struct monitor_t* monitor = st->monitor;
  size_t index_of_the_day = 0;

  while (monitor->num_of_conibals != 0) {
    printf("Day Number %zu begins\n", index_of_the_day++);

    pthread_mutex_lock(&(monitor->mtx));
    pthread_cond_broadcast(&(monitor->cond)); //наступил день
    monitor->time_of_the_day = IS_NIGHT;
    pthread_mutex_unlock(&(monitor->mtx));


    // ждем пока все охотники вернуться с охоты
    pthread_mutex_lock(&(monitor->mtx));
    while (monitor->num_of_replies != monitor->num_of_conibals) {
      pthread_cond_wait(&(monitor->cond), &(monitor->mtx)); 
    }
    pthread_mutex_unlock(&(monitor->mtx));

    //повар подходит к котлу и начинает распределять еду
    if (monitor->food_pieces == 0) {
      printf("We are dead: 0 pieces of food\n");
    }
    
    else {

      int fed_canibals = 0;
      printf("Cooker: took 1 byte\n");
      monitor->food_pieces--;

      //Сначала еду получают те, кто был успешен на охоте
      for (int i = 0; i < monitor->num_of_conibals; i++) {
        if (monitor->hunters_status[i] == HUNT_SUC) {
          printf("Canibal %d: took 1 byte\n", i);
          monitor->hunters_status[i] = HUNT_EAT; //отмечаем его как поевшего
          monitor->food_pieces--;
          fed_canibals++;
        }
      }

      if (monitor->food_pieces > 0) {
        for (int i = 0; i < monitor->num_of_conibals; i++) {
          if (monitor->hunters_status[i] == HUNT_FAIL && monitor->food_pieces != 0) {
            printf("Canibal %d: took 1 byte\n", i);
            monitor->hunters_status[i] = HUNT_EAT; //отмечаем его как поевшего
            monitor->food_pieces--;
            fed_canibals++;
          }
        }
      }

      if (fed_canibals != monitor->num_of_conibals) {
        for (int i = 0; i < monitor->num_of_conibals; i++) {
          if (monitor->hunters_status[i] == HUNT_FAIL) {
            pthread_cancel(threads[i]);
            monitor->hunters_status[i] = HUNT_DEAD;
            monitor->num_of_conibals--;
          }
        }
      }
    }
  }

  return NULL;
}


void monitor_init(struct monitor_t* monitor, int num_of_conibals) {
  assert(monitor);

  pthread_mutex_init(&(monitor->mtx), NULL);
  pthread_cond_init(&(monitor->cond), NULL);
  pthread_cond_init(&(monitor->cond_replied), NULL);

  monitor->num_of_conibals = num_of_conibals;
  monitor->num_of_replies = 0;
  monitor->time_of_the_day = IS_NIGHT;
  monitor->hunters_status = calloc(num_of_conibals, sizeof(int));

  for (int i = 0; i < num_of_conibals; i++) {
    monitor->hunters_status[i] = HUNT_FAIL;
  }

  printf("monitor is created\n");
}

void monitor_destroy(struct monitor_t* monitor) {
  assert(monitor);

  pthread_cond_destroy(&(monitor->cond));
  pthread_mutex_destroy(&(monitor->mtx));

  free(monitor->hunters_status);
  printf("monitor is destroyed\n");
}

int init_canibals(int num_of_canibals, struct monitor_t* monitor, pthread_t* threads) {
  for (int i = 0; i < num_of_canibals; i++) {
    struct argument st = {.index = i, .monitor = monitor, .pthreads = threads}; 
    if (pthread_create(&threads[i], NULL, canibals, &st) != 0) {
      perror("Failed to create thread");
      pthread_mutex_lock(&(monitor->mtx));
      pthread_cond_broadcast(&(monitor->cond));
      pthread_mutex_unlock(&(monitor->mtx));

      // удаляем уе созданные потоки
      for (int j = 0; j < i; j++) {
        pthread_join(threads[j], NULL);
      }

      monitor_destroy(monitor);
      return 0;
    }
  }
  struct argument st = {.index = 0, .monitor = monitor, .pthreads = threads};
  if (pthread_create(&threads[num_of_canibals], NULL, cooker, &st) != 0) {
    perror("Failed to create thread");
    return 0;
  }

  return 0;
} 

int get_food(int hunter_status) {
  if (hunter_status == HUNT_NOEAT) {
    return 0;
  }

  else {
    return rand()%2;
  }
}