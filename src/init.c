/*


fork/exec + (simple) wait loop
*/

#include "init.h"
#include "util.h"

#include <errno.h> // strerror, errno
#include <stdio.h> // fprintf, stderr
#include <string.h> // strerror
#include <sys/wait.h> // waitpid, WIFEXITED, WEXITSTATUS, etc..
#include <unistd.h> // fork, execvp, _exit, pid_t

int init_run(char *const argv[]) {

  if (!argv || !argv[0]) {
    fprintf(stderr, "minijfc: error: init run called with empty arguments\n");
    return -1;
  }

  pid_t pid = fork();

  if (pid < 0) {
    fprintf(stderr, "minijfc: error: fork failed: %s\n", strerror(errno));
    return -1;
  }

  // Child process
  if (pid == 0) {
    execvp(argv[0], argv); // (file, args), uses current environment
    fprintf(stderr, "minijfc: error: execvp(\"%s\") failed: %s\n", argv[0], strerror(errno));
    _exit(127); // exec error exit code
  }

  // Parent process

  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    fprintf(stderr, "minijfc: error: waitpid failed: %s\n", strerror(errno));
    return -1;
  }

  if (WIFEXITED(status)) return WEXITSTATUS(status);
  if (WIFSIGNALED(status)) return 128 + WTERMSIG(status);
  return 1; // should not reach here
}