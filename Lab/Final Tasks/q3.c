#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

int main(int argc, char* argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: ./q3 <source> <destination>\n");
    exit(EXIT_FAILURE);
  }

  const char* src_name = argv[1];
  const char* dst_name = argv[2];

  int src_fd = open(src_name, O_RDONLY);
  if (src_fd == -1) {
    perror("open source");
    exit(EXIT_FAILURE);
  }

  int dst_fd = open(dst_name, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (dst_fd == -1) {
    perror("open destination");
    close(src_fd);
    exit(EXIT_FAILURE);
  }

  char buffer[BUFFER_SIZE];
  ssize_t bytes_read;
  while ((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
    ssize_t bytes_written = write(dst_fd, buffer, bytes_read);
    if (bytes_written != bytes_read) {
      perror("write");
      close(src_fd);
      close(dst_fd);
      exit(EXIT_FAILURE);
    }
  }

  if (bytes_read == -1) {
    perror("read");
  }

  close(src_fd);
  close(dst_fd);
}