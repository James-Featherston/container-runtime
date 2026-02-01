/*
File system isolation logic.
*/
#define _GNU_SOURCE
#include "rootfs.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

int rootfs_enter(const char *rootfs_path) {
  if (!rootfs_path) {
    fprintf(stderr, "minijfc: error: rootfs path is NULL\n");
    return -1;
  }

  // Change root directory to rootfs_path
  if (chroot(rootfs_path) == -1) {
    perror("chroot");
    return -1;
  }

  // Change working directory to new root
  if (chdir("/") == -1) {
    perror("chdir");
    return -1;
  }
  return 0;
}

int rootfs_mount_proc(void) {
  // Create /proc directory if it doesn't exist 0555 = (r-x r-x r-x)
  if (mkdir("/proc", 0555) != 0) {
    if (errno != EEXIST) { // Ignore error if /proc already exists
      fprintf(stderr,
      "minilxc: error: mkdir(/proc) failed: %s\n",
      strerror(errno));
      return -1;
    }
  }

  // Mount proc filesystem at /proc
  if (mount("proc", "/proc", "proc", 0, NULL) == -1) {
    if (errno == EBUSY) {
      // already mounted (rare in your setup, but harmless)
      return 0;
    }
    fprintf(stderr, "minilxc: error: mount proc on /proc failed: %s\n", strerror(errno));
    return -1;
  }
  return 0;
}