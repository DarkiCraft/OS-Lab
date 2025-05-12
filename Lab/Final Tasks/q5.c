#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

int is_running = 1;

void sigint_handler(int sig) {
  (void)sig;
  is_running = 0;
  printf("Paused.\n");
}

void sigtstp_handler(int sig) {
  (void)sig;
  is_running = 1;
  printf("Resumed.\n");
}

int main() {
  if (signal(SIGINT, sigint_handler) != 0) {
    perror("signal");
    exit(EXIT_FAILURE);
  }

  if (signal(SIGTSTP, sigtstp_handler) != 0) {
    perror("signal");
    exit(EXIT_FAILURE);
  }

  printf("Countdown:\n");

  for (size_t i = 0; i < 10; i++) {
    while (!is_running) {
      pause();
    }

    printf("%2zu", 10 - i);
    sleep(1);
  }
}