#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

//--------------------------------------------------------------------------------
// Mutex (Mutual Exclusion)
// - Mutexes are used to prevent race conditions by allowing only one thread to access a critical section at a time
//   -> A thread locks the mutex before entering the critical section.
//   -> Once done, the thread unlocks the mutex, allowing other threads to enter.
// - If a thread tries to enter a critical section while it’s locked, it will be blocked until unlock
// - Deadlocks happen when two or more threads are waiting for each other to release locks
//   -> To avoid, the lock ordering in distinct functions should be the same
// - Threads that don’t have the mutex lock can access locked resources directly
// - Reentrant mutexes allow a thread to lock the same mutex multiple times
//   -> Useful in recursive function calls
//--------------------------------------------------------------------------------
// int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr);
// int pthread_mutex_destroy(pthread_mutex_t* mutex)
// Brief: Initialize and destroy a mutex
//
// Parameters: mutex - Pointer to a mutex object
//             attr  - Pointer to an attribute object
//                     -> Pass NULL for default attributes
//
// Returns: 0 on success; non-zero error-number on error, and errno is set
//
// Errors:
// - EBUSY  - attempted to destroy an object in use (locked e.g.)
// - EINVAL - mutex is an invalid object, or attr is an invalid object
// - EAGAIN - system lacks resources (other than memory) to initialize mutex object
// - ENOMEM - system lacks memory initialize mutex object
// - EPERM  - lack of permissions to perform the necessary operation
//
// Usage:
//   pthread_mutex_t mutex;
//   pthread_mutexattr_t attr;                     // initialize attributes as needed
//   if (pthread_mutex_init(&mutex, &attr) != 0) { // pass NULL in place of attr for defaults
//     perror("pthread_mutex_init");
//     // handle error accordingly
//   }
//   // mutex is used
//   if (pthread_mutex_destroy(&mutex) != 0) {
//     perror("pthread_mutex_init");
//     // handle error accordingly
//   }
//
// Notes:
// - If default attributes are desired, a static initialization may be performed by:
//   pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
// - A mutex may be reinitialized after it is destroyed
// - The following is undefined behaviour:
//   -> Using an unitialized mutex
//   -> Initializing a mutex multiple times (without destroying)
//   -> Destroying an uninitialized mutex
//   -> Destroying a destroyed mutex
//
// Search pthread_mutex_init(2) | pthread_mutex_destroy(2) for more information
//--------------------------------------------------------------------------------
// int pthread_mutex_lock(pthread_mutex_t* mutex);
// int pthread_mutex_trylock(pthread_mutex_t* mutex);
// int pthread_mutex_unlock(pthread_mutex_t* mutex);
// Brief: Lock and unlock a mutex
//
// Parameters: mutex - Pointer to a mutex object (must be initialized)
//
// Returns: 0 on success; non-zero error-number on error, and errno is set
//
// Errors:
// - EINVAL - mutex is invalid or not initialized
// - EAGAIN - maximum number of recursive locks exceeded (recursive mutexes only)
// - EDEADLK - deadlock detected (e.g., thread already holds the lock on non-recursive mutex)
// - EBUSY  - (trylock only) mutex is already locked by another thread
// - EPERM  - (unlock only) current thread does not own the mutex
//
// Usage:
//   pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//   // Lock the mutex (blocks until acquired)
//   if (pthread_mutex_lock(&mutex) != 0) {
//     perror("pthread_mutex_lock");
//     // handle error accordingly
//   }
//   // Critical section
//   // ...
//   // Unlock the mutex
//   if (pthread_mutex_unlock(&mutex) != 0) {
//     perror("pthread_mutex_unlock");
//     // handle error accordingly
//   }
//
//   // Try to lock the mutex without blocking
//   if (pthread_mutex_trylock(&mutex) == 0) {
//     // Critical section
//     pthread_mutex_unlock(&mutex);
//   } else {
//     // mutex was already locked; handle accordingly
//   }
//
// Notes:
// - A thread attempting to lock a mutex it already owns (on a non-recursive mutex) causes deadlock
// - trylock is non-blocking; it fails immediately if the mutex is already locked
// - Only the thread that locked the mutex should unlock it
// - Undefined behaviour:
//   -> Locking or unlocking an uninitialized mutex
//   -> Unlocking a mutex not owned by the calling thread
//
// Search pthread_mutex_lock(2) | pthread_mutex_trylock(2) | pthread_mutex_unlock(2) for more information
//--------------------------------------------------------------------------------
// int pthread_mutex_timedlock(pthread_mutex_t* mutex, const struct timespec* timeout);
// Brief: Attempt to lock a mutex, blocking until acquired or timeout expires
//
// Parameters:
//   mutex   - Pointer to a mutex object (must be initialized)
//   timeout - Absolute timeout (CLOCK_REALTIME); specifies latest time to acquire lock
//
// Returns: 0 on success; non-zero error-number on error or timeout, and errno is set
//
// Errors:
// - EINVAL - mutex or timeout is invalid; timeout value is in the past
// - ETIMEDOUT - mutex could not be locked before the specified timeout expired
// - EAGAIN - maximum number of recursive locks exceeded (recursive mutexes only)
// - EDEADLK - deadlock detected (e.g., thread already owns the mutex)
// - ENOTSUP - system does not support pthread_mutex_timedlock
//
// Usage:
//   pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
//   struct timespec ts;
//   clock_gettime(CLOCK_REALTIME, &ts);
//   ts.tv_sec += 5; // wait up to 5 seconds
//   if (pthread_mutex_timedlock(&mutex, &ts) != 0) {
//     perror("pthread_mutex_timedlock");
//     // handle timeout or error
//   } else {
//     // Critical section
//     pthread_mutex_unlock(&mutex);
//   }
//
// Notes:
// - timeout is an *absolute* time, not a duration
// - Requires CLOCK_REALTIME support; may be affected by system time changes
// - Behavior is undefined if used on an uninitialized mutex
// - Acts like pthread_mutex_lock, but returns ETIMEDOUT if not acquired in time
//
// Search pthread_mutex_timedlock(2) for more information
//--------------------------------------------------------------------------------

