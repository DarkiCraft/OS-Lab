#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // for getpid()

//--------------------------------------------------------------------------------
// int pthread_attr_init(pthread_attr_t* attr);
// int pthread_attr_destroy(pthread_attr_t* attr);
// Brief: Default initializes/destroys a pthread_attr_t object
//
// Parameters: pthread_attr_t - pointer to a pthread_attr_t object, which will be initialized/destroyed
//
// Returns: 0 on success; on-zero error-number on error, sets errno, and contents of *attr are undefined
//
// Errors:
// - ENOMEM - Insufficient memory to initialise the thread attributes object.
//
// Usage:
//   pthread_attr_t attr;
//   pthread_attr_init(&attr);
//   // modify specific attributes if desired
//   pthread_create(&ids[0], &attr, func_1, (void*)some_arg); // plus error handling
//   pthread_create(&ids[1], &attr, func_2, (void*)some_arg); // plus error handling
//   // destroy after use
//   pthread_attr_destroy(&attr);
//
// Notes:
// - Default initialization depends on implementation of compiler, library, and system
// - A single attr object can be used to create multiple threads
// - Calling pthread_attr_t on an already initialized attr is undefined
// - Using an uninitialized or destroyed attr is undefined
// - Destroyed attr can be reinitialized with pthread_attr_init()
//
// Search pthread_attr_init(3) | pthread_attr_destroy(3) for more information
//--------------------------------------------------------------------------------
// The following are all the get/set functions for each attribute
// Returns: 0 on success; non-zero error-number on error, errno is set
// (Check online for errors, too much to put here T_T)
//--------------------------------------------------------------------------------
// int pthread_attr_getdetachstate(const pthread_attr_t* attr, int* detachstate);
// int pthread_attr_setdetachstate(pthread_attr_t* attr, int detachstate);
//
// Controls whether threads are joinable or detached.
//
// detachstate:
// - PTHREAD_CREATE_JOINABLE (default): Must be joined with pthread_join()
// - PTHREAD_CREATE_DETACHED: Resources are automatically released on exit
//--------------------------------------------------------------------------------
// int pthread_attr_getguardsize(const pthread_attr_t* attr, size_t* size);
// int pthread_attr_setguardsize(pthread_attr_t* attr, size_t size);
//
// Controls the size of the guard area (non-accessible memory) at end of stack
// Helps detect stack overflows. Default size varies by system.
//--------------------------------------------------------------------------------
// int pthread_attr_getschedpolicy(const pthread_attr_t* attr, int* policy);
// int pthread_attr_setschedpolicy(pthread_attr_t* attr, int policy);
//
// policy options (POSIX):
// - SCHED_OTHER (default, implementation defined)
// - SCHED_FIFO: FIFO scheduling
// - SCHED_RR: Round-robin scheduling
//
// int pthread_attr_getschedparam(const pthread_attr_t* attr, struct sched_param* param);
// int pthread_attr_setschedparam(pthread_attr_t* attr, const struct sched_param* param);
//
// sched_param contains at least: int sched_priority
// Only applies if thread inherits custom scheduling
//--------------------------------------------------------------------------------
// int pthread_attr_getinheritsched(const pthread_attr_t* attr, int* inherit);
// int pthread_attr_setinheritsched(pthread_attr_t* attr, int inherit);
//
// inherit options:
// - PTHREAD_INHERIT_SCHED (default): Thread inherits parent’s policy
// - PTHREAD_EXPLICIT_SCHED: Use policy/params from attr
//--------------------------------------------------------------------------------
// int pthread_attr_getscope(const pthread_attr_t* attr, int* scope);
// int pthread_attr_setscope(pthread_attr_t* attr, int scope);
//
// scope options:
// - PTHREAD_SCOPE_SYSTEM: Competes with all threads on system (default)
// - PTHREAD_SCOPE_PROCESS: Competes with threads in same process only
// (Not all systems support both; check at runtime)
//--------------------------------------------------------------------------------
// int pthread_attr_getstacksize(const pthread_attr_t* attr, size_t* size);
// int pthread_attr_setstacksize(pthread_attr_t* attr, size_t size);
//
// Sets the size of the thread's stack. Must be >= PTHREAD_STACK_MIN.
//
// int pthread_attr_getstackaddr(const pthread_attr_t* attr, void** addr);
// int pthread_attr_setstackaddr(pthread_attr_t* attr, void* addr);
//
// Sets the *starting address* of the stack (not recommended — use with care)
// Often paired with stacksize. Deprecated in favor of pthread_attr_setstack()
//--------------------------------------------------------------------------------

void* thread_func(void* arg) {
  (void)arg;
  printf("Thread running (PID: %d)\n", getpid());
  return NULL;
}

int main() {
  pthread_t tid;
  pthread_attr_t attr;

  // Initialize thread attributes
  if (pthread_attr_init(&attr) != 0) {
    perror("pthread_attr_init");
    return EXIT_FAILURE;
  }

  // Set detach state to JOINABLE (default, just for demo)
  if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE) != 0) {
    perror("pthread_attr_setdetachstate");
    pthread_attr_destroy(&attr);
    return EXIT_FAILURE;
  }

  // Set custom stack size (e.g., 1 MB)
  if (pthread_attr_setstacksize(&attr, 1024 * 1024) != 0) {
    perror("pthread_attr_setstacksize");
    pthread_attr_destroy(&attr);
    return EXIT_FAILURE;
  }

  // Set scheduling policy (optional, OS must allow non-default)
  if (pthread_attr_setschedpolicy(&attr, SCHED_OTHER) != 0) {
    perror("pthread_attr_setschedpolicy (may be restricted)");
  }

  // Create the thread with custom attributes
  if (pthread_create(&tid, &attr, thread_func, NULL) != 0) {
    perror("pthread_create");
    pthread_attr_destroy(&attr);
    return EXIT_FAILURE;
  }

  // Join thread
  if (pthread_join(tid, NULL) != 0) {
    perror("pthread_join");
    pthread_attr_destroy(&attr);
    return EXIT_FAILURE;
  }
  printf("Thread joined successfully\n");

  // Clean up
  if (pthread_attr_destroy(&attr) != 0) {
    perror("pthread_attr_destroy");
  }
}