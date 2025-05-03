#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // for pause()

// Basics:
// - When a signal is raised, it is sent to ALL threads
// - A thread may block, ignore, or call a custom signal handler
// - Signals may be sent via kill(), pthread_kill() in the program or via the terminal

// int signal(int sig, void (*sighandler)(int));
// Brief: Sets a callable function when a signal is detected by a thread
//
// Paramters: sig        - signal to "assign" (use defined macros i.e., SIGINT, SIGTERM, etc.)
//            sighandler - function to "assign" signal to
//                         -> function must be of the form: void sighandler(int)
//
// Returns: previous value of the sighandler (SIG_DFL, SIG_IGN, etc.) on success, SIG_ERR on error, and errno is set
//
// Errors:
// - EINVAL - sig is invalid
//
// Usage:
//   void some_signal_handler(int sig); // defined globally or in some other file
//   if (signal(SIGINT, some_signal_handler) != SIG_ERR) {
//      perror("signal");
//      // handle error accordingly
//   }
//
// A SIGINT signal (Ctrl + C) will now cause the specified some_signal_handler() function to execute
//
// Notes:
// - SIGTERM and SIGSTOP cannot be assigned new functions
// - Implementation of signal() is extremely diverse on different systems
// - Always prefer to use sigaction() (see b_sigaction.c)
// - SIG_IGN may be specified in sighandler which will ignore the signal in that thread
// - SIG_DFL may be specified in sighandler to retore default behavior (only portable part of signal())
// - If some other sighandler is specified (some custom function), then it flows as follows:
//   -> signal disposition is reset to SIG_DFL (or it might not be)
//   -> signal is blocked temporarily
//   -> signal is passed to specified custom sighandler
//   -> signal is unblocked after sighandler has executed
// - Please note that the above flow is highly variable between systems and might not be true for all
// - Using signal() in multithreaded programs is undefined
//
// Search signal(2) for more information

void signal_handler(int sig) {
  (void)sig;
  printf("SIGINT caught. Exiting...\n");
  exit(EXIT_SUCCESS);
}

int main() {
  if (signal(SIGINT, signal_handler) == SIG_ERR) {
    perror("signal");
    exit(EXIT_FAILURE);
  }

  printf("Press Ctrl + C to exit\n");

  // simulate infinite loop
  while (1) {
    pause();  // pause() suspends thread until a signal arrives, saving resources
              // always returns -1, and always sets errno to EINTR
  }
}
