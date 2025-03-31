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

#ifndef UART_MMIO_H_
#define UART_MMIO_H_

/**
 * To fill this header file,
 * look at the document describing the Versatile Application Board:
 *
 *    Versatile Application Baseboard for ARM926EJ-S (DUI0225)
 */

/*
 * We need the base address for the different serial lines.
 */

#define UART0_BASE_ADDRESS ((void*)0x101F1000) 
#define UART1_BASE_ADDRESS ((void*)0x101F2000) 
#define UART2_BASE_ADDRESS ((void*)0x101F3000) 

/*
 * Is the UART chipset a PL011?
 * If so, we need the details for the data and status registers.
 */
// debut deu refistre de data de mon uart debut de la où sont stocker les données
#define UART_DR 0x00 
//debut des registre de flag de mon uart pour connaitre l'etat de mon uart
#define UART_FR 0x18 

//registre qui permet d'activer ou de désactiver les interruptions
// if 1<<4 = 1 then the RX interrupt is enabled
// if 1<<5 = 1 then the TX interrupt is enabled
#define UART_IMSC 0x38 

//permet de savoir si une interruption est en attente 
#define UART_RIS 0x3C 

//permet de savoir si une interruption est en attente et si elle est active
#define UART_MIS 0x40 

// permet de ne pas prendre en compte une interruption déjà traitée
#define UART_ICR 0x44 

#endif /* UART_MMIO_H_ */
