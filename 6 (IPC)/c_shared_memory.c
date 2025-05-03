#include <fcntl.h>      // for O_CREAT, O_RDWR
#include <stdio.h>      // for perror(), fprintf()
#include <stdlib.h>     // for exit(), malloc()
#include <string.h>     // for memset()
#include <sys/mman.h>   // for shm_open(), mmap(), munmap()
#include <sys/types.h>  // for pid_t
#include <sys/wait.h>   // for wait()
#include <unistd.h>     // for ftruncate(), close()

#define SHM_NAME "/my_shared_memory"
#define BUFFER_SIZE 1024

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
  if (ptr == MAP_FAILED) {
    handle_error(msg);
  }
}

// Resource cleanup functions
void cleanup_shm(void* ptr, int shm_fd, int unlink_shm) {
  if (ptr != NULL && ptr != MAP_FAILED) {
    if (munmap(ptr, BUFFER_SIZE) != 0) {
      perror("munmap");
    }
  }

  if (shm_fd != -1) {
    if (close(shm_fd) != 0) {
      perror("close");
    }
  }

  if (unlink_shm != 0) {
    if (shm_unlink(SHM_NAME) != 0) {
      perror("shm_unlink");
    }
  }
}

// Handle child process logic - write to shared memory
void child_process(void* ptr, int shm_fd) {
  int num;
  (void)printf("Enter number of elements: ");
  if (scanf("%d", &num) != 1) {
    (void)fprintf(stderr, "Invalid input.\n");
    cleanup_shm(ptr, shm_fd, 0);  // Child doesn't unlink
    exit(EXIT_FAILURE);
  }

  if (num <= 0 || num > (int)(BUFFER_SIZE / sizeof(int)) - 1) {
    (void)fprintf(stderr, "0 < num < %d\n", (int)(BUFFER_SIZE / sizeof(int)));
    cleanup_shm(ptr, shm_fd, 0);  // Child doesn't unlink
    exit(EXIT_FAILURE);
  }

  int* shm_data = (int*)ptr;
  shm_data[0]   = num;

  (void)printf("Enter %d numbers: ", num);
  for (int i = 0; i < num; i++) {
    if (scanf("%d", &shm_data[i + 1]) != 1) {
      (void)fprintf(stderr, "Invalid input.\n");
      cleanup_shm(ptr, shm_fd, 0);  // Child doesn't unlink
      exit(EXIT_FAILURE);
    }
  }

  // Normal cleanup - no need to handle errors here as we're exiting anyway
  cleanup_shm(ptr, shm_fd, 0);  // Child doesn't unlink
}

// Handle parent process logic - read from shared memory
void parent_process(void* ptr, int shm_fd) {
  // Wait for child to finish writing
  if (wait(NULL) != 0) {
    perror("wait");
    cleanup_shm(ptr, shm_fd, 1);  // Parent unlinks
    exit(EXIT_FAILURE);
  }

  int* shm_data = (int*)ptr;
  int num       = shm_data[0];

  if (num <= 0 || num > (int)(BUFFER_SIZE / sizeof(int)) - 1) {
    (void)fprintf(stderr, "Invalid number of elements in shared memory: %d\n", num);
    cleanup_shm(ptr, shm_fd, 1);  // Parent unlinks
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < num; i++) {
    (void)printf("%d ", shm_data[i + 1]);
  }
  (void)putchar('\n');

  // Normal cleanup - parent always unlinks
  cleanup_shm(ptr, shm_fd, 1);
}

int main() {
  // Create or open the shared memory object
  int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  if (shm_fd == -1) {
    handle_error("shm_open");
  }

  // Set the size of the shared memory object
  if (ftruncate(shm_fd, BUFFER_SIZE) == -1) {
    perror("ftruncate");
    cleanup_shm(NULL, shm_fd, 1);
    exit(EXIT_FAILURE);
  }

  // Map the shared memory object into the address space
  void* ptr = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  check_pointer(ptr, "mmap");

  pid_t pid = fork();
  if (pid == -1) {
    perror("fork");
    cleanup_shm(ptr, shm_fd, 1);
    exit(EXIT_FAILURE);
  }

  if (pid == 0) {
    // Child process
    child_process(ptr, shm_fd);
  } else {
    // Parent process
    parent_process(ptr, shm_fd);
  }

  return EXIT_SUCCESS;
}