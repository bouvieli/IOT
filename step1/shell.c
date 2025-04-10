#include "shell.h"
#include "uart.h"
#include "isr.h"




char ligne[20];
char modif[100];



int ligne_index = 0;
int lignett=0;
int stock_index = 0;
int position = 0;
int max_columns = 19;
bool prevenu = false;
// si premier carac = \033 ou 0x1B alors c une chaine de carac a envoyer 
set<char> interpret(char buffer [], int offset){
    char c;
    uint32_t status;
    static int visible = 1;
    c = buffer[0];
    if (c== 0x0D || c == '\n') {
        if (ligne_index == lignett){
          
          lignett++;
        }
        
          ligne_index++;
        modif[0] = '\r';
        modif[1] = '\n';
        modif[2] = '>';
        /*uart_send(UART0, '\r');
        uart_send(UART0, '\n');
        uart_send_string(UART0, ">");*/
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
            modif[0] = '\b';
            //uart_send(UART0, '\b');
            int K = 0;
            for (int i = position-1; i < stock_index; i++) {
              modif[K+1] = ligne[i];
              //uart_send(UART0, ligne[i]);
              K++;
            }
            int L=K;
            stock_index--;
            while (K>0){
              modif[L+1] = '\b';
              //uart_send(UART0, '\b');
              K--;
              L++;
            }
            
            position--;

         
          
    }
        // rendre le curseur invisible/visible Ctrl+G
    else if (c== 0x07){
          if (visible == 1){
            visible = 0;
            // rendre le curseur invisible
            modif[0] = '\033';
            modif[1] = '[';
            modif[2] = '?';
            modif[3] = '2';
            modif[4] = '5';
            modif[5] = 'l';
          
            //uart_send_string(UART0, "\033[?25l");
          }
          else {
            visible = 1;
            // rendre le curseur visible
            modif[0] = '\033';
            modif[1] = '[';
            modif[2] = '?';
            modif[3] = '2';
            modif[4] = '5';
            modif[5] = 'h';
          
            //uart_send_string(UART0, "\033[?25h");
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
            modif[0] = '\r';
            modif[1] = '\n';
            modif[2] = '>';
            /*uart_send(UART0, '\r');
            uart_send(UART0, '\n');
            uart_send_string(UART0, ">");*/
            int k = 4;
            for (int i=0; i<4; i++){
              modif[i+3] = ' ';
              //uart_send(UART0, ' ');
            }
            int j =7;
            for (int m = position ; m < stock_index; m++){
              modif[j] = ligne[m];
              j++;
             // uart_send(UART0, ligne[m]);
              k ++;
               
             }


            lignett++;
            ligne_index++;

            for (int m = 0; m < 4; m++){
              ligne[m] = ' ';
              modif[j] = '\b';
              j++;
              //uart_send(UART0, '\b');
            }

            for (int i= 0; i< k-4; i++){
              ligne[i+4] = ligne[position+i];
              modif[j] = '\b';
              j++;
              //uart_send(UART0, '\b');
              
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
           int t =0;
           for (int i = position; i < stock_index; i++) {
            modif[t] = ligne[i];
            t++;
              //uart_send(UART0, ligne[i]);
            }
            
            
            for (int i = position; i < stock_index - 4; i++) {
              modif[t] = '\b';
              t++;
              //uart_send(UART0, '\b');
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

            /*uart_send(UART0, '\r');
            uart_send(UART0, '\n');
            uart_send_string(UART0, ">");*/
            modif[0] = '\r';
            modif[1] = '\n';
            modif[2] = '>';
            int k = 0;
            for (int m = position ; m < stock_index; m++){
              modif[k+3] = ligne[m];
              //uart_send(UART0, ligne[m]);
              k ++;
               
             }
            
             int l=k+3;
           // uart_send(UART0, ligne[stock_index-1]);
            //\033[r;cH
          
            
            //move_cursor(ligne_index, max_columns-k);
            ligne_index++;
            lignett++;

            for (int i= 0; i< k; i++){
              ligne[i] = ligne[position+i];
              modif[l] = '\b';
              l++;
              //uart_send(UART0, '\b');
              
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
          int t = 0;
          for (int i = position; i < stock_index; i++) {
            modif[t] = ligne[i];
            t++;
            //uart_send(UART0, ligne[i]);
          }
          for (int i = position; i < stock_index - 1; i++) {
            modif[t] = '\b';
            t++;
            //uart_send(UART0, '\b');
          }
          position += 1;
          

        
          
        }
    }
    else if (c== 0x03){
          // effacer le terminal 
          // 0x03 = CTRL-C
            modif[0] = '\033';
            modif[1] = '[';
            modif[2] = 'H';
            modif[3] = '\033';
            modif[4] = '[';
            modif[5] = 'J';

            //uart_send_string(UART0, "\033[H\033[J");
            //uart_send_string(UART0, ">");
            for (int i = 0; i < stock_index; i++) {
              ligne[i] = 0;
            }
            position = 0;
            stock_index = 0;
            ligne_index = 0; 
            lignett = 0;
            

    }

    else if (c =='\033' && offset >= 3){
        
        c = buffer[1];
        if (c== '['){
          c = buffer[2];
          if (c=='A' && ligne_index > 0){
            // si j'appuis sur la fleche haut
            ligne_index --;
            modif[0] = '\x1B';
            modif[1] = '[';
            modif[2] = 'A';
            //uart_send_string(UART0, "\x1B[A");
          }
          else if (c== 'B' && ligne_index < lignett){
            // si j'appuis sur la fleche bas
            
            ligne_index ++;
            modif[0] = '\x1B';
            modif[1] = '[';
            modif[2] = 'B';
            // uart_send_string(UART0, "\x1B[B");
          
          }
          else if (c== 'C'){
            // si j'appuis sur la fleche droite
           
            position ++;
            modif[0] = '\x1B';
            modif[1] = '[';
            modif[2] = 'C';
            //uart_send_string(UART0, "\x1B[C");
          }
          else if (c== 'D'){
            // si j'appuis sur la fleche gauche
            
            position --;
            modif[0] = '\x1B';
            modif[1] = '[';
            modif[2] = 'D';
            //uart_send_string(UART0, "\x1B[D");
          }

        }
        
    }
      
        else {
          if (ligne_index == lignett){
            
          
            if (stock_index > max_columns){
            
              modif[0] = c;
              //uart_send(UART0, c);
              int j=1;
              for (int m = position ; m < stock_index-1; m++){
                modif[j] = ligne[m];
                j++;
                //uart_send(UART0, ligne[m]);
               
                
              }
              // sauter à la ligne, afficher prompt et caractere restants
             /* uart_send(UART0, '\r');
              uart_send(UART0, '\n');
              uart_send_string(UART0, ">");*/
              modif[j] = '\r';
              modif[j+1] = '\n';
              modif[j+2] = '>';

              ligne_index++;
              lignett++;
  
              //uart_send(UART0, ligne[stock_index-1]);
              modif[j+3] = ligne[stock_index-1];
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
            int t = 0;
            for (int i = position; i < stock_index; i++) {
              modif[t] = ligne[i];
              t++;
                //uart_send(UART0, ligne[i]);
            }
            for (int i = position; i < stock_index - 1; i++) {
              modif[t] = '\b';
              t++;
               // uart_send(UART0, '\b');
            }
            position += 1;
      
            
            
            
              
            
          }
          
        }
        }
      
       
      return modif;

}
    

//char line[MAX_CHARS];
//uint32_t nchars = 0;

/*void process_buffer(uint32_t uartno) {
    char c;
    struct uart *u = get_uart(uartno);
    while (!ring_is_empty(&u->ring_lecture)) {
        c = ring_get(&u->ring_lecture);
        line[nchars] = c;
        nchars++;
         if (c =='\n'){
          interpret(line, nchars);
          nchars = 0;
        }
    }
}*/
void process_buffer(uint32_t uartno) {
    char c;
    struct uart *u = get_uart(uartno);
    
    if (!ring_is_empty(&u->ring_lecture)) {
        core_enable_irqs();
        u->rl(&u->cookie);
        
        //u->we(&u->cookie);
        core_disable_irqs();
        
        
    }
}