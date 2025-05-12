#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//--------------------------------------------------------------------------------
// Thread Variable Scopes in C (with pthreads)
//
// 1. Global Variables:
//    - Shared by all threads
//    - Must use synchronization (e.g., mutexes) to avoid race conditions
//
// 2. Local Variables (inside a function):
//    - Each thread gets its own copy when it runs the function
//    - No interference between threads
//
// 3. Static Local Variables:
//    - Shared among all threads calling the function
//    - Acts like a global for that function — requires synchronization
//
// 4. Thread-Local Storage (TLS) — using `__thread` (GCC/Clang):
//    - Syntax: `static __thread int x;` or `__thread int x;`
//    - Each thread gets its own *independent* copy of the variable
//    - Same thread sees the same value across function calls
//    - Other threads have separate, isolated copies
//
// Notes:
// - `__thread` works with statics or globals, not automatic (stack) locals
// - Do NOT use `__thread` for pointers to shared data — the pointer itself is per-thread,
//   but not the data it points to unless also thread-local
// - `__thread` is non-standard (GCC/Clang). For portable POSIX code, use `pthread_key_t`.
//
// Example:
//   static __thread int counter = 0;
//   counter++; // each thread increments its own 'counter'
//--------------------------------------------------------------------------------

int global_var       = 0;  // Shared across all threads
__thread int tls_var = 0;  // Thread-local: each thread gets its own copy

void* thread_func(void* arg) {
  int local_var              = 0;  // Private to this function call
  static int static_func_var = 0;  // Shared across all threads

  size_t idx = *(size_t*)arg;

  for (int i = 0; i < 3; i++) {
    global_var++;
    local_var++;
    static_func_var++;
    tls_var++;

    printf("Thread %zu | global=%d, local=%d, static_func=%d, tls=%d\n", idx, global_var, local_var, static_func_var, tls_var);

    sleep(1);
  }

  free(arg);

  return NULL;
}

int main() {
  pthread_t threads[2];

  for (size_t i = 0; i < 2; i++) {
    size_t* arg = (size_t*)malloc(sizeof(size_t));
    if (arg == NULL) {
      perror("malloc");
      exit(EXIT_FAILURE);
    }
    *arg = i + 1;

    if (pthread_create(&threads[i], NULL, thread_func, (void*)arg) != 0) {
      perror("pthread_create");
      exit(EXIT_FAILURE);
    }
  }

  for (size_t i = 0; i < 2; ++i) {
    if (pthread_join(threads[i], NULL) != 0) {
      perror("pthread_join");
    }
  }
}
