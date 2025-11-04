#include <stdio.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct Shower_enjoers
{
  size_t N;
  size_t M;
  size_t W;
};

enum visitor_status {
  SUCCESS = 1,
  FAILURE = 0,
};

enum VisitorGender {
  NotSet = 0,
  Man = 3,
  Woman = 4,
};

int create_semaphore(const char* name, int flags, struct Shower_enjoers* st);
void delete_semaphore(int semid);
void init_processes(struct Shower_enjoers* st, int semid);
void shower_visitor(int gender, int semid, int index);