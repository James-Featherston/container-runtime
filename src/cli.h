#ifndef _CLI_H
#define _CLI_H

#include "container.h"

typedef enum {
    CLI_ACT_NONE = 0,
    CLI_ACT_RUN,
    CLI_ACT_PS,
    CLI_ACT_KILL,
    CLI_ACT_HELP
} cli_action_t;

struct cli_result {
    cli_action_t action;
    struct container_config ccfg; // used when action == RUN
    char *id;               // used when action == KILL
};

int cli_parse(int argc, char **argv, struct cli_result *cli_res);

#endif