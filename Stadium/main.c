#include "runners_jundge.h"
#include <sys/types.h>


static size_t N = 126;
const char* name = "/test_queue";

int main(void) {
  
  struct mq_attr attr = {
    .mq_flags = 0,
    .mq_maxmsg = MSG_MAX,      // макс. сообщений в очереди
    .mq_msgsize = MSG_SIZE,   // макс. размер одного сообщения
    .mq_curmsgs = 0
  };

  mqd_t* queue_array = queues_for_runners(O_CREAT|O_RDWR, &attr); 
  //| comunication with judge| comunication with runners |ready status|
  
  init_runners(queue_array, N);
  judge(queue_array, N);
  queues_for_runners_delete(queue_array);
  //init_runners(queue_id, N);  //starts N processes
  //judge(queue_id, N);         //gives start to the first runner
  //msgctl(queue_id, IPC_RMID, &My_st);

  return 0;
}