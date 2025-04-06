#include buffer.h

bool_t ring_is_full(){
    int next = (head + 1) % MAX_CHARS;
    return (next == tail);
}
bool_t ring_is_empty(){
    return (head == tail);
}
void ring_push(uint32_t value){
    if (!ring_is_full()){
        buffer[head] = value;
        head = (head + 1) % MAX_CHARS;
    }
}

uint32_t ring_get(){
    if (!ring_is_empty()){
        uint32_t value = buffer[tail];
        tail = (tail + 1) % MAX_CHARS;
        return value;
    }
    return 0;
}