pthread_mutex_t mutex;
int shared_data = 0;

void* unsafe_direct_access(void* arg) {
  // Directly modifies shared data without locking (unsafe)
  for (int i = 0; i < 5; ++i) {
    shared_data++;  // no lock
    printf("[Unsafe] shared_data = %d\n", shared_data);
    usleep(100000);  // simulate work
  }
  return NULL;
}

void* safe_mutex_lock_access(void* arg) {
  for (int i = 0; i < 5; ++i) {
    if (pthread_mutex_lock(&mutex) != 0) {
      perror("pthread_mutex_lock");
      return NULL;
    }

    shared_data++;
    (void)printf("[Safe] shared_data = %d\n", shared_data);

    if (pthread_mutex_unlock(&mutex) != 0) {
      perror("pthread_mutex_unlock");
      return NULL;
    }
    usleep(100000);
  }
  return NULL;
}

int main() {
  // 1. Initialize mutex
  if (pthread_mutex_init(&mutex, NULL) != 0) {
    perror("pthread_mutex_init");
    exit(EXIT_FAILURE);
  }

  // 2. Demonstrate unsafe access
  printf("=== UNSAFE ACCESS (NO LOCK) ===\n");
  pthread_t t1, t2;
  if (pthread_create(&t1, NULL, unsafe_direct_access, NULL) != 0 || pthread_create(&t2, NULL, unsafe_direct_access, NULL) != 0) {
    perror("pthread_create");
    pthread_mutex_destroy(&mutex);
    exit(EXIT_FAILURE);
  }

  pthread_join(t1, NULL);
  pthread_join(t2, NULL);

  shared_data = 0;  // reset

  // 3. Demonstrate safe access with lock/unlock
  printf("\n=== SAFE ACCESS (WITH LOCK) ===\n");
  if (pthread_create(&t1, NULL, safe_mutex_lock_access, NULL) != 0 || pthread_create(&t2, NULL, safe_mutex_lock_access, NULL) != 0) {
    perror("pthread_create");
    pthread_mutex_destroy(&mutex);
    exit(EXIT_FAILURE);
  }

  pthread_join(t1, NULL);
  pthread_join(t2, NULL);

  // 4. Trylock demo
  printf("\n=== TRYLOCK DEMO ===\n");
  int ret = pthread_mutex_trylock(&mutex);
  if (ret == 0) {
    printf("trylock succeeded: acquired mutex\n");
    if (pthread_mutex_unlock(&mutex) != 0) perror("pthread_mutex_unlock");
  } else if (ret == EBUSY) {
    printf("trylock failed: mutex already locked\n");
  } else {
    fprintf(stderr, "pthread_mutex_trylock: %s\n", strerror(ret));
  }

  // 5. Timedlock demo
  printf("\n=== TIMEDLOCK DEMO ===\n");
  if (pthread_mutex_lock(&mutex) != 0) {
    perror("pthread_mutex_lock (before timedlock)");
    pthread_mutex_destroy(&mutex);
    exit(EXIT_FAILURE);
  }

  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += 2;

  ret = pthread_mutex_timedlock(&mutex, &ts);
  if (ret == 0) {
    printf("timedlock succeeded: acquired mutex\n");
    pthread_mutex_unlock(&mutex);
  } else if (ret == ETIMEDOUT) {
    printf("timedlock timed out: mutex was not acquired\n");
  } else {
    fprintf(stderr, "pthread_mutex_timedlock: %s\n", strerror(ret));
  }

  // Unlock from original lock
  if (pthread_mutex_unlock(&mutex) != 0) {
    perror("pthread_mutex_unlock (after timedlock)");
  }

  // 6. Destroy mutex
  if (pthread_mutex_destroy(&mutex) != 0) {
    perror("pthread_mutex_destroy");
    exit(EXIT_FAILURE);
  }

  printf("\nProgram completed successfully.\n");
}
