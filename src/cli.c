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
#include <stdint.h> //uint types
#include <ctype.h> // tolower()
#include <errno.h>

// Function definitions
void cfg_init_defaults(struct container_config *cfg);
void cli_print_usage(FILE *out);
int cli_parse_run(int argc, char **argv, struct container_config *ccfg);
static int validate_path(const char *path);
static int parse_bytes(const char *s, long long *bytes);
static int validate_cpu_limit_args(const char *quota_s, const char *period_s, int *quota_is_max, uint64_t *quota, uint64_t *period);

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
  ccfg->cpu_limit_set = 0;
  ccfg->flags = 0;
}

void cli_print_usage(FILE *out) {
  fprintf(out, "Options:\n");
  fprintf(out, "\t minijfc run --rootfs PATH [--hostname NAME] [--mem SIZE] [--cpu  QUOTA PERIOD] -- CMD\n");
  fprintf(out, "\t minijfc ps\n");
  fprintf(out, "\t minijfc kill ID\n");
}

void cli_print_run_usage(FILE * out) {
  fprintf(out, "Usage: minijfc run --rootfs PATH [--hostname NAME] [--mem SIZE] [--cpu QUOTA PERIOD] -- CMD\n");
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
      long long bytes = 0;
      if (parse_bytes(argv[1], &bytes) < 0 || bytes == 0) {
        fprintf(stderr, "minijfc: error: invalid --mem value\n");
        return 2;
      } 
      ccfg->mem_limit_bytes = bytes;
    } else if (strcmp(argv[0], "--cpu") == 0) {
      if (argc < 2) {
        fprintf(stderr, "minijfc: error: missing argument for --cpu\n");
        cli_print_run_usage(stderr);
        return 2;
      }

      int quota_is_max = 0;
      uint64_t quota = 0, period = 0;
      if (validate_cpu_limit_args(argv[1], argv[2], &quota_is_max, &quota, &period) != 0) {
        fprintf(stderr, "minijfc: error: invalid --cpu value\n");
        return 2;
      }
      ccfg->cpu_limit_set = 1;
      ccfg->cpu_quota_is_max = (quota_is_max != 0);
      ccfg->cpu_quota_us = quota;
      ccfg->cpu_period_us = period;
      argc -= 1; // Use one additional arg than other argumemts would
      argv += 1;
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

static int parse_bytes(const char *s, long long *bytes) {
  if (!s || !*s) return -1;

  errno = 0;
  char *end = NULL;
  unsigned long long base = strtoull(s, &end, 10); // First non integer stored in end, base 10
  if (errno != 0 || end == s) return -1;

  uint64_t mult = 1;
  if (*end != '\0') {
    char c = (char)tolower((unsigned char)*end);
    
    switch (c) {
      case 'k': mult = 1024ULL; break;
      case 'm': mult = 1024ULL * 1024ULL; break;
      case 'g': mult = 1024ULL * 1024ULL * 1024ULL; break;
      case 't': mult = 1024ULL * 1024ULL * 1024ULL * 1024ULL; break;
      default: return -1;
    }
  }

  if (base > 0 && (uint64_t)base > UINT64_MAX / mult) return -1;

  *bytes = (uint64_t)base * mult;
  return 0;
}

static int parse_u64(const char *s, uint64_t *out) {
  if (!s || !*s) return -1;
  errno = 0;
  char *end = NULL;
  unsigned long long v = strtoull(s, &end, 10);
  if (errno != 0 || end == s || *end != '\0') return -1;
  *out = (uint64_t)v;
  return 0;
}

static int validate_cpu_limit_args(
  const char *quota_s,
  const char *period_s,
  int *quota_is_max,
  uint64_t *quota,
  uint64_t *period
) {
  if (!quota_s || !period_s) return -1;
  if (strcmp(quota_s, "max") == 0 ) {
    *quota_is_max = 1;
    *quota = 0;
  } else {
    *quota_is_max = 0;
    if (parse_u64(quota_s, quota) != 0 || *quota == 0) return -1;
  }
  if (*quota_is_max == 0) {
    if (parse_u64(period_s, period) != 0 || *period == 0) return -1;
  } else {
    *period = 100000; // Default period value for max quota
  }
  return 0;
}