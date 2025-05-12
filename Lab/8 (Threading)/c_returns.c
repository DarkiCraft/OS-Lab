#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// This demonstrates how to safely return values from threads using heap-allocated memory.
// The thread function performs computations and returns a malloc-ed result.
// The calling function is responsible for freeing the returned result, while the thread function must free any "consumed" argument data
// Avoid returning addresses of local variables — always

typedef struct {
  size_t n;
  int* arr;
} fn_arg;  // represents an array

typedef struct {
  int sum;
} fn_ret;

// remember to pay attention to "consuming" and "sharing" patterns as said in b_arguments.c

// function to sum a passed array and return it
void* array_sum(void* arg) {
  if (arg == NULL) {
    return NULL;  // passed arg is NULL
  }

  fn_arg* argument = (fn_arg*)arg;  // cast to desired object type

  // DO NOT return an address of a local variable, that results in undefined behaviour and unintended outputs
  // instead we will use malloc, and free later

  // allocate the returning object, this will be free-ed in the calling function
  fn_ret* ret_val = (fn_ret*)malloc(sizeof(fn_ret));
  // essentially, the calling function will "consume" the returned object (see b_arguments.c)
  if (ret_val == NULL) {
    perror("malloc");
    return NULL;
  }
  ret_val->sum = 0;

  for (size_t i = 0; i < argument->n; i++) {
    ret_val->sum += argument->arr[i];
  }

  free(arg);  // since the function "consumes" the argument, remember to free it in here

  return (void*)ret_val;
}

int main() {
  pthread_t id;

  // allocate memory for function argment
  fn_arg* arg = (fn_arg*)malloc(sizeof(fn_arg));
  if (arg == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  (void)printf("Enter number of elements in array: ");
  if (scanf("%zu", &arg->n) != 1) {  // %zu specifier is for size_t data-type
    fprintf(stderr, "Invalid input.\n");
    free(arg);  // cleanup on failure
    exit(EXIT_FAILURE);
  }

  if ((arg->arr = (int*)malloc(arg->n * sizeof(int))) == NULL) {
    perror("malloc");
    free(arg);  // cleanup on failure
    exit(EXIT_FAILURE);
  }

  (void)printf("Enter %zu elements: ", arg->n);
  for (size_t i = 0; i < arg->n; i++) {
    if (scanf("%d", &arg->arr[i]) != 1) {
      fprintf(stderr, "Invalid input.\n");
      free(arg->arr);  // cleanup on failure
      free(arg);
      exit(EXIT_FAILURE);
    }
  }

  // create function thread
  if (pthread_create(&id, NULL, array_sum, (void*)arg) != 0) {
    perror("pthread_create");
    free(arg->arr);  // cleanup on failure
    free(arg);
    exit(EXIT_FAILURE);
  }

  fn_ret* ret_val;  // make sure it is a pointer object
  if (pthread_join(id, (void**)&ret_val) != 0) {
    perror("pthread_join");
    exit(EXIT_FAILURE);  // no cleanup needed here, we cannot be certain if the thread function cleaned or not
                         // consider using some global flag
  }

  if (ret_val == NULL) {
    (void)printf("Thread function returned NULL\n");
  } else {
    printf("Sum: %d\n", ret_val->sum);
  }

  free(ret_val);  // remember to free returned value as calling thread "consumed" and is responsible
}