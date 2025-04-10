#ifndef APPLICATION_H_
#define APPLICATION_H_
struct cookie;
void read_listener(void *addr);
void write_listener(void *addr);
void app_start();
void app_write(struct cookie *cook);
#endif /* APPLICATION_H_ */