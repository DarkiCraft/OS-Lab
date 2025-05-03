#include <errno.h>      // for errno
#include <fcntl.h>      // for open()
#include <stdio.h>      // for perror(), fprintf()
#include <stdlib.h>     // for exit()
#include <sys/stat.h>   // for mkfifo()
#include <sys/types.h>  // for pid_t
#include <sys/wait.h>   // for wait()
#include <unistd.h>     // for unlink(), read(), write(), close()

int mkfifo(const char* path_name, mode_t mode);
// Brief: Creates a named pipe (FIFO) for inter-process communication.
//
// Parameters:
//   path_name - The path where the FIFO will be created. It should be a valid path to a file
//              in a writable directory, such as "/tmp/my_named_pipe".
//   mode     - The file permissions for the FIFO. These are specified using the usual mode
//              bits, such as 0666 for read-write permissions for all users.
//
// Returns:
//   0 on success; -1 on failure, setting errno to indicate the error.
//
// Errors:
// - EEXIST  - The FIFO already exists at the specified path.
// - ENOSPC  - No space left on device to create the FIFO.
// - EFAULT  - Invalid address for the pathname argument.
// - ENOMEM  - Insufficient memory to create the FIFO.
// - EPERM   - Insufficient permissions to create the FIFO.
//
// Usage:
//   if (mkfifo("/tmp/my_named_pipe", 0666) == -1) {
//       perror("mkfifo");
//       exit(EXIT_FAILURE);
// }
//   The FIFO can then be opened using `open()` for reading and writing.
//
// Notes:
// - A FIFO is a named pipe that appears as a special file in the filesystem.
// - FIFOs allow communication between unrelated processes.
// - Data written to a FIFO is buffered until read by another process.
// - The FIFO file will persist in the filesystem until unlinked.
//
// Search mkfifo(2) for more information about named pipes and file permissions.

int unlink(const char* path_name);
// Brief: Removes a file or a named pipe (FIFO) from the filesystem.
//
// Parameters:
//   path_name - The path to the file or FIFO to be removed from the filesystem.
//
// Returns:
//   0 on success; -1 on failure, setting errno to indicate the error.
//
// Errors:
// - ENOENT  - The specified file does not exist.
// - EACCES  - Permission denied to unlink the file.
// - EBUSY   - The file is currently open and in use (for FIFOs, in particular).
// - EFAULT  - Invalid address for the pathname argument.
//
// Usage:
//   if (unlink("/tmp/my_named_pipe") == -1) {
//       perror("unlink");
//       exit(EXIT_FAILURE);
// }
//
// Notes:
// - After unlinking, the FIFO is removed from the filesystem and can no longer be accessed by path.
// - If any process has the FIFO open when it is unlinked, the FIFO remains available until it is closed.
// - Unlinking a FIFO is important for cleaning up system resources and avoiding unnecessary filesystem clutter.
//
// Search unlink(2) for more information about file deletion and cleanup.

const char* FIFO_PATH = "/tmp/my_named_pipe";  // can be some other name too
const int BUFFER_SIZE = 1024;

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

// Resource cleanup functions
void cleanup_fifo() {
  if (unlink(FIFO_PATH) != 0) {
    perror("unlink");
  }
}

void close_fd(int fd) {
  if (close(fd) != 0) {
    perror("close");
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

// Handle child process logic
void child_process() {
  int fd = open(FIFO_PATH, O_WRONLY);
  if (fd == -1) {
    handle_error("open (child)");
  }

  int num;
  (void)printf("Enter number of elements: ");
  if (scanf("%d", &num) != 1) {
    (void)fprintf(stderr, "Invalid input.\n");
    close_fd(fd);
    exit(EXIT_FAILURE);
  }

  if (num <= 0 || num > (int)(BUFFER_SIZE / sizeof(int))) {
    (void)fprintf(stderr, "0 < num < %d\n", (int)(BUFFER_SIZE / sizeof(int) + 1));
    close_fd(fd);
    exit(EXIT_FAILURE);
  }

  int* arr = (int*)malloc(num * sizeof(int));
  check_pointer(arr, "malloc");

  (void)printf("Enter %d numbers: ", num);
  for (size_t i = 0; i < (size_t)num; i++) {
    if (scanf("%d", &arr[i]) != 1) {
      (void)fprintf(stderr, "Invalid input.\n");
      free(arr);
      close_fd(fd);
      exit(EXIT_FAILURE);
    }
  }

  if (write_all(fd, &num, sizeof(int)) == -1 || write_all(fd, arr, num * sizeof(int)) == -1) {
    perror("write_all");
    free(arr);
    close_fd(fd);
    exit(EXIT_FAILURE);
  }

  free(arr);
  close_fd(fd);
}

// Handle parent process logic
void parent_process() {
  int fd = open(FIFO_PATH, O_RDONLY);
  if (fd == -1) {
    perror("open (parent)");
    cleanup_fifo();
    exit(EXIT_FAILURE);
  }

  int num;
  if (read_all(fd, &num, sizeof(int)) == -1) {
    perror("read_all");
    close_fd(fd);
    cleanup_fifo();
    exit(EXIT_FAILURE);
  }

  int* arr = (int*)malloc(num * sizeof(int));
  if (arr == NULL) {
    perror("malloc");
    close_fd(fd);
    cleanup_fifo();
    exit(EXIT_FAILURE);
  }

  if (read_all(fd, arr, num * sizeof(int)) == -1) {
    perror("read_all");
    free(arr);
    close_fd(fd);
    cleanup_fifo();
    exit(EXIT_FAILURE);
  }

  for (size_t i = 0; i < (size_t)num; i++) {
    (void)printf("%d ", arr[i]);
  }
  (void)putchar('\n');

  free(arr);
  close_fd(fd);
  cleanup_fifo();

  if (wait(NULL) == -1) {
    perror("wait");
  }
}

// Program to send numbers from child to parent using a named pipe (FIFO)
int main() {
  // Create the named pipe with read-write permissions
  if (mkfifo(FIFO_PATH, 0666) == -1) {
    if (errno != EEXIST) {  // Ignore if the FIFO already exists
      handle_error("mkfifo");
    }
  }

  pid_t pid = fork();
  if (pid == -1) {
    perror("fork");
    cleanup_fifo();
    exit(EXIT_FAILURE);
  }

  if (pid == 0) {
    // Child process
    child_process();
  } else {
    // Parent process
    parent_process();
  }

  return EXIT_SUCCESS;
}