/* 
This is the main runtime for the container management system.
It handles starting, stopping, and managing containers based on user commands.
*/

#define _GNU_SOURCE // Allows for linux specific features (not POSIX compliant)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sched.h> // Scheduler functions clone, unshare
#include <unistd.h> // Unix standard functions pipe, write, close, getuid, getgid
#include <sys/types.h> // pid_t, uid_t, gid_t
#include <signal.h> // Signals
#include <sys/wait.h> // waitpid
#include <sys/mount.h> // mount, umount


#include "container.h"
#include "namespaces.h"
#include "rootfs.h"
#include "init.h"

#define STACK_SIZE (1024 * 1024)

struct child_args {
    const struct container_config *ccfg;
    int sync_fd;  // read end
};


static int container_child_main(void *arg);

int container_run(const struct container_config *ccfg) {
  // Create a pipe for synchronization
  int sync_pipe[2]; // [0] = read end, [1] = write end
  if (pipe(sync_pipe) == -1) {
    perror("pipe");
    return 1;
  }

  // allocate child stack
  void *child_stack = malloc(STACK_SIZE);
  if (child_stack == NULL) {
    close(sync_pipe[0]);
    close(sync_pipe[1]);
    perror("malloc");
    return 1;
  } 
  void *stack_top = (char *)child_stack + STACK_SIZE;

  struct child_args args = {
    .ccfg = ccfg,
    .sync_fd = sync_pipe[0], // Pass read end to child
  };
  
  int flags = CLONE_NEWUSER | CLONE_NEWPID | SIGCHLD; // New user, PID namespaces, signal on exit
  pid_t child_pid = clone(container_child_main, stack_top, flags, &args);
  if (child_pid == -1) {
    perror("clone");
    free(child_stack);
    close(sync_pipe[0]);
    close(sync_pipe[1]);
    return 1;
  }
  // parent closes read end, writes uid/gid maps via namespaces_setup_userns(child_pid)
  close(sync_pipe[0]);

  uid_t uid = getuid();
  gid_t gid = getgid();
  if (namespaces_setup_userns_maps(child_pid, uid, gid) != 0) {
    kill(child_pid, SIGKILL);
    close(sync_pipe[1]);
    free(child_stack);
    return 1;
  }

  // parent writes one byte to pipe to unblock child
  char buf = 'x';
  if( write(sync_pipe[1], &buf, 1) != 1) {
    perror("write to sync pipe");
    kill(child_pid, SIGKILL);
    close(sync_pipe[1]);
    free(child_stack);
    return 1;
  }
  close(sync_pipe[1]);

  // parent waits for child exit
  int status;
  if (waitpid(child_pid, &status, 0) == -1) {
    perror("waitpid");
    free(child_stack);
    return 1;
  }

  free(child_stack);

  if (WIFEXITED(status)) return WEXITSTATUS(status); // return child's exit code
  if (WIFSIGNALED(status)) return 128 + WTERMSIG(status); // return signal code
  return 1; // should not reach here
}

static int container_child_main(void *arg) {
  struct child_args *cargs = (struct child_args *)arg;
  const struct container_config *ccfg = cargs->ccfg;

  // wait on pipe for parent to finish uid/gid maps
  char buf;
  if (read(cargs->sync_fd, &buf, 1) != 1) {
    perror("read from sync pipe");
    return 1;
  }
  close(cargs->sync_fd);

  // Setup namespaces: mount, uts
  if (unshare(CLONE_NEWNS | CLONE_NEWUTS) == -1) {
    perror("unshare");
    return 1;
  }

  // Makes it such that mounts do not propagate to host
  // mount() attaches a file system to the processes directory tree
  // Here we remount the root (/) as private
  if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL) == -1) {
    perror("mount MS_PRIVATE");
    return 1;
  }

  // Set hostname
  const char *hostname = ccfg->hostname ? ccfg->hostname : "minijfc-container";
  if (sethostname(hostname, strlen(hostname)) < 0) {
    perror("sethostname");
    return 1;
  }
  // Enter root filesystem
  if (rootfs_bind_devnull(ccfg->rootfs) != 0) return 1;
  if (rootfs_enter(ccfg->rootfs) != 0) return 1;
  if (rootfs_mount_proc() != 0) return 1;

  init_run(ccfg->argv);
  return 0;
}