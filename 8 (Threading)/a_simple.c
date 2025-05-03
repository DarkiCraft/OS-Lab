#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

//--------------------------------------------------------------------------------
// Basics
// - All threads are part of a single pool under the calling process
// - The process automatically calls exit() if all running threads have terminated OR if the main() thread terminates
// - If exit() is manually called by any thread in the process, every other thread terminates
// - Thread resources may or may not be cleaned up automatically
//   -> joinable threads must be manyally cleaned at pthread_join()
//   -> detached threads are cleaned automatically at completion
// - Only joinable threads can return; via pthread_join()
//   -> detached threads cannot return anywhere
//--------------------------------------------------------------------------------
// int pthread_create(pthread_t* thread_id, pthread_attr_t* attributes, void* (*routine)(void*), void* argument);
// Brief: Creates a thread for the passed routine
//
// Parameters: thread_id  - Pointer to a pthread_t object which will store the created thread's id
//             attributes - Thread attributes
//                          -> Pass NULL for default attributes
//             routine    - Pointer to function which will be called in the created thread
//                          -> The function must be of the form: void* routine(void* arg);
//             argument   - Pointer to argument which will be passed to the called routine
//                          -> Must be vast to (void*)
// Returns: 0 on succes; non-zero error-number on error, sets errno, and contents of *thread_id are undefined
//
// Errors:
// - EAGAIN - Insufficient resources to create another thread | some other system imposed limit
// - EINVAL - Invalid value of passed attributes
// - EPERM  - Invalid permissions in passed attributes
//
// Usage:
//   void* some_function(void* arg); // defined globally, or in another file, etc
//   pthread_t id;
//   int* some_arg; // example argument (see b_arguments.c for detail)
//   if (pthread_create(&id), NULL, some_function, (void*)some_arg) != 0) {
//     perror("pthread_create");
//     // handle error accordingly
//   }
//
//  Thread may be joined, or set as detached, or some other desired function may be performed via its id.
//
// Notes:
// - The new thread will only terminate in one of the following ways:
//   -> pthread_exit() is called in the routine body
//   -> return statement is reached in the routine body (practically same as pthread_exit())
//   -> It is canceled via pthread_cancel() (see e_cancel.c)
//   -> Any thread in the current process calls exit()
//   -> main() thread reaches return statement
// - The new thread's CPU-clocktime is set to 0
// - The new thread inherit's a copy of the creating thread's signal masks (see 9 (Signals) folder)
// - The new thread will have no pending signals
// - A thread is either joinable or detached (see d_attributes.c)
//
// Search pthread_create(3) for more information
//--------------------------------------------------------------------------------
// int pthread_join (pthread_t thread_id, void** ret);
// Brief: Joins the thread specified by thread_id, places the return value (if any) in ret, cleans up any resources
//
// Parameters: thread_id - Thread id to join
//             ret       - Pointer to pointer to an object to store the return value
//                         -> Must be cast to (void**)
//                         -> Pass NULL to ignore return value
//
// Returns: 0 on success, non-zero error-number on error, sets errno, and contents of **ret are undefined
//
// Errors:
// - EDEADLK - a deadlock was detected (2 threads joining each other e.g.)
// - EINVAL  - called join on a detached thread | another thread has already called join on this id
// - ESRCH   - id does not represent a valid thread
//
// Usage:
//   pthread_t id;
//   int* ret_val; // example return location (see c_returns for detail)
//   // Assume thread has been created and id is valid
//   if (pthread_join(id, (void**)&ret_val) != 0) {
//     perror("pthread_join");
//     // handle error accordingly
//   }
// Notes:
// - Waits for the thread to terminate
//   -> If the thread has already been terminated, it returns immediately, and follows cleanup
//   -> Not joining join-able threads is undefined behaviour and leaks resources
// - If the calling thread is cancelled on pthread_join(), the target thread is still joinable from another thread
// - If the target thread is cancelled, PTHREAD_CANCELED is placed in *ret_val (not **ret_val)
//
// Search pthread_join(3) for more information
//--------------------------------------------------------------------------------
// int pthread_exit(void* ret_val);
// Brief: Terminates the calling thread and returns a value via ret_val (if the thread is joinable). ret_val will be available to
//        the thread calling pthread_join()
//
// Parameters: ret_val - pointer to an object to return in pthread_join()
//                     -> must be cast to (void*)
//                     -> pass NULL if no return value is required
//
// Returns: void
//
// Errors: none
//
// Usage:
//   void* some_routine(void* arg) {
//     if (condition) {
//       pthread_exit(NULL);
//     }
//
//     int* ret_val; // example return value (see c_returns for detail)
//     pthread_exit((void*)ret_val);
//   }
//
// Thread calling pthread_join() will take the ret_val accordingly
//
// Notes:
// - Is the same as returning the respective object
// - If pthread_exit() is called in the main() thread,
//   it waits until all other threads in the process are terminated before terminating itself
//
// Search pthread_exit(3) for more information
//--------------------------------------------------------------------------------

void* function_a(void* arg) {
  (void)arg;
  (void)printf("In function_a()\n");
  pthread_exit(NULL);
}

void* function_b(void* arg) {
  (void)arg;
  (void)printf("In function_b()\n");
  pthread_exit(NULL);
}

void* function_c(void* arg) {
  (void)arg;
  (void)printf("In function_c()\n");
  pthread_exit(NULL);
}

int main() {
  void* (*functions[])(void*) = {function_a, function_b, function_c};  // array of function pointers to iterate over
  pthread_t thread_ids[3];
  int is_thread_created[3] = {0};  // to check which thread was successfully created

  for (size_t i = 0; i < sizeof(functions) / sizeof(void*); i++) {
    if (pthread_create(&thread_ids[i], NULL, functions[i], NULL) != 0) {
      perror("pthread_create");
    }
    is_thread_created[i] = 1;  // set current thread as created
  }

  for (size_t i = 0; i < sizeof(functions) / sizeof(void*); i++) {
    if (!is_thread_created[i]) {
      continue;  // only join threads which were created
    }

    if (pthread_join(thread_ids[i], NULL) != 0) {
      perror("pthread_join");
    }
  }

  // note that the order of execution is undefined
}