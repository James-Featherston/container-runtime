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

static volatile sig_atomic_t pending_signal = 0;

static void on_signal(int sig) {
  pending_signal = sig;
}

static int install_handlers() {

  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = on_signal; // Handler for signals
  sigemptyset(&sa.sa_mask); // No additional signals blocked during handler
  sa.sa_flags = 0; // No flags

  if (sigaction(SIGINT, &sa, NULL) < 0) return -1; // SIGINT handler
  if (sigaction(SIGTERM, &sa, NULL) < 0) return -1; // Terminate handler
  if (sigaction(SIGHUP, &sa, NULL) < 0) return -1; // Signals child processes to terminate
  return 0;
}

int init_run(char *const argv[]) {
  if (install_handlers() < 0) {
    perror("sigaction");
    return 1;
  }
  if (!argv || !argv[0]) {
    fprintf(stderr, "minijfc: error: init run called with empty arguments\n");
    return 1;
  }

  pid_t orig_pgrp = -1; // original process group unknonw

  if (isatty(STDIN_FILENO)) {
      orig_pgrp = tcgetpgrp(STDIN_FILENO); // remember the foreground process group
  }

  pid_t main_pid = fork();

  if (main_pid < 0) {
    fprintf(stderr, "minijfc: error: fork failed: %s\n", strerror(errno));
    return -1;
  }

  // Main/child process
  if (main_pid == 0) {
    setpgid(0, 0);
    execvp(argv[0], argv); // (file, args), uses current environment
    fprintf(stderr, "minijfc: error: execvp(\"%s\") failed: %s\n", argv[0], strerror(errno));
    _exit(127); // exec error exit code
  }

  setpgid(main_pid, main_pid); // Ensure main process is in its own process group

  if (orig_pgrp != -1) { // We have a controlling terminal
    if (tcsetpgrp(STDIN_FILENO, main_pid) < 0) { // Give terminal control to main process group
      fprintf(stderr, "minijfc: warning: tcsetpgrp failed: %s\n", strerror(errno));
    }
  }

  int main_status = 0;
  int have_main_status = 0;


  // Wait loop
  while (1) {
    // Have signal to forward
    if (pending_signal != 0) {
      int sig = pending_signal;
      pending_signal = 0;
      // Send signal to group
      kill(-main_pid, sig);
    } 
    int status;
    pid_t pid = waitpid(-1, &status, 0); // Wait for a child process

    if (pid < 0) {
      if (errno == EINTR) continue; // Interrupted by signal
      if (errno == ECHILD) break; // No more child processes
      perror("waitpid");
      return 1;
    }

    if (pid == main_pid) {
      main_status = status;
      have_main_status = 1;
      break;
    }
  }
  kill(-main_pid, SIGTERM); // Ensure all children are terminated

  if (!have_main_status) {
    fprintf(stderr, "minijfc: error: main process exited but no status recorded\n");
    return 1;
  }
  if (WIFEXITED(main_status)) return WEXITSTATUS(main_status);
  if (WIFSIGNALED(main_status)) return 128 + WTERMSIG(main_status);
  return 1; // should not reach here
}