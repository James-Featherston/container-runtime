/*
Namespace setup logic.
*/

#include "namespaces.h"
#include "util.h"

#define _GNU_SOURCE
#include "namespaces.h"

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int namespaces_setup_userns_maps(pid_t child_pid, uid_t uid, gid_t gid) {
  char path[256];
  char map[128];

  // write deny to /proc/<pid>/setgroups (ignore ENOENT)
  snprintf(path, sizeof(path), "/proc/%d/setgroups", child_pid);
  if (write_file(path, "deny\n") != 0) {
    if (errno != ENOENT) {}
  }

  // Maps uid/gid 0 in the container to the real uid/gid outside
  snprintf(path, sizeof(path), "/proc/%d/uid_map", child_pid);
  snprintf(map, sizeof(map), "0 %d 1\n", uid);
  if (write_file(path, map) != 0) {
    fprintf(stderr, "minijfc: error: write uid_map failed: %s\n", strerror(errno));
    return -1;
  }

  snprintf(path, sizeof(path), "/proc/%d/gid_map", child_pid);
  snprintf(map, sizeof(map), "0 %d 1\n", (int)gid);
  if (write_file(path, map) != 0) {
    fprintf(stderr, "minijfc: error: write gid_map failed: %s\n", strerror(errno));
    return -1;
  }

  return 0;
}