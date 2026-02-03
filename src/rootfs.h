#ifndef ROOTFS_H
#define ROOTFS_H

int rootfs_enter(const char *rootfs_path);
int rootfs_mount_proc(void);
int rootfs_bind_devnull(const char *rootfs_path);

#endif