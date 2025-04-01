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
#include "isr.h"
#include "isr-mmio.h"

/*
 * Assembly functions:
 */
extern void _irqs_setup(void);
extern void _irqs_enable(void);
extern void _irqs_disable(void);
extern void _wfi(void);

/*
 * Data structure for handlers and cookies
 */
struct handler {
  void (*callback)(uint32_t, void*);
  void *cookie;
};

struct handler handlers[NIRQS];

/*
 * Interrupt Service Routine, up-called from assembly.
 * Needs to interogate the VIC about the masked interrupt
 * status and call the corresponding handlers.
 */
void isr() {
  // lit le registre d'état des interruptions
  // qui indique quelles interruptions IRQ sont actives
  uint32_t irq = *(volatile uint32_t*)(VIC_BASE_ADDR + VICIRQSTATUS);


  for (int i = 0; i < NIRQS; i++) {
    // si l'interruption est active on appelle la fonction de traitement
    // d'interruption associée
    // et on lui passe le cookie associé
    if (irq & (1 << i)) {
      if (handlers[i].callback) {
        handlers[i].callback(i, handlers[i].cookie);
        *(volatile uint32_t*)(VIC_BASE_ADDR + VICINTCLEAR) = (1 << i);
      }
    
  
  }

  }
}

// active les interruptions au niveau du processeur
void core_enable_irqs() {
  _irqs_enable();
}

// désactive les interruptions au niveau du processeur
void core_disable_irqs() {
  _irqs_disable();
}

// met le processeur en attente d'une interruption
void core_halt() {
  _wfi();
}

/*
 * Initial setup our interrupt support,
 * need to initialize both the hardware and software
 * sides.
 */
void vic_setup_irqs() {
  _irqs_setup();

  for (int i = 0; i < NIRQS; i++) {
    handlers[i].callback = NULL;
    handlers[i].cookie = NULL;
  }
 // toutes les interruptions sont effacées
  *(volatile uint32_t*)(VIC_BASE_ADDR + VICINTCLEAR) = 0xFFFFFFFF;
  // toutes les interruptions sont désactivées
  *(volatile uint32_t*)(VIC_BASE_ADDR + VICINTENABLE) = 0x00000000;

  
}

/*
 * Enables the given interrupt at the VIC level.
 */

void vic_enable_irq(uint32_t irq, void (*callback)(uint32_t, void*), void *cookie) {
  // enregistre le callback et le cookie associé à l'interruption du périphérique
  handlers[irq].callback = callback;
  handlers[irq].cookie = cookie;

// active interuption en mode irq du peripherique
*(volatile uint32_t*)(VIC_BASE_ADDR + VICINTENABLE) |= (1 << irq);

}

/*
 * Disables the given interrupt at the VIC level.
 */
void vic_disable_irq(uint32_t irq) {
  // désactive l'interruption du périphérique
  *(volatile uint32_t*)(VIC_BASE_ADDR + VICINTCLEAR) &= ~(1 << irq);
  // clear the handler
  handlers[irq].callback = NULL;
  handlers[irq].cookie = NULL;
}
