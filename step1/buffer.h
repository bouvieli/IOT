#define MAX_CHARS 512
volatile uint32_t tail = 0;
volatile uint32_t head = 0;
volatile uint32_t buffer[MAX_CHARS];

bool_t ring_is_full();
bool_t ring_is_empty();
void ring_push(uint32_t value);
uint32_t ring_get();
