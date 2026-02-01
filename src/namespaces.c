/*
uid/gid mapping + unshare helpers
*/

#include "namespaces.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>

int namespaces_setup_userns_maps(pid_t child_pid, uid_t uid, gid_t gid) {

  TODO_PANIC("namespaces_setup_userns_maps not implemented");
  // write deny to /proc/<pid>/setgroups (ignore ENOENT)

  // write "0 <uid> 1\n" to /proc/<pid>/uid_map

  // write "0 <gid> 1\n" to /proc/<pid>/gid_map
    return 0;
}