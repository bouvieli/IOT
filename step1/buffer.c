#include "buffer.h"

bool ring_is_full(ring_buffer_t *ring){
    int next = (ring->head + 1) % MAX_CHARS;
    return (next == ring->tail);
}
bool ring_is_empty(ring_buffer_t *ring){
    return (ring->head == ring->tail);
}
void ring_push(ring_buffer_t *ring, char value){
    if (!ring_is_full(ring)){
        ring->buffer[ring->head] = value;
        ring->head = (ring->head + 1) % MAX_CHARS;
        
    }
}

char ring_get(ring_buffer_t *ring){
    if (!ring_is_empty( ring)){
        char value = ring->buffer[ring->tail];
        ring->tail = (ring->tail + 1) % MAX_CHARS;
        return value;
        /*char value = buffer[tail];
        tail = (tail + 1) % MAX_CHARS;
        return value;*/
    }
    return 0;
}

