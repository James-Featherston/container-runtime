#ifndef CONTAINER_H
#define CONTAINER_H
#include <stdint.h>

struct container_config {
    char *rootfs; // path to root filesystem
    char *hostname; // hostname for the container

    char **argv;        // command to run (NULL-terminated)
    int argc;

    long long mem_limit_bytes;   // -1 if not specified

    int cpu_limit_set; // 0 if not specified
    int cpu_quota_is_max;
    uint64_t cpu_quota_us;
    uint64_t cpu_period_us;


    int flags; // bitmask for various flags
};

int container_run(const struct container_config *ccfg);

#endif // CONTAINER_H