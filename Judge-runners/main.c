#include "runners_jundge.h"
#include <sys/types.h>


static int N = 5;

int main(void) {
  int queue_id = create_queue(IPC_CREAT|0666);
  struct msqid_ds My_st = {};

  init_runners(queue_id, N);  //starts N processes
  judge(queue_id, N);         //gives start to the first runner
  msgctl(queue_id, IPC_RMID, &My_st);

  return 0;
}