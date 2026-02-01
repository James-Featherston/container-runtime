
#pragma once
#include <sys/types.h>


int namespaces_setup_userns_maps(pid_t child_pid, uid_t uid, gid_t gid);