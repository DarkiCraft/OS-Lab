#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>  // for pause()

// int sigaction

// int sigaction (int sig, const struct sigaction* new_act, struct sigaction* old_act);
// Brief: Sets a callable function when a signal is detected by a thread (preferred over signal())
//
// Paramters: sig     - signal to "assign" (use defined macros i.e., SIGINT, SIGTERM, etc.)
//            new_act - new action to assign to signal
//                      -> Pass NULL to not assign anything, useful for querying current action via old_act
//            old_act - old action, if needed
//                      -> Pass NULL if old_act is not needed
//
// Returns:  0 on success; -1 on error, and errno is set
//
// Errors:
// - EFAULT - passed new_act and old_act correspond to invalid memory
// - EINVAL - invalid signal is passed | attempted to change action of SIGTERM and SIGSTOP
//
// Usage:
//   void some_signal_handler(int sig); // defined globally or in some other file
//   struct sigaction sa;
//   memset(&sa, 0, sizeof(sa));               // initialize to rmpty
//   sa.sa_handler = some_signal_handler;      // set sighandler
//   sigemptyset(&sa.sa_mask);                 // mask all signals
//   sa.sa_flags = 0;                          // default flags
//   if (sigaction(SIGINT, &sa, NULL) == -1) { // add sa.sa_handler for SIGINT
//     perror("sigaction");
//     // handle error accordingly
//   }
//
// A SIGINT signal (Ctrl + C) will now cause the specified some_signal_handler() function to execute
//
// Flags: (not required in syllabus, only for extra info)
//
// Notes:
// - actions for SIGKILL and SIGSTOP cannot be changed
// - sa_handler and sa_sigaction (members of struct sigaction) might be a union
//   -> DO NOT assign BOTH sa_handler and sa_sigaction. ONLY assign ONE
// - A child created via fork() will inherit parent's signal dispositions
//
// Search sigaction(2) for more information

// SIGUSR1 handler
void usr1_handler(int sig) {
  (void)sig;
  printf("Parent: Received SIGUSR1 from child\n");
}

// SIGINT handler
void sigint_handler(int sig) {
  (void)sig;
  printf("Parent: Caught SIGINT (Ctrl + C). Exiting...\n");
  exit(EXIT_SUCCESS);
}

int main() {
  pid_t pid;

  // Set up SIGUSR1 handler
  struct sigaction sa_usr1;
  memset(&sa_usr1, 0, sizeof(sa_usr1));
  sa_usr1.sa_handler = usr1_handler;
  sigemptyset(&sa_usr1.sa_mask);
  sa_usr1.sa_flags = 0;
  if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1) {
    perror("sigaction - SIGUSR1");
    exit(EXIT_FAILURE);
  }

  // Set up SIGINT handler
  struct sigaction sa_int;
  memset(&sa_int, 0, sizeof(sa_int));
  sa_int.sa_handler = sigint_handler;
  sigemptyset(&sa_int.sa_mask);
  sa_int.sa_flags = 0;
  if (sigaction(SIGINT, &sa_int, NULL) == -1) {
    perror("sigaction - SIGINT");
    exit(EXIT_FAILURE);
  }

  pid = fork();
  if (pid == -1) {
    perror("fork");
    exit(EXIT_FAILURE);
  }

  if (pid == 0) {
    // Child process
    printf("Child: Sleeping for 2 seconds...\n");
    sleep(2);
    printf("Child: Sending SIGUSR1 to parent (PID %d)\n", getppid());
    kill(getppid(), SIGUSR1);  // kill() is used to send signals (stupid naming, I know)
    printf("Child: Exiting\n");
    exit(EXIT_SUCCESS);
  } else {
    // Parent process
    printf("Parent: Waiting for signal from child (PID %d)...\n", pid);
    wait(NULL);  // Wait for child to finish
    printf("Parent: Child exited. Now waiting for SIGINT (Ctrl + C)...\n");
    while (1) pause();  // Wait for SIGINT to exit
  }
}