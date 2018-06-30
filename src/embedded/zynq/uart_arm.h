/*
 * uart_zynq.h
 *
 *  Created on: 27. jun. 2018
 *      Author: Karl
 */

#ifndef SRC_UART_ZYNQ_H_
#define SRC_UART_ZYNQ_H_

int uart_init();

void uart_send(const u8 *reply, u16 num_bytes);

#endif /* SRC_UART_ZYNQ_H_ */
