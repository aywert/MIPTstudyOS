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
  pthread_cond_t cond_return; // сигнализирует о том, что охотники закончили охоту
  pthread_cond_t cond_sleep;

  int time_of_the_day;
  int num_of_conibals;
  int* hunters_status;
  int hunters_returned;
  int hunters_slept;

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
  srand(time(NULL));

  struct monitor_t monitor;
  monitor_init(&monitor, N);

  pthread_t threads[N+1];
  //запускаем конибалов (охотников и повора)
  init_canibals(N, &monitor, threads);
 

  printf("hello\n");
  monitor_destroy(&monitor);

  return 0;
}

void* canibals(void* arg) {
  assert(arg);
  struct argument* st = (struct argument*) arg;
  struct monitor_t* monitor = st->monitor;
  int index_of_canibal = st->index;

  while(1) {
    //printf("here %d\n", st->index);
    // Ждем начала день
    pthread_mutex_lock(&(monitor->mtx)); 
    while (monitor->time_of_the_day != IS_DAY) {
      //printf("waiting for the day\n");
      pthread_cond_wait(&(monitor->cond), &(monitor->mtx));
    }
    pthread_mutex_unlock(&(monitor->mtx));

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

    monitor->hunters_returned++;
    if (monitor->hunters_returned == monitor->num_of_conibals) {
      pthread_cond_signal(&(monitor->cond_return));
    }
    pthread_mutex_unlock(&(monitor->mtx)); 

    //теперь он ждет окончания дня
    pthread_mutex_lock(&(monitor->mtx)); 
    while (monitor->time_of_the_day != IS_NIGHT) {
      //printf("i am here waiting\n");
      pthread_cond_wait(&(monitor->cond), &(monitor->mtx));
    }

    if (monitor->hunters_status[st->index] == HUNT_DEAD) {
      //printf("Canibal %d is dead\n", st->index);
      pthread_mutex_unlock(&(monitor->mtx));
      pthread_exit(0);
    }
    
    monitor->hunters_slept++;
    // printf("Canibal %d\n", st->index);
    // printf("monitor->num_of_conibals = %d\n", monitor->num_of_conibals);
    // printf("monitor->hunters_slept = %d\n", monitor->hunters_slept);
    if (monitor->hunters_slept == monitor->num_of_conibals) {
      pthread_cond_signal(&(monitor->cond_sleep));
    }

    pthread_mutex_unlock(&(monitor->mtx));
  }

  printf("i am conibal\n");
  return NULL;
}


void* cooker(void* arg) {

  assert(arg);
  printf("cooker\n");
  struct argument* st = (struct argument*) arg;
  pthread_t* threads  = st->pthreads;
  struct monitor_t* monitor = st->monitor;
  int N = monitor->num_of_conibals;
  size_t index_of_the_day = 1;

  while (monitor->num_of_conibals != 0) {
    printf("-----Day Number %zu begins-----\n", index_of_the_day++);

    pthread_mutex_lock(&(monitor->mtx));
    monitor->time_of_the_day = IS_DAY;
    pthread_cond_broadcast(&(monitor->cond)); //наступил день
    pthread_mutex_unlock(&(monitor->mtx));

    // ждем пока все охотники вернуться с охоты
    pthread_mutex_lock(&(monitor->mtx));
    while (monitor->hunters_returned != monitor->num_of_conibals) {
      pthread_cond_wait(&(monitor->cond_return), &(monitor->mtx)); 
    }
    pthread_mutex_unlock(&(monitor->mtx));

    //повар подходит к котлу и начинает распределять еду
    printf("Distibuting food:\n");
    if (monitor->food_pieces == 0) {
      printf("Cooker: nothing to take :(\n");
    }
    else { 
      printf("Cooker: took 1 byte\n");
      monitor->food_pieces--; 
    }

    int fed_canibals = 0;

    //Сначала еду получают те, кто был успешен на охоте
    for (int i = 0; i < N; i++) {
      if (monitor->hunters_status[i] == HUNT_SUC) {
        if (monitor->food_pieces != 0) {
          printf("Canibal %d: took 1 byte\n", i);
          monitor->hunters_status[i] = HUNT_EAT; //отмечаем его как поевшего
          monitor->food_pieces--;
          fed_canibals++;
        }
        else {
          printf("Canibal %d: nothing to eat :(\n", i);
          monitor->hunters_status[i] = HUNT_NOEAT;;
        } 
      }
    }
    //теперь еду получают неудачливые охотники
    for (int i = 0; i < N; i++) {
      if (monitor->hunters_status[i] == HUNT_FAIL && monitor->food_pieces != 0) {
        printf("Canibal %d: took 1 byte\n", i);
        monitor->hunters_status[i] = HUNT_EAT; //отмечаем его как поевшего
        monitor->food_pieces--;
        fed_canibals++;
      }
    }
    //Все неудачливые канибалы, которым не достался кусок, зарезаны поваром
    if (fed_canibals != monitor->num_of_conibals) {
      for (int i = 0; i < N; i++) {
        if (monitor->hunters_status[i] == HUNT_FAIL) {
          printf("Canibal %d: got murdered\n", i);
          monitor->hunters_status[i] = HUNT_DEAD;
          monitor->num_of_conibals--;
        }
      }
    }

    fed_canibals = 0;
  

  //Наступила ночь
  printf("Night is set\n");
  pthread_mutex_lock(&(monitor->mtx));
    monitor->hunters_returned = 0;
    monitor->time_of_the_day = IS_NIGHT;
    pthread_cond_broadcast(&(monitor->cond));
  pthread_mutex_unlock(&(monitor->mtx));

  //Ждем сообщение от всех охотников, что они поспали
  printf("Waiting for the hunters to wake up\n");
  pthread_mutex_lock(&(monitor->mtx));
  while (monitor->hunters_slept < monitor->num_of_conibals) {
    pthread_cond_wait(&(monitor->cond_sleep), &(monitor->mtx)); 
  }

  monitor->hunters_slept = 0;
  pthread_mutex_unlock(&(monitor->mtx));
  printf("Everyone woke up. Should claim the beggining of the day\n");

  }

  return NULL;
}


