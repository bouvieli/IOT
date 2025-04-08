/*
 * Copyright: Olivier Gruber (olivier dot gruber at acm dot org)
 *
 * This program is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <https://www.gnu.org/licenses/>.
 */

#include "main.h"
#include "uart.h"
#include "uart-mmio.h"
#include "isr.h"
#include "isr-mmio.h"
#include "buffer.h"




struct uart {
  uint8_t uartno; // the UART numéro
  void* bar;      // base address register for this UART
};

static
struct uart uarts[NUARTS];

/*char ligne[20];

int ligne_index = 0;
int lignett=0;
int stock_index = 0;
int position = 0;
int max_columns = 19;
*/


static
void uart_init(uint32_t uartno, void* bar) {
  struct uart* uart = &uarts[uartno];
  uart->uartno = uartno;
  uart->bar = bar;
  // no hardware initialization necessary
  // when running on QEMU, the UARTs are
  // already initialized, as long as we
  // do not rely on interrupts.
}

void uarts_init() {
  uart_init(UART0,UART0_BASE_ADDRESS);
  uart_init(UART1,UART1_BASE_ADDRESS);
  uart_init(UART2,UART2_BASE_ADDRESS);
}

void uart_enable(uint32_t uartno) {
  struct uart*uart = &uarts[uartno];
  // activer l'inteeruption de reception
  // en mode IRQ
  *((volatile uint32_t*)(uart->bar + UART_IMSC)) |= 1<<4;
  // desactiver l'interruption de transmission
  // en mode IRQ
  *((volatile uint32_t*)(uart->bar + UART_IMSC)) &= ~(1<<5);

  
}

void uart_disable(uint32_t uartno) {
  struct uart*uart = &uarts[uartno];
  // désactiver l'interruption de reception
  // en mode IRQ
  *((volatile uint32_t*)(uart->bar + UART_IMSC)) &= ~(1<<4);
  // désactiver l'interruption de transmission
  // en mode IRQ
  *((volatile uint32_t*)(uart->bar + UART_IMSC)) &= ~(1<<5);
}

void uart_receive(uint32_t uartno, char *pt) {

  struct uart* uart = &uarts[uartno];
  // tant que le bit 4 du registre de flag est a 1 le fifo de reception est vide il faut attendre
  //pour pouvoir lire des données de l'uart
  while(mmio_read32(uart->bar,UART_FR ) & 1<<4){} 

  *pt = (char)mmio_read32(uart->bar,UART_DR );


}

/**
 * Sends a character through the given uart, this is a blocking call
 * until the character has been sent.
 */
void uart_send(uint32_t uartno, char s) {
  struct uart* uart = &uarts[uartno];
 
  
 // tant que le bit 5 du registre de flag est a 1 le fifo de trans est pleins il faut attendre
 //pour pouvoir envoyer des données vers l'uart
  while(mmio_read32(uart->bar,UART_FR)& 1<<5){} 
  mmio_write32(uart->bar,UART_DR,s);
  
  



  
}

/**
 * This is a wrapper function, provided for simplicity,
 * it sends a C string through the given uart.
 */

 void move_cursor(int x, int y) {
  char pos[16];
  int i = 0;
  pos[i ++] = '\033';
  pos[i ++] = '[';
  if (x < 10) {
    pos[i ++] = '0' + x;
  } else {
    pos[i ++] = '0' + (x / 10);
    pos[i ++] = '0' + (x % 10);
  }
  pos[i ++] = ';';
  if (y < 10) {
    pos[i ++] = '0' + y;
  } else {
    pos[i ++] = '0' + (y / 10);
    pos[i ++] = '0' + (y % 10);
  }
  pos[i ++] = 'H';
  pos[i ++] = '\0';

 
  
  uart_send_string(UART0, pos);
}

void uart_send_string(uint32_t uartno, const char *s) {
  while (*s != '\0') {
    uart_send(uartno, *s);
    s++;
  }
}


// fonction callback de l'interruption 
void uart_isr(uint32_t irq, void* cookie) {
  char c;
  uint32_t status;
 // static int visible = 1;
  //char buffer[20];
  //int offset = 0;

  // on lit le registre d'état des interruptions
  if (irq == UART0_IRQ) {
    status = mmio_read32(UART0_BASE_ADDRESS, UART_MIS);
    // si l'interruption de reception est active
    if (status & (1<<4)) {
    // on lit le caractère reçu
    // tant que le fifo n'est pas vide
      while(!(mmio_read32(UART0_BASE_ADDRESS, UART_FR) & (1<<4))) {
        uart_receive(UART0, &c);
        ring_push(c);
        // verification que push fonctionne bien 
      }
      
    }
  }

       
      
    
       
  else if (irq == UART1_IRQ) {
    status = mmio_read32(UART1_BASE_ADDRESS, UART_MIS);
    if (status & (1<<4)) {
      // on lit le caractère reçu
      while(!(mmio_read32(UART1_BASE_ADDRESS, UART_FR) & (1<<4))) {
        
        uart_receive(UART1, &c);
      //  uart_send_string(UART1, "Received: ");
        uart_send(UART1, c);

      }
     
    
    }
    
  }
  else if (irq == UART2_IRQ) {
    status = mmio_read32(UART2_BASE_ADDRESS, UART_MIS);
    if (status & (1<<4)) {
      while(!(mmio_read32(UART2_BASE_ADDRESS, UART_FR) & (1<<4))) {

      // on lit le caractère reçu
        uart_receive(UART2, &c);
      // et on l'affiche sur la sortie standard
        //uart_send_string(UART2, "Received: ");
        uart_send(UART2, c);
      
      }
    mmio_write32(UART2_BASE_ADDRESS, UART_ICR, (1<<4));
    //uart_send_string(UART2, "\n");
    }

  }
  else {
    return; // on ne gère pas d'autres interruptions
  }
  
 

}

