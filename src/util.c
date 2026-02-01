#define _GNU_SOURCE // For O_CLOEXEC
#include "util.h"

#include <errno.h> // For errno
#include <fcntl.h> // For open
#include <string.h>
#include <unistd.h> // For write, close

void todo_panic(const char *msg, const char *file, int line) {
  fprintf(stderr, "minijfc: TODO: %s (%s:%d)\n", msg, file, line);
  abort();
}

void write_all(int fd, const char *buf, size_t len) {
  size_t total_written = 0;
  while (total_written < len) {
    ssize_t n = write(fd, buf + total_written, len - total_written);
    if (n == -1) {
      perror("write");
      exit(1);
    }
    total_written += n;
  }
}

int write_file(const char *path, const char *s) {
    int fd = open(path, O_WRONLY | O_CLOEXEC);
    if (fd < 0)
        return -1;

    int rc = write_all(fd, s, strlen(s));
    int saved_errno = errno;

    close(fd);
    errno = saved_errno;
    return rc;
}