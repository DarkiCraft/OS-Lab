#include <stdio.h>      // for perror()
#include <stdlib.h>     // for exit()
#include <sys/types.h>  // for pid_t
#include <sys/wait.h>   // for wait()
#include <unistd.h>     // for pipe(), read(), write(), close()

int pipe(int pipe_fd[2]);
// Brief: Creates an unnamed pipe for inter-process communication.
//
// Paramters: pipe_fd An array of two integers that will be filled with the file descriptors for the pipe.
//                    - pipe_fd[0]: the read end of the pipe.
//                    - pipe_fd[1]: the write end of the pipe.
//
// Returns: 0 on success; -1 on failure, setting errno to indicate the error.
//
// Errors:
// - EFAULT - Invalid address (pipe_fd points to an invalid memory address).
// - EMFILE - Too many file descriptors are in use by the process.
// - ENFILE - The system-wide limit on the number of open file descriptors has been reached.
// - ENOMEM - Insufficient memory to create the pipe buffer.
// - EPERM  - The process does not have permission to create a pipe.
//
// Usage:
//   int pipe_fd[2];
//   if (pipe(pipe_fd) == -1) {
//       perror("pipe");
//       exit(EXIT_FAILURE);
//   }
//
// pipe_fd[0] is the read end, pipe_fd[1] is the write end
// Use pipe_fd[0] to read and pipe_fd[1] to write.
//
// Notes:
// - A pipe is unidirectional (data flows in one direction only).
// - The write end of the pipe is used by the producer (parent process or child).
// - The read end of the pipe is used by the consumer (child process or parent).
// - When the write end of the pipe is closed, the reader receives EOF (End of File) on subsequent reads.
// - Data written to the pipe is buffered and read in chunks by the reader.
// - If the pipe is full and the writer tries to write, the write call blocks until space is available.
// - If the pipe is empty and the reader tries to read, the read call blocks until data is available.
// - Closing unused ends of the pipe in both the parent and child process is critical to avoid memory leaks and unexpected behavior.
// - Always ensure that file descriptors are closed after use to avoid resource leakage.
//
// Search pipe(2) for more information about file descriptors and process communication.

const int BUFFER_SIZE = 1024;
const int READ_END    = 0;
const int WRITE_END   = 1;

// Error handling utilities as functions
void handle_error(const char* msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}

void check_result(int result, const char* msg) {
  if (result == -1) {
    handle_error(msg);
  }
}

void check_pointer(void* ptr, const char* msg) {
  if (ptr == NULL) {
    handle_error(msg);
  }
}

// Since pipes do not guarantee all the data is read in one go, we can use loops to ensure complete read/write
ssize_t write_all(int fd, const void* buffer, size_t bytes) {
  size_t total    = 0;
  const char* ptr = buffer;
  while (total < bytes) {
    ssize_t written = write(fd, ptr + total, bytes - total);
    if (written <= 0) {
      return -1;
    }
    total += written;
  }
  return total;
}
ssize_t read_all(int fd, void* buffer, size_t bytes) {
  size_t total = 0;
  char* ptr    = buffer;
  while (total < bytes) {
    ssize_t r = read(fd, ptr + total, bytes - total);
    if (r <= 0) {
      return -1;
    }
    total += r;
  }
  return total;
}

// Function to handle child process logic
void child_process(int write_fd) {
  int num;
  (void)printf("Enter number of elements: ");
  if (scanf("%d", &num) != 1) {
    fprintf(stderr, "Invalid input.\n");
    exit(EXIT_FAILURE);
  }

  if (num <= 0 || num > (int)(BUFFER_SIZE / sizeof(int))) {
    fprintf(stderr, "0 < num < %d\n", (int)(BUFFER_SIZE / sizeof(int) + 1));
    exit(EXIT_FAILURE);
  }

  int* arr = (int*)malloc(num * sizeof(int));
  check_pointer(arr, "malloc");

  (void)printf("Enter %d numbers: ", num);
  for (size_t i = 0; i < (size_t)num; i++) {
    if (scanf("%d", &arr[i]) != 1) {
      fprintf(stderr, "Invalid input.\n");
      free(arr);
      exit(EXIT_FAILURE);
    }
  }

  // Write the number of elements
  if (write_all(write_fd, &num, sizeof(int)) == -1) {
    free(arr);
    handle_error("write_all");
  }

  // Write all numbers from array
  if (write_all(write_fd, arr, num * sizeof(int)) == -1) {
    free(arr);
    handle_error("write_all");
  }

  free(arr);
}

// Function to handle parent process logic
void parent_process(int read_fd) {
  int num;

  // Read number of elements
  check_result(read_all(read_fd, &num, sizeof(int)), "read_all");

  int* arr = (int*)malloc(num * sizeof(int));
  check_pointer(arr, "malloc");

  // Read all numbers to array
  check_result(read_all(read_fd, arr, num * sizeof(int)), "read_all");

  for (size_t i = 0; i < (size_t)num; i++) {
    (void)printf("%d ", arr[i]);
  }
  (void)putchar('\n');

  free(arr);
  check_result(wait(NULL), "wait");
}

// Program to send numbers from child to parent
int main() {
  int fd[2];
  check_result(pipe(fd), "pipe");

  pid_t pid;
  pid = fork();
  check_result(pid, "fork");

  if (pid == 0) {  // Child process
    check_result(close(fd[READ_END]), "close");
    child_process(fd[WRITE_END]);
    check_result(close(fd[WRITE_END]), "close");
  } else {  // Parent process
    check_result(close(fd[WRITE_END]), "close");
    parent_process(fd[READ_END]);
    check_result(close(fd[READ_END]), "close");
  }
}