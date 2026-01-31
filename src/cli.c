/*
CLI parsing functions
Parse arguments into a single config struct.
*/

#include "cli.h"
#include <stdio.h>
#include <string.h>

// Function definitions
void cfg_init_defaults(struct container_config *cfg);
void cli_print_usage(FILE *out);
int cli_parse_run(int argc, char **argv, struct container_config *cfg);


int cli_parse(int argc, char **argv, struct cli_result *cli_res) {
  int r;
  cfg_init_defaults(&cli_res->ccfg);

  // CASE 0: missing subcommand
  if (argc < 2) {
    fprintf(stderr, "minijfc: missing subcommand\n");
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
    // TODO: Validate presence of ID
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
  printf("PRINT USAGE INFO HERE\n");
}

// TODO: Add parse helpers

int cli_parse_run(int argc, char **argv, struct container_config *cfg) {
    // TODO: Implement parse run
    // parse run-specific options
    // parse options
    // find '--'
    // fill cfg->argv
    // validate

    return 0;
}