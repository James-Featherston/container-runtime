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

int write_all(int fd, const char *buf, size_t len) {
  while (len > 0) {
    ssize_t n = write(fd, buf, len);
    if (n < 0) {
      if (errno == EINTR)
        continue;
      return -1;
  }
    buf += (size_t)n;
    len -= (size_t)n;
  }
  return 0;
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