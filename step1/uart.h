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

#ifndef UART_H_
#define UART_H_

/*
 * Defines the number of available UARTs
 * and their respective numéro.
 */
#define NUARTS 3
#define UART0 0
#define UART1 1
#define UART2 2
#include <stdint.h>
#include "buffer.h"

struct uart {
    uint32_t uartno; // the UART numéro
    void* bar;      // base address register for this UART
    ring_buffer_t ring_lecture; // buffer de reception
    ring_buffer_t ring_ecriture; // buffer d'ecriture
    void (*rl)( void*); // read listener
    void (*we)( void*); // write listener
  };
  
  extern
  struct uart uarts[NUARTS];

/*
 * Receives a one-byte character, which is compatible
 * with ASCII encoding. This function blocks, spinning,
 * until there is one character available, that is,
 * there is at least one character available in the
 * UART RX FIFO queue.
 */
void uart_receive(uint32_t uartno, char *pt);

/**
 * Write a one-byte character through the given uart,
 * this is a blocking call. This is compatible with a
 * basic ASCII encoding. This function blocks, spinning,
 * until there is room in the UART TX FIFO queue to send
 * the character.
 */
void uart_send(uint32_t uartno, char s);

/**
 * This is a wrapper function, provided for simplicity,
 * it sends the given C string through the given uart,
 * using the function uart_send.
 */
void uart_send_string(uint32_t uartno, const char *s);

/*
 * Global initialization for all the UARTs
 */
void uarts_init();

/*
 * Enables the UART, identified by the given numéro.
 * Nothing to do on QEMU until we use interrupts...
 * You can enable or disable the individual interrupts by changing
 */
void uart_enable(uint32_t uartno);

/*
 * Disables the UART, identified by the given numéro.
 * Nothing to do on QEMU until we use interrupts...
 */
void uart_disable(uint32_t uartno);

void uart_isr(uint32_t irq, void* cookie);

struct uart* get_uart(uint32_t uartno) ;

#endif /* UART_H_ */
