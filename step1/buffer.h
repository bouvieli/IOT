#include <stdbool.h>
#include <stdint.h>
#define MAX_CHARS 512



bool ring_is_full();
bool ring_is_empty();
void ring_push(char value);
char ring_get();

