/*
 * @file uart_ARM.h
 *
 * @author Karl Emil Sandvik Bohne
 */

#ifndef SRC_UART_ARM_H_
#define SRC_UART_ARM_H_

#include "xil_types.h"

int arm_uart_init();

void uart_send(const u8 *reply, u16 num_bytes);

#endif /* SRC_UART_ARM_H_ */
