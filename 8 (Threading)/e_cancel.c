#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // for sleep()

//--------------------------------------------------------------------------------
// int pthread_cancel(pthread_t thread);
// Brief: Requests cancellation of the target thread (asynchronously).
//
// Parameters:
// - thread: The thread to cancel (pthread_t)
//
// Returns: 0 on success; non-zero error-number on failure
//
// Errors:
// - ESRCH - No thread with the ID thread could be found.
//
// Notes:
// - This is a *request*. Actual cancellation depends on the target thread's cancelability settings.
// - Thread must be in a cancelable state and reach a *cancellation point* to be canceled.
// - A canceled thread will exit with status PTHREAD_CANCELED.
// - The target thread can disable or defer cancellation (see pthread_setcancelstatetype)
//
// Search pthread_cancel(3) | pthread_setcancelstate(3) for more info
// Scroll down https://man7.org/linux/man-pages/man7/pthreads.7.html for the list of all cancellations points
//--------------------------------------------------------------------------------
// int pthread_setcancelstate(int state, int* oldstate);
// Brief: Enables or disables thread cancellation.
//
// Parameters:
// - state: One of the following:
//     - PTHREAD_CANCEL_ENABLE: Thread can be canceled (default)
//     - PTHREAD_CANCEL_DISABLE: Thread ignores cancel requests
// - oldstate: (optional) Stores previous state
//
// Returns: 0 on success; non-zero error-number on failure
//
// Notes:
// - Disabling cancel means pthread_cancel() has no effect until re-enabled
// - Good to disable cancel briefly while in critical sections (e.g., resource cleanup)
//--------------------------------------------------------------------------------
// int pthread_setcanceltype(int type, int* oldtype);
// Brief: Controls *when* cancellation happens (if enabled).
//
// Parameters:
// - type: One of the following:
//     - PTHREAD_CANCEL_DEFERRED: Cancel only at cancellation points (default)
//     - PTHREAD_CANCEL_ASYNCHRONOUS: Cancel immediately (dangerous)
// - oldtype: (optional) Stores previous type
//
// Returns: 0 on success; non-zero error-number on failure
//
// Notes:
// - Prefer DEFERRED: safer and more predictable
// - ASYNCHRONOUS can interrupt code at any moment — risky
//--------------------------------------------------------------------------------
// void pthread_testcancel();
// Brief: Explicitly checks for pending cancellation and exits if requested.
//
// Usage: Place manually inside long loops or CPU-bound code that lacks natural
//        cancellation points (like sleep, read, write, etc.)
//
// Notes:
// - Only has effect if cancellation is enabled and type is DEFERRED
// - Returns nothing; if cancel is pending, thread exits immediately with
//   return value PTHREAD_CANCELED
//--------------------------------------------------------------------------------

void* worker_thread(void* arg) {
  (void)arg;
  printf("Worker: Started (will run for ~10s unless cancelled)\n");

  // Enable cancellation (default, but good to be explicit)
  pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);  // only cancel at cancellation points

  for (int i = 0; i < 10; ++i) {
    printf("Worker: %d\n", i);
    sleep(1);  // sleep() is a cancellation point
  }

  printf("Worker: Finished normally\n");
  return NULL;
}

int main() {
  pthread_t tid;

  // Create the thread
  if (pthread_create(&tid, NULL, worker_thread, NULL) != 0) {
    perror("pthread_create");
    return EXIT_FAILURE;
  }

  // Let the thread run for 3 seconds
  sleep(3);

  // Cancel the thread
  printf("Main: Requesting cancellation of worker\n");
  if (pthread_cancel(tid) != 0) {
    perror("pthread_cancel");
    return EXIT_FAILURE;
  }

  // Wait for the thread to finish and check if it was canceled
  void* retval;
  if (pthread_join(tid, &retval) != 0) {
    perror("pthread_join");
    return EXIT_FAILURE;
  }

  if (retval == PTHREAD_CANCELED) {
    printf("Main: Worker thread was canceled\n");
  } else {
    printf("Main: Worker thread exited normally\n");
  }
}
