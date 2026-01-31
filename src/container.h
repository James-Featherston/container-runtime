#ifndef CONTAINER_H
#define CONTAINER_H

struct container_config {
    char *rootfs; // path to root filesystem
    char *hostname; // hostname for the container

    char **argv;        // command to run (NULL-terminated)
    int argc;

    long long mem_limit_bytes;   // -1 if not specified
    double cpu_limit;            // -1 if not specified

    int flags; // bitmask for various flags
};

#endif // CONTAINER_H