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
#include <stdio.h>


struct uart {
  uint8_t uartno; // the UART numéro
  void* bar;      // base address register for this UART
};

static
struct uart uarts[NUARTS];

char ligne[20];

int ligne_index = 0;
int lignett=0;
int stock_index = 0;
int position = 0;
int max_columns = 19;



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
  static int visible = 1;

  // on lit le registre d'état des interruptions
  if (irq == UART0_IRQ) {
    status = mmio_read32(UART0_BASE_ADDRESS, UART_MIS);
    // si l'interruption de reception est active
    if (status & (1<<4)) {
    // on lit le caractère reçu
    // tant que le fifo n'est pas vide
      //while(!(mmio_read32(UART0_BASE_ADDRESS, UART_FR) & (1<<4))) {
        uart_receive(UART0, &c);
       // si j'appuis sur la fleche entrée 
         if (c== 0x0D || c == '\n') {
          if (ligne_index == lignett){
            
            lignett++;
          }
          
            ligne_index++;
          
          uart_send(UART0, '\r');
          uart_send(UART0, '\n');
          uart_send_string(UART0, ">");
          // on vide le buffer de ligne et remet la position a 0
          for (int i = 0; i < stock_index; i++) {
           
            ligne[i] = ' ';
           
          }

          
          position = 0;
          stock_index = 0;

        }
      
    
        // si j'efface le caractère précédent
        // j'empeche de supprimer si je suis pas sur la ligne sur laquelle on était actuellement en train d'écrire
        else if ( c == 0x7F && ligne_index == lignett) {
         

            // j'efface le caractère dans le buffer
            for (int i = position; i <stock_index ; i++) {
              ligne[i-1] = ligne[i];
            }
            ligne[stock_index-1] = ' ';
            
            
            // réecrire la ligne 
            uart_send(UART0, '\b');
            int K = 0;
            for (int i = position-1; i < stock_index; i++) {
              uart_send(UART0, ligne[i]);
              K++;
            }
            stock_index--;
            while (K>0){
              uart_send(UART0, '\b');
              K--;
            }
            
            position--;

         
          
        }
        // rendre le curseur invisible/visible Ctrl+G
        else if (c== 0x07){
          if (visible == 1){
            visible = 0;
            // rendre le curseur invisible
            uart_send_string(UART0, "\033[?25l");
          }
          else {
            visible = 1;
            // rendre le curseur visible
            uart_send_string(UART0, "\033[?25h");
          }
          
        }
        else if (c==0x09 && ligne_index == lignett){
          // si j'appuis sur la fleche tab
          // on affiche 4 espaces
          
          // on decale tout ce qui est a droite du curseur de 4
          // TODO: quand curseur avant caractere dessent avec mes l'ecrase
          // si dessus le laisse en dernier avant d'aller à la ligne suivante
          if (stock_index + 4 >max_columns){
            int diff = max_columns - stock_index;
            
          
            // sauter à la ligne, afficher prompt et caractere restants
            uart_send(UART0, '\r');
            uart_send(UART0, '\n');
            uart_send_string(UART0, ">");
            int k = 4;
            for (int i=0; i<4; i++){
              
              uart_send(UART0, ' ');
            }
            for (int m = position ; m < stock_index; m++){
              uart_send(UART0, ligne[m]);
              k ++;
               
             }


            lignett++;
            ligne_index++;

            for (int m = 0; m < 4; m++){
              ligne[m] = ' ';
              uart_send(UART0, '\b');
            }

            for (int i= 0; i< k-4; i++){
              ligne[i+4] = ligne[position+i];
              uart_send(UART0, '\b');
              
            }
            
            
            
            stock_index = k;
            position = 0;
          }
           
            

          else {
            for (int i = stock_index-1; i >= position; i--) {
            ligne[i+4] = ligne[i];
           }
            for (int j = position; j < position+4; j++) {
            ligne[j] = ' ';
            }
            stock_index += 4;
          // on affiche la ligne
           for (int i = position; i < stock_index; i++) {
              uart_send(UART0, ligne[i]);
            }
            
            
            for (int i = position; i < stock_index - 4; i++) {
              uart_send(UART0, '\b');
            }
           position += 4;
        
            
            
          }

          
        }
        else if (c==0x20 && ligne_index == lignett){
          // si j'appuis sur la fleche espace
          // si fin de ligne caractère dessent et je l'ecrase si refait espacea

          if (stock_index > max_columns){
            
            
            //uart_send(UART0, ' ');
            
            
            // tant que prompte pas sur dernière colonne afficher en dessous
            // une fois qu'il
            uart_send(UART0, '\r');
            uart_send(UART0, '\n');
            uart_send_string(UART0, ">");
            int k = 0;
            for (int m = position ; m < stock_index; m++){
              uart_send(UART0, ligne[m]);
              k ++;
               
             }
            

           // uart_send(UART0, ligne[stock_index-1]);
            //\033[r;cH
          
            
            //move_cursor(ligne_index, max_columns-k);
            ligne_index++;
            lignett++;

            for (int i= 0; i< k; i++){
              ligne[i] = ligne[position+i];
              uart_send(UART0, '\b');
              
            }
            
            //uart_send(UART0, '\b');
            //ligne[0] = ligne[stock_index-1];
            
            
            
            stock_index = k;
            position = 0;
          }
          else {
        
          for (int i = stock_index; i > position; i--) {
            ligne[i] = ligne[i-1];
          }
        
          ligne[position] = ' ';
          
          stock_index += 1;
          // on affiche la ligne
          for (int i = position; i < stock_index; i++) {
            uart_send(UART0, ligne[i]);
          }
          for (int i = position; i < stock_index - 1; i++) {
            uart_send(UART0, '\b');
          }
          position += 1;
          

        
          
        }
        }
        else if (c== 0x03){
          // effacer le terminal 
          // 0x03 = CTRL-C
          
            uart_send_string(UART0, "\033[H\033[J");
            uart_send_string(UART0, ">");
            for (int i = 0; i < stock_index; i++) {
              ligne[i] = 0;
            }
            position = 0;
            stock_index = 0;
            ligne_index = 0; 
            lignett = 0;
            

        }

       else if (c =='\033'){
        
        uart_receive(UART0, &c);
        if (c== '['){
          uart_receive(UART0, &c);
          if (c=='A' && ligne_index > 0){
            // si j'appuis sur la fleche haut
            ligne_index --;
            uart_send_string(UART0, "\x1B[A");
          }
          else if (c== 'B' && ligne_index < lignett){
            // si j'appuis sur la fleche bas
            
            ligne_index ++;
            uart_send_string(UART0, "\x1B[B");
          
          }
          else if (c== 'C'){
            // si j'appuis sur la fleche droite
           
            position ++;
            uart_send_string(UART0, "\x1B[C");
          }
          else if (c== 'D'){
            // si j'appuis sur la fleche gauche
            
            position --;
            uart_send_string(UART0, "\x1B[D");
          }

        }
        
       }
      
        else {
          if (ligne_index == lignett){
            
          
            if (stock_index > max_columns){
            
            
              uart_send(UART0, c);
              
              for (int m = position ; m < stock_index-1; m++){
               uart_send(UART0, ligne[m]);
                
              }
              // sauter à la ligne, afficher prompt et caractere restants
              uart_send(UART0, '\r');
              uart_send(UART0, '\n');
              uart_send_string(UART0, ">");
              ligne_index++;
              lignett++;
  
              uart_send(UART0, ligne[stock_index-1]);
              ligne[0] = ligne[stock_index-1];
              
              for (int i= 1; i< stock_index; i++){
                ligne[i] = ' ';
              }
              stock_index = 1;
              position = 0;
            }
            else {
          
              for (int i = stock_index; i > position; i--) {
                ligne[i] = ligne[i-1];
              }
          
              ligne[position] = c;
            
              stock_index += 1;
            // on affiche la ligne
              for (int i = position; i < stock_index; i++) {
                uart_send(UART0, ligne[i]);
              }
              for (int i = position; i < stock_index - 1; i++) {
                uart_send(UART0, '\b');
            }
            position += 1;
      
            
            
            
              
            
          }
          
        }
        
    // et on l'affiche sur la sortie standard
    // tant que le fifo n'est pas vide
    // on lit le caractère reçu
    // et on l'affiche sur la sortie standard

      
      // et on l'affiche sur la sortie standard
       //uart_send_string(UART0, "Received: ");
        
      //}
    
    }
    }
  // on la confirme en écrivant dans le registre d'interruption
  // ce n'est pas nécéssaire car la fifo est vide 
    //mmio_write32(UART0_BASE_ADDRESS, UART_ICR, (1<<4));
    //uart_send_string(UART0, "\n");
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
     
      // et on l'affiche sur la sortie standard
      
      //uart_send_string(UART1, "\n");
      //mmio_write32(UART1_BASE_ADDRESS, UART_ICR, (1<<4));
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

