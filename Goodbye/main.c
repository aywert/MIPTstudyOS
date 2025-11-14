#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

void goodbye_hdlr(int signal);

int main() {
  signal(SIGINT, goodbye_hdlr); //sets new handler
  printf("Hello, world!\n");
  while(1) sleep(1);
  
  return 0;
}

void goodbye_hdlr(int signal) {
  const char* buf = "Goodbye!\n";
  write(1, buf, strlen(buf));
  _exit(0);
}