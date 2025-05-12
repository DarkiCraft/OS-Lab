#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//--------------------------------------------------------------------------------
// Basics:
// - Semaphores are signaling mechanisms used to control access to shared resources
//   -> A semaphore holds a value representing available "permits"
//   -> Threads call wait (decrement) before entering critical section
//   -> When done, they call post (increment) to release the permit
// - If the semaphore value is 0, wait blocks until another thread posts
// - Unlike mutexes, semaphores can allow multiple threads into the critical section
//   -> Useful for managing access to a limited number of resources (e.g., connection pool)
// - trywait offers non-blocking behavior, returning immediately if no permits available
// - getvalue retrieves current count (can be negative if threads are waiting)
// - Common uses include:
//   -> Binary semaphore (initial value 1): similar to a mutex
//   -> Counting semaphore (initial value > 1): allows N threads access
// - Deadlocks can still occur if semaphores are not carefully ordered
// - Always ensure proper pairing of wait and post to prevent starvation or leaks
// - Unlike mutexes, semaphores are not tied to ownership (any thread can post or wait)
//--------------------------------------------------------------------------------
// int sem_init(sem_t* sem, int pshared, unsigned int value);
// int sem_destroy(sem_t* sem);
// Brief: Initialize and destroy an unnamed semaphore
//
// Parameters:
//   sem     - Pointer to a semaphore object
//   pshared - 0 for thread sharing (within process); non-zero for process sharing
//   value   - Initial value (must be >= 0)
//
// Returns: 0 on success; -1 on error, and errno is set
//
// Errors:
// - EINVAL - invalid value, or pshared not supported
// - ENOSYS - semaphores not supported on this system
// - ENOMEM - insufficient memory to initialize
// - EBUSY  - attempted to destroy a semaphore still in use
//
// Usage:
//   sem_t sem;
//   if (sem_init(&sem, 0, 1) == -1) {
//     perror("sem_init");
//     // handle error
//   }
//   // semaphore usage
//   if (sem_destroy(&sem) == -1) {
//     perror("sem_destroy");
//     // handle error
//   }
//
// Notes:
// - Unnamed semaphores are used for synchronization among threads in same process
// - If pshared != 0, the semaphore must be placed in shared memory
// - Behavior is undefined if using an uninitialized semaphore
// - A semaphore must be destroyed only after no thread is using it
//
// Search sem_init(3) | sem_destroy(3) for more information
//--------------------------------------------------------------------------------
// int sem_getvalue(sem_t* sem, int* sval);
// Brief: Retrieve the current value of a semaphore
//
// Parameters:
//   sem  - Pointer to a valid semaphore object
//   sval - Pointer to an integer to store the value
//
// Returns: 0 on success; -1 on error, and errno is set
//
// Errors:
// - EINVAL - sem or sval is invalid
//
// Usage:
//   int sval;
//   if (sem_getvalue(&sem, &sval) == -1) {
//     perror("sem_getvalue");
//     // handle error
//   } else {
//     printf("Current semaphore value: %d\n", sval);
//   }
//
// Notes:
// - The value reported may be outdated immediately after retrieval
// - If threads are blocked in sem_wait, the value may be negative (e.g., -N)
// - Use for debugging or informative purposes only, not synchronization logic
//
// Search sem_getvalue(3) for more information
//--------------------------------------------------------------------------------
// int sem_post(sem_t* sem);
// Brief: Increment (unlock) a semaphore
//
// Parameters:
//   sem - Pointer to a valid semaphore object
//
// Returns: 0 on success; -1 on error, and errno is set
//
// Errors:
// - EINVAL - sem is invalid
// - EOVERFLOW - value exceeds SEM_VALUE_MAX
//
// Usage:
//   if (sem_post(&sem) == -1) {
//     perror("sem_post");
//     // handle error
//   }
//
// Notes:
// - Increments the semaphore value; if threads are blocked in sem_wait, one is woken
// - Acts like unlock; multiple posts may be needed for multiple waiters
// - Overposting is allowed but can overflow if not managed
//
// Search sem_post(3) for more information
//--------------------------------------------------------------------------------
// int sem_wait(sem_t *sem);
// int sem_trywait(sem_t *sem);
// Brief: Decrement (lock) a semaphore; blocks or returns immediately if unavailable
//
// Parameters:
//   sem - Pointer to a valid semaphore object
//
// Returns: 0 on success; -1 on error, and errno is set
//
// Errors:
// - EINVAL - sem is invalid
// - EAGAIN - (trywait only) semaphore currently unavailable
// - EINTR  - (wait only) call was interrupted by signal
//
// Usage:
//   if (sem_wait(&sem) == -1) {
//     perror("sem_wait");
//     // handle error (e.g., EINTR)
//   }
//
//   if (sem_trywait(&sem) == -1) {
//     if (errno == EAGAIN) {
//       printf("Semaphore not available (non-blocking)\n");
//     } else {
//       perror("sem_trywait");
//       // handle error
//     }
//   }
//
// Notes:
// - sem_wait blocks until semaphore value > 0, then decrements it
// - sem_trywait returns immediately if the value is 0
// - Repeated wait calls can starve threads if not balanced with post
// - Do not destroy a semaphore while threads are blocked in wait
//
// Search sem_wait(3) | sem_trywait(3) for more information
//--------------------------------------------------------------------------------

