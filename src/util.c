#include "util.h"

void todo_panic(const char *msg, const char *file, int line) {
  fprintf(stderr, "minijfc: TODO: %s (%s:%d)\n", msg, file, line);
  abort();
}