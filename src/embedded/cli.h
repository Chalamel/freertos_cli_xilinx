/*
 * @file cli.h
 *
 *  Created on: 25. jun. 2018
 *      Author: Karl Emil Sandvik Bohne
 */

#ifndef CLI_H
#define CLI_H

#include <stdint.h>


/**
 * Shall be 1 if data for the CLI is received in an ISR context, or 0 otherwise
 */
#define CLI_INPUT_RECVD_FROM_ISR 1

/**
 * Length of the FreeRTOS queue to which received characters are posted
 */
#define CLI_RECV_QUEUE_LEN 512

/**
 * Max allowable length of terminal input for a single command
 */
#define MAX_INPUT_LENGTH 256

/**
 * Max allowable length of terminal output, per call to command handler
 */
#define MAX_OUTPUT_LENGTH 256


/**
 * Must be implemented by user.
 * Called by CLI task when a reply to a received command is ready and can be sent. 
 * Typically this is a wrapper for a uart-/socket send-/write function. Function may block
 * 
 * @param data Pointer to the data to be sent
 * @param len The length of the data pointed to by "data"
 * 
 * @return The number of bytes that was sent by the function
 */
uint32_t cli_send(char* data, uint32_t len);

/**
 * Function to be called by user when data for the CLI is received
 * 
 * @param data Pointer to the received data
 * @param len The length of the received data
 * 
 * @return The number of chars posted to the CLI character receive queue
 */
uint32_t cli_receive(char* data, uint32_t len);

/**
 * Must be implemented by user.
 * Called by CLI task in order to perform the actual write-operation in memory.
 * 
 * @param addr The memory address where the value is to be written
 * @param val The value that will be written to addr
 */
void cli_wr_addr(uint32_t addr, uint32_t val);

/**
 * Must be implemented by user.
 * Function is called by CLI task in order to perform the actual read-operation from memory
 * 
 * @param addr The memory address at which to perform the read-operation
 * 
 * @return The value stored at addr
 */
uint32_t cli_rd_addr(uint32_t addr);

/**
 * Main function for the CLI task. It initializes a reception-queue, registers CLI commands, afterwards
 * it blocks on the queue.
 * 
 * The interface responsible for RX/TX of CLI characters must be initialized before calling this function
 *
 * @param *p Nothing
 */
void cli_task(void *p);

#endif /* CLI_H */
