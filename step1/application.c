#include "application.h"
#include "shell.h"
#include "uart.h"


#include <stdbool.h>
#include <stdint.h>
#define MAX_CHARS 512

struct cookie{
    uint32_t uartno;
    char buffer[MAX_CHARS];
    uint32_t head;
    uint32_t tail;
    bool processing;
};
struct cookie cookie;

void app_start() {
    cookie.uartno = UART0;
    cookie.head = 0;
    cookie.tail = 0;
    cookie.processing = false;

  uart_init(UART0, read_listener, write_listener, &cookie);

}


// recupère les données stoquées par l'application , regarde si peut les envoyer au buffer d'écriture

void app_write(struct cookie *cook) {
    while (cook->tail < cook->head) {
        uint32_t c = cook->buffer[cook->tail++];
        //if (! write_on_ring(cook->uartno, c)) {
         //   return;
       // }
        if (c == '\n') {
            interpret(cook->buffer, cook->head);
            cook->head = 0;
            cook->tail = 0;
            cook->processing = false;
        }
    }
}
  

  
  



// qd l'application initialise l'uart elle donne un cookie qui est l'adresse avec laquelle la boucle principale de notre machine devra 
//appeler le listener de lecture et d'écriture de l'application
// afin que cette dernière sache qu'elle peut commencer à traiter les données

// qd la machine previens l'application qu'elle a des données à lire
// celle ci lit les données et les met dans un buffer avant de les traiter
void read_listener(void *addr) {
    struct cookie *cook = (struct cookie *)addr;
    char c;
  while (read_on_ring(cook->uartno, &c)&&(!cook->processing)) {
    cook->buffer[cook->head++] = (char)c;
    cook->processing = (c=='\n');
    //app_write(cook);
    

    
  }
  interpret(cook->buffer, cook->head);
    cook->head = 0;
    cook->tail = 0;
    cook->processing = false;

}

void write_listener(void *addr){
    struct cookie *cook = (struct cookie *)addr;
    app_write(cook);
   
}