#define THREAD_COUNT 4
#define ITERATIONS 5

int shared_counter = 0;
sem_t semaphore;

void safe_increment(const char* name) {
  for (int i = 0; i < ITERATIONS; i++) {
    if (sem_wait(&semaphore) == -1) {
      perror("sem_wait");
      return;
    }

    int temp = shared_counter;
    usleep(10000);  // simulate work
    shared_counter = temp + 1;
    printf("[%s] safely incremented: %d\n", name, shared_counter);

    if (sem_post(&semaphore) == -1) {
      perror("sem_post");
      return;
    }

    usleep(10000);
  }
}

void unsafe_increment(const char* name) {
  for (int i = 0; i < ITERATIONS; i++) {
    int temp = shared_counter;
    usleep(10000);  // simulate work
    shared_counter = temp + 1;
    printf("[%s] unsafely incremented: %d (⚠️ no lock)\n", name, shared_counter);
    usleep(10000);
  }
}

void* thread_routine(void* arg) {
  const char* name = (const char*)arg;

  if (strcmp(name, "UnsafeThread") == 0)
    unsafe_increment(name);
  else
    safe_increment(name);

  return NULL;
}

int main() {
  pthread_t threads[THREAD_COUNT];
  const char* names[THREAD_COUNT] = {"SafeThread1", "SafeThread2", "UnsafeThread", "TrywaitThread"};

  // 1. Initialize semaphore
  if (sem_init(&semaphore, 0, 1) == -1) {
    perror("sem_init");
    exit(EXIT_FAILURE);
  }

  // 2. Get initial value
  int sval;
  if (sem_getvalue(&semaphore, &sval) == -1) {
    perror("sem_getvalue");
  } else {
    printf("Initial semaphore value: %d\n", sval);
  }

  // 3. Demonstrate trywait
  printf("[Main] Trying sem_trywait...\n");
  if (sem_trywait(&semaphore) == -1) {
    if (errno == EAGAIN)
      printf("[Main] Semaphore unavailable (EAGAIN)\n");
    else
      perror("sem_trywait");
  } else {
    printf("[Main] sem_trywait succeeded\n");
    sem_post(&semaphore);  // restore
  }

  // 4. Create threads
  for (int i = 0; i < THREAD_COUNT; i++) {
    if (pthread_create(&threads[i], NULL, thread_routine, (void*)names[i]) != 0) {
      perror("pthread_create");
      exit(EXIT_FAILURE);
    }
  }

  // 5. Join threads
  for (int i = 0; i < THREAD_COUNT; i++) {
    pthread_join(threads[i], NULL);
  }

  // 6. Final counter value
  printf("\nFinal shared counter value: %d\n", shared_counter);

  // 7. Destroy semaphore
  if (sem_destroy(&semaphore) == -1) {
    perror("sem_destroy");
  }
}