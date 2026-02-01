#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

int write_all(int fd, const char *buf, size_t len);
int write_file(const char *path, const char *s);
void todo_panic(const char *msg, const char *file, int line);

#define TODO_PANIC(msg) todo_panic((msg), __FILE__, __LINE__)

