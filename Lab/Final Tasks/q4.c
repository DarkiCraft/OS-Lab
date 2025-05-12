#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

void sig_handler(int sig) {
  (void)sig;
  printf("SIGINT caught, terminating safely.\n");
  exit(EXIT_SUCCESS);
}

int main() {
  if (signal(SIGINT, sig_handler) != 0) {
    perror("signal");
    exit(EXIT_FAILURE);
  }

  while (1) {
    printf("Running...\n");
    sleep(2);
  }
}