#pragma once
#include <stdio.h>
#include <stdlib.h>

void todo_panic(const char *msg, const char *file, int line);

#define TODO_PANIC(msg) todo_panic((msg), __FILE__, __LINE__)
