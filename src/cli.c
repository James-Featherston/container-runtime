/*
CLI parsing functions
Parse arguments into a single config struct.
*/

#include "cli.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h> // For HOST_NAME_MAX
#include <sys/stat.h> // For file system metadata

// Function definitions
void cfg_init_defaults(struct container_config *cfg);
void cli_print_usage(FILE *out);
int cli_parse_run(int argc, char **argv, struct container_config *ccfg);
static int validate_path(const char *path);


int cli_parse(int argc, char **argv, struct cli_result *cli_res) {
  int r;
  cfg_init_defaults(&cli_res->ccfg);

  // CASE 0: missing subcommand
  if (argc < 2) {
    fprintf(stderr, "minijfc: missing subcommand\n");
    cli_print_usage(stderr);
    return 2;
  }

  // CASE 1: run
  if (strcmp(argv[1], "run") == 0) {
    cli_res->action = CLI_ACT_RUN;
    r = cli_parse_run(argc - 2, &argv[2], &cli_res->ccfg);
    return r;
  }

  // CASE 2: ps
  if (strcmp(argv[1], "ps") == 0) {
    cli_res->action = CLI_ACT_PS;
    return 0;
  }

  // CASE 3: kill
  if (strcmp(argv[1], "kill") == 0) {
    cli_res->action = CLI_ACT_KILL;
    if (argc < 3) {
      fprintf(stderr, "minijfc: 'kill' requires container ID\n");
      return 2;
    }
    if (argc > 3) {
      fprintf(stderr, "minijfc: 'kill' takes only one argument\n");
      return 2;
    }
    cli_res->id = argv[2];
    return 0;
  }

  // CASE 4: help
  if (strcmp(argv[1], "help") == 0 ||
      strcmp(argv[1], "--help") == 0 ||
      strcmp(argv[1], "-h") == 0) {
    cli_res->action = CLI_ACT_HELP;
    return 0;
  }

  // CASE 5: unknown subcommand
  fprintf(stderr, "minijfc: unknown subcommand '%s'\n", argv[1]);
  cli_print_usage(stderr);
  return 2;
}

void cfg_init_defaults(struct container_config *ccfg) {
  ccfg->rootfs = NULL;
  ccfg->hostname = NULL;
  ccfg->argv = NULL;
  ccfg->argc = 0;
  ccfg->mem_limit_bytes = -1;
  ccfg->cpu_limit = -1;
  ccfg->flags = 0;
}

void cli_print_usage(FILE *out) {
  fprintf(out, "Options:\n");
  fprintf(out, "\t minijfc run --rootfs PATH [--hostname NAME] [--mem SIZE] [--cpu  FRACTION] -- CMD\n");
  fprintf(out, "\t minijfc ps\n");
  fprintf(out, "\t minijfc kill ID\n");
}

void cli_print_run_usage(FILE * out) {
  fprintf(out, "Usage: minijfc run --rootfs PATH [--hostname NAME] [--mem SIZE] [--cpu FRACTION] -- CMD\n");
}

int cli_parse_run(int argc, char **argv, struct container_config *ccfg) {
  int r;

  // parse run-specific options
  if (argc < 1) {
    fprintf(stderr, "minijfc: 'run' requires arguments\n");
    cli_print_run_usage(stderr);
    return 2;
  }

  if (strcmp(argv[0], "--rootfs") != 0) {
    fprintf(stderr, "minijfc: error: --rootfs is required\n");
    cli_print_run_usage(stderr);
    return 2;
  }

  if (argc < 2) {
    fprintf(stderr, "minijfc: error: missing argument for --rootfs\n");
    cli_print_run_usage(stderr);
    return 2;
  }
  
  if ((r = validate_path(argv[1])) != 0) {
    return r;
  }
  ccfg->rootfs = argv[1];

  argc -= 2;
  argv += 2;

  while (argc > 0) {
    if (strcmp(argv[0], "--hostname") == 0) {
      if (argc < 2) {
        fprintf(stderr, "minijfc: error: missing argument for --hostname\n");
        cli_print_run_usage(stderr);
        return 2;
      }
      if (strlen(argv[1]) >= HOST_NAME_MAX) {
        fprintf(stderr, "minijfc: error: hostname too long (MAX %d characters)\n", HOST_NAME_MAX - 1);
        cli_print_run_usage(stderr);
        return 2;
      }
      ccfg->hostname = argv[1];
    } else if (strcmp(argv[0], "--mem") == 0) {
      if (argc < 2) {
        fprintf(stderr, "minijfc: error: missing argument for --mem\n");
        cli_print_run_usage(stderr);
        return 2;
      }
      // TODO: NEED TO VALIDATE MEMORY SIZE
      ccfg->mem_limit_bytes = atol(argv[1]);
      TODO_PANIC("Memory limit not implemented");
    } else if (strcmp(argv[0], "--cpu") == 0) {
      if (argc < 2) {
        fprintf(stderr, "minijfc: error: missing argument for --cpu\n");
        cli_print_run_usage(stderr);
        return 2;
      }
      // TODO: NEED TO VALIDATE CPU LIMIT
      ccfg->cpu_limit = atof(argv[1]);
      TODO_PANIC("CPU limit not implemented");
    } else if (strcmp(argv[0], "--") == 0) {
      break;
    } else {
      fprintf(stderr, "minijfc: error: unknown option '%s'\n", argv[0]);
      cli_print_run_usage(stderr);
      return 2;
    }
    argc -= 2;
    argv += 2;
  }
  if (argc == 0 || strcmp(argv[0], "--") != 0) {
    fprintf(stderr, "minijfc: error: missing '--' before command\n");
    cli_print_run_usage(stderr);
    return 2;
  }
  argc -= 1;
  argv += 1; 
  if (argc == 0) {
    fprintf(stderr, "minijfc: error: missing command to run\n");
    cli_print_run_usage(stderr);
    return 2;
  }
  ccfg->argv = argv;
  ccfg->argc = argc;

  return 0;
}

static int validate_path(const char *path) {
  struct stat st; // Structure to hold file system metadata

  if (!path) {
      fprintf(stderr, "minijfc: internal error: must pass non-null string to validate_path\n");
      return -1;
    }

  if (stat(path, &st) != 0) { // Stat system call to get file info
    fprintf(stderr, "minijfc: error: cannot access path '%s'\n", path);
    return 2; 
  }

  if (!S_ISDIR(st.st_mode)) { // Check if it's a directory
    fprintf(stderr, "minijfc: error: path '%s' is not a directory\n", path);
    return 2; 
  }

  return 0;
}