void monitor_init(struct monitor_t* monitor, int num_of_conibals) {
  assert(monitor);

  pthread_mutex_init(&(monitor->mtx), NULL);
  pthread_cond_init(&(monitor->cond), NULL);
  pthread_cond_init(&(monitor->cond_return), NULL);
  pthread_cond_init(&(monitor->cond_sleep), NULL);

  monitor->num_of_conibals = num_of_conibals;
  monitor->hunters_returned = 0;
  monitor->hunters_slept = 0;
  monitor->food_pieces = 0;
  monitor->time_of_the_day = IS_NIGHT;
  monitor->hunters_status = calloc(num_of_conibals, sizeof(int));

  for (int i = 0; i < num_of_conibals; i++) {
    monitor->hunters_status[i] = HUNT_FAIL;
  }

  printf("monitor is created\n");
}

void monitor_destroy(struct monitor_t* monitor) {
  assert(monitor);

  pthread_cond_destroy(&(monitor->cond_return));
  pthread_cond_destroy(&(monitor->cond_sleep));
  pthread_cond_destroy(&(monitor->cond));
  pthread_mutex_destroy(&(monitor->mtx));

  free(monitor->hunters_status);
  printf("monitor is destroyed\n");
}

int init_canibals(int num_of_canibals, struct monitor_t* monitor, pthread_t* threads) {
  struct argument* st_array = (struct argument*)calloc(num_of_canibals+1, sizeof(struct argument));
  for (int i = 0; i < num_of_canibals; i++) {
    st_array[i].index = i; st_array[i].monitor = monitor; st_array[i].pthreads = threads; 
    if (pthread_create(&threads[i], NULL, canibals, &st_array[i]) != 0) {
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
    st_array[num_of_canibals].index    = num_of_canibals; 
    st_array[num_of_canibals].monitor  = monitor; 
    st_array[num_of_canibals].pthreads = threads; 
    if (pthread_create(&threads[num_of_canibals], NULL, cooker, &st_array[num_of_canibals]) != 0) {
    perror("Failed to create thread");
    return 0;
  }

  printf("Canibals are initiated\n");

  for (int i = 0; i < num_of_canibals+1; i++) {
    pthread_join(threads[i], NULL);
  }

  free(st_array);

  printf("Canibals left\n");
  return 0;
} 

int get_food(int hunter_status) {
  if (hunter_status == HUNT_NOEAT) {
    return 0;
  }

  else {
    int num = rand();
    //printf("get_food = %d; %d\n", num, num%2);
    return num%2;
  }
}