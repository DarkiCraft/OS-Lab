#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  int a;
  int b;
  int c;
} fn_arg;

// Thread argument handling: dynamically allocated memory is passed to threads.
// DO NOT pass an address of a local variable, that results in undefined behaviour and unintended outputs
// If the thread "consumes" the argument, it is responsible for freeing it.
// If the thread only "shares" the argument, the main thread retains ownership and must free it after join.

// Routine "consumes" the argument: it owns and frees it
void* consumes_arg(void* arg) {
  if (arg == NULL) {
    return NULL;
  }

  fn_arg* argument = (fn_arg*)arg;
  (void)printf("%d %d %d\n", argument->a, argument->b, argument->c);
  free(argument);  // responsibility lies here

  return NULL;
}

// Routine "shares" the argument: the caller is still responsible for freeing it
void* shares_arg(void* arg) {
  if (arg == NULL) {
    return NULL;
  }

  fn_arg* argument = (fn_arg*)arg;
  argument->a      = 1;
  argument->b      = 2;
  argument->c      = 3;

  return NULL;
}

int main() {
  pthread_t id;

  // Example of thread consuming the argument
  fn_arg* owned_arg = malloc(sizeof(fn_arg));
  if (owned_arg == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }
  owned_arg->a = 10;
  owned_arg->b = 20;
  owned_arg->c = 30;

  if (pthread_create(&id, NULL, consumes_arg, owned_arg) != 0) {
    perror("pthread_create");
    free(owned_arg);  // cleanup on failure
    exit(EXIT_FAILURE);
  }
  if (pthread_join(id, NULL) != 0) {
    perror("pthread_join");
    exit(EXIT_FAILURE);
  }

  // Example of thread sharing the argument
  fn_arg* shared_arg = malloc(sizeof(fn_arg));
  if (shared_arg == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);  // no cleanup needed here, we cannot be certain if the thread function cleaned or not
                         // consider using some global flag
  }

  if (pthread_create(&id, NULL, shares_arg, shared_arg) != 0) {
    perror("pthread_create");
    free(shared_arg);  // cleanup on failure
    exit(EXIT_FAILURE);
  }
  if (pthread_join(id, NULL) != 0) {
    perror("pthread_join");
    free(shared_arg);  // remember to clean because it is the caller's responsibility
    exit(EXIT_FAILURE);
  }
  (void)printf("%d %d %d\n", shared_arg->a, shared_arg->b, shared_arg->c);  // access after join
  free(shared_arg);                                                         // caller retains responsibility to clean memory
}