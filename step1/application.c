#include "application.h"
#include "buffer.h"
#include "shell.h"
char line[MAX_CHARS];
uint32_t nchars = 0;
void read_listener(void *addr) {
  while (!ring_is_empty(addr)) {
    char c = ring_get(addr);
    line[nchars] = c;
    nchars++;
    if (c == '\n') {
      interpret(line, nchars);
      nchars = 0;
    }
    
  }
}

void write_listener(void *addr){}