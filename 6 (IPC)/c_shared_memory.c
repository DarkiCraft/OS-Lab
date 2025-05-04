#include <fcntl.h>      // for O_CREAT, O_RDWR
#include <stdio.h>      // for perror(), fprintf(), printf()
#include <stdlib.h>     // for exit(), malloc()
#include <string.h>     // for memset()
#include <sys/mman.h>   // for shm_open(), mmap(), munmap(), shm_unlink()
#include <sys/types.h>  // for pid_t
#include <sys/wait.h>   // for wait()
#include <unistd.h>     // for ftruncate(), close()

//--------------------------------------------------------------------------------
// int shm_open(const char *name, int oflag, mode_t mode);
// Brief: Opens or creates a POSIX shared memory object.
//
// Parameters:
// - name: A string starting with '/' that identifies the shared memory object.
// - oflag: File access modes (e.g., O_CREAT | O_RDWR).
// - mode: File permission bits (e.g., 0666).
//
// Returns: File descriptor on success; -1 on failure with errno set.
//
// Errors:
// - EACCES: Permission denied.
// - EEXIST: O_CREAT | O_EXCL was used and the object already exists.
// - EINVAL: Name does not start with '/' or is too long.
// - EMFILE/ENFILE: Process or system file descriptor limits exceeded.
// - ENOENT: The shared memory object does not exist and O_CREAT was not specified.
//
// Usage:
//   int fd = shm_open("/my_shm", O_CREAT | O_RDWR, 0666);
//
// Notes:
// - Shared memory objects must be truncated using ftruncate() before mapping.
// - Use mmap() to map the object into memory.
// - Use shm_unlink() to remove the shared memory object from the system.
//
// Search shm_open(3) and mmap(2) for more information.
//--------------------------------------------------------------------------------
// int shm_unlink(const char *name);
// Brief: Removes a POSIX shared memory object name.
//
// Parameters:
// - name: The name of the shared memory object to remove (must start with '/').
//
// Returns: 0 on success; -1 on failure with errno set appropriately.
//
// Description:
// - Marks the shared memory object referenced by 'name' for removal.
// - The actual removal only happens once all processes have unmapped and closed it.
// - Similar in concept to unlinking a file: removes name, but not the data until
//   it is no longer in use.
//
// Errors:
// - EACCES: Permission denied.
// - ENAMETOOLONG: The name is too long.
// - ENOENT: The shared memory object does not exist.
//
// Usage:
//   shm_unlink("/my_shared_memory");
//
// Notes:
// - Should typically be done by the parent or the process that created the object.
// - It’s safe to call even if other processes still have it open or mapped.
//
// Search shm_unlink(3) for more details.
//--------------------------------------------------------------------------------
// int ftruncate(int fd, off_t length);
// Brief: Changes the size of the file or shared memory object associated with the file descriptor to the specified length.
//
// Parameters:
//   - fd: A file descriptor obtained via shm_open() or open().
//   - length: The desired size of the file or shared memory object in bytes.
//
// Returns:
//   - 0 on success.
//   - -1 on failure, with errno set to indicate the error.
//
// Description:
//   ftruncate() resizes a file or shared memory object. For shared memory, it is essential to set the
//   size before mapping the memory with mmap(). If the file is larger than the new size, the excess data
//   is discarded. If smaller, the file is truncated to the new size. The memory is not initialized when the
//   size is increased; the new space may contain garbage data.
//
// Errors:
//   - EBADF: The file descriptor is not valid.
//   - EINVAL: fd does not refer to a file that supports resizing.
//   - EFBIG: The length exceeds the maximum file size for the system.
//   - EIO: A hardware error occurred during the operation.
//   - ENOSPC: Insufficient space to extend the file or shared memory object.
//   - EINTR: The operation was interrupted by a signal.
//
// Usage:
//   int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
//   if (shm_fd == -1) {
//       handle_error("shm_open");
//   }
//
//   if (ftruncate(shm_fd, BUFFER_SIZE) == -1) {
//       perror("ftruncate");
//       cleanup_shm(NULL, shm_fd, 1);
//       exit(EXIT_FAILURE);
//   }
//
// Notes:
//   - Always call ftruncate() before mmap() to ensure the size of the shared memory object is set correctly.
//   - The new space in the file or shared memory object is uninitialized after a size increase.
//   - For shared memory, ftruncate() allocates virtual memory but does not write to it directly.
//
// Search ftruncate(2) for more information about file descriptors and memory management.
//--------------------------------------------------------------------------------

#define SHM_NAME "/my_shared_memory"
#define BUFFER_SIZE 1024

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

void child_process(void* ptr, int shm_fd) {
  int num;
  (void)printf("Enter number of elements: ");
  if (scanf("%d", &num) != 1) {
    (void)fprintf(stderr, "Invalid input.\n");
    cleanup_shm(ptr, shm_fd, 0);
    exit(EXIT_FAILURE);
  }

  if (num <= 0 || num > (int)(BUFFER_SIZE / sizeof(int)) - 1) {
    (void)fprintf(stderr, "0 < num < %d\n", (int)(BUFFER_SIZE / sizeof(int)));
    cleanup_shm(ptr, shm_fd, 0);
    exit(EXIT_FAILURE);
  }

  int* shm_data = (int*)ptr;
  shm_data[0]   = num;

  (void)printf("Enter %d numbers: ", num);
  for (int i = 0; i < num; i++) {
    if (scanf("%d", &shm_data[i + 1]) != 1) {
      (void)fprintf(stderr, "Invalid input.\n");
      cleanup_shm(ptr, shm_fd, 0);
      exit(EXIT_FAILURE);
    }
  }

  // Child does not unlink the shared memory
  cleanup_shm(ptr, shm_fd, 0);
}

void parent_process(void* ptr, int shm_fd) {
  // Wait for child to complete
  if (wait(NULL) != 0) {
    perror("wait");
    cleanup_shm(ptr, shm_fd, 1);
    exit(EXIT_FAILURE);
  }

  int* shm_data = (int*)ptr;
  int num       = shm_data[0];

  if (num <= 0 || num > (int)(BUFFER_SIZE / sizeof(int)) - 1) {
    (void)fprintf(stderr, "Invalid number of elements in shared memory: %d\n", num);
    cleanup_shm(ptr, shm_fd, 1);
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < num; i++) {
    (void)printf("%d ", shm_data[i + 1]);
  }
  (void)putchar('\n');

  // Parent always unlinks the shared memory
  cleanup_shm(ptr, shm_fd, 1);
}

int main() {
  // Create or open shared memory
  int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  if (shm_fd == -1) {
    handle_error("shm_open");
  }

  // Resize shared memory
  if (ftruncate(shm_fd, BUFFER_SIZE) == -1) {
    perror("ftruncate");
    cleanup_shm(NULL, shm_fd, 1);
    exit(EXIT_FAILURE);
  }

  // Map shared memory into process address space
  void* ptr = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
  check_pointer(ptr, "mmap");

  pid_t pid = fork();
  if (pid == -1) {
    perror("fork");
    cleanup_shm(ptr, shm_fd, 1);
    exit(EXIT_FAILURE);
  }

  if (pid == 0) {
    // Child process writes to shared memory
    child_process(ptr, shm_fd);
  } else {
    // Parent process reads from shared memory
    parent_process(ptr, shm_fd);
  }
}