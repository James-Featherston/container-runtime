/* 
Entry Point into the Program 
Author: James Featherston
*/

#include <stdio.h>
#include <stdlib.h>
#include "cli.h"
#include "container.h"


int main(int argc, char *argv[]){

    int r;
    struct cli_result cli_res;

    // Parse CLI arguments
    if ((r = cli_parse(argc, argv, &cli_res)) != 0) {
        return r;
    }

    switch (cli_res.action) {
        case CLI_ACT_RUN: 
            printf("TODO: Run container\n");
            break;
        case CLI_ACT_PS:
            printf("TODO: List containers\n");
            break;
        case CLI_ACT_KILL:
            printf("TODO: Kill container\n");
            break;
        case CLI_ACT_HELP:
            printf("TODO: Print help\n");
            break;
        default:
            fprintf(stderr, "internal error\n");
            return 1;
    }

    return 0;
}