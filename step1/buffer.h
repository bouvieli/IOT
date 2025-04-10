#include <stdbool.h>
#include <stdint.h>
#define MAX_CHARS 512

typedef struct {
    char buffer[MAX_CHARS];
    uint32_t head;
    uint32_t tail;
} ring_buffer_t;

bool ring_is_full(ring_buffer_t *ring);
bool ring_is_empty(ring_buffer_t *ring);
void ring_push(ring_buffer_t *ring,char value);
char ring_get(ring_buffer_t *ring);

