#include "buffer.h"
uint32_t tail = 0;
uint32_t head = 0;
char buffer[MAX_CHARS];

bool ring_is_full(){
    int next = (head + 1) % MAX_CHARS;
    return (next == tail);
}
bool ring_is_empty(){
    return (head == tail);
}
void ring_push(char value){
    if (!ring_is_full()){
        buffer[head] = value;
        head = (head + 1) % MAX_CHARS;
    }
}

char ring_get(){
    if (!ring_is_empty()){
        char value = buffer[tail];
        tail = (tail + 1) % MAX_CHARS;
        return value;
    }
    return 0;
}

