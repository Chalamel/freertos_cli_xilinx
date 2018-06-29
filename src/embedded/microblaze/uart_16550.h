/*
 * uart.h
 *
 *  Created on: 25. jun. 2018
 *      Author: Karl
 */

#ifndef SRC_UART_H_
#define SRC_UART_H_

/*
 * uart.c
 *
 *  Created on: 25. jun. 2018
 *      Author: Karl
 */

/**
 * @brief Initializes the UART: sets baud rate, interrupt-threshold, installs the ISR and enables interrupts
 *
 * @return XST_SUCCESS if successful. XST_FAILURE otherwise.
 */
int uart_init();

/**
 * @brief Sends a reply via the UART.
 *
 * @param reply Pointer to the reply to be sent
 *
 * @return void
 */
void uart_send(const u8 *reply, u16 num_bytes);

#endif /* SRC_UART_H_ */
