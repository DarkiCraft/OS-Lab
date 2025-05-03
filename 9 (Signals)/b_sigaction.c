#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
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

void handle_sigchld(int sig) {
  (void)sig;
  int status;
  pid_t pid = wait(&status);  // Reap child
  printf("Parent: Received SIGCHLD — child %d exited\n", pid);
}

void handle_sigint(int sig) {
  (void)sig;
  printf("Parent: Caught SIGINT, exiting...\n");
  exit(0);
}

int main() {
  // Set SIGCHLD handler
  struct sigaction sa_chld;
  sa_chld.sa_handler = handle_sigchld;
  sigemptyset(&sa_chld.sa_mask);
  sa_chld.sa_flags = 0;
  if (sigaction(SIGCHLD, &sa_chld, NULL) == -1) {
    perror("sigaction(SIGCHLD)");
    exit(1);
  }

  // Set SIGINT handler
  signal(SIGINT, handle_sigint);  // using signal() for simplicity

  pid_t pid = fork();
  if (pid == 0) {
    // Child process
    printf("Child: Sleeping for 3 seconds\n");
    sleep(3);
    printf("Child: Exiting\n");
    exit(0);
  }

  // Parent process
  printf("Parent: Waiting for child to exit (SIGCHLD). Press Ctrl + C to exit\n");
  while (1) {
    pause();  // wait for signals
  }
}