/*
File system isolation logic.
*/
#define _GNU_SOURCE
#include "rootfs.h"
#include "util.h"

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mount.h> 
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

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
      "minijfc: error: mkdir(/proc) failed: %s\n",
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
    fprintf(stderr, "minijfc: error: mount proc on /proc failed: %s\n", strerror(errno));
    return -1;
  }
  return 0;
}


// Create a directory if it does not already exist
static int mkdir_p_one(const char *path, mode_t mode) {
  if (mkdir(path, mode) == 0) return 0;
  if (errno == EEXIST) return 0;
  return -1;
}

// Bind the rootfs to devnull
int rootfs_bind_devnull(const char *rootfs_path) {
  if (!rootfs_path) {
    fprintf(stderr, "minilxc: error: rootfs path is NULL\n");
    return -1;
  }

  // Build path <rootfs>/dev and <rootfs>/del/null
  char dev_dir[PATH_MAX];
  char dev_null[PATH_MAX];
  snprintf(dev_dir, sizeof(dev_dir), "%s/dev", rootfs_path);
  snprintf(dev_null, sizeof(dev_null), "%s/dev/null", rootfs_path);

  // Make <rootfs>/dev if it does not exist
  if (mkdir_p_one(dev_dir, 0755) != 0) {
    fprintf(stderr, "minijfc: error: mkdir(\"%s\") failed: %s\n",
            dev_dir, strerror(errno));
    return -1;
  }

  // Ensure mountpoint file <rootfs>/dev/null exists, else create it
  int fd = open(dev_null, O_CREAT | O_RDWR, 0666);
  if (fd < 0) {
    fprintf(stderr, "minijfc: error: open(\"%s\") failed: %s\n",
            dev_null, strerror(errno));
    return -1;
  }
  close(fd);

  // Bind mount host /dev/null onto <rootfs>/dev/null
  if (mount("/dev/null", dev_null, NULL, MS_BIND, NULL) != 0) {
    fprintf(stderr, "minijfc: error: bind-mount /dev/null -> %s failed: %s\n",
            dev_null, strerror(errno));
    return -1;
  }

  return 0;
}