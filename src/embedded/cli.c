/*
 * @file cli.c
 * 
 * @brief Simple FreeRTOS CLI implementation providing AXI address-space access
 *
 * @author Karl Emil Sandvik Bohne
 */

#include <stdlib.h>
#include <stdio.h>

#include "sw_opts.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "xil_types.h"
#include "xil_io.h"

#include "cli.h"

#include YOUR_UART_HEADER

#define MAX_INPUT_LENGTH 200 //<! Max allowable length of terminal input
#define MAX_OUTPUT_LENGTH 200 //<! Max allowable length of terminal output (per call to command handler)
#define UART_BUF_SIZE 1024

QueueHandle_t uart_buf; //<! The UART - CLI queue. Change this to a StreamBuffer when it works

/**
 * @brief Registers the defined commands with the CLI, allowing them to be used.
 */
BaseType_t reg_cli_cmds() {

    if (FreeRTOS_CLIRegisterCommand(rd_addr_cmd_ptr) != pdPASS) {
        xil_printf("Failed to the add read-command for the CLI.\n");
        return pdFAIL;
    }

    if (FreeRTOS_CLIRegisterCommand(wr_addr_cmd_ptr) != pdPASS) {
        xil_printf("Failed to add the write-command for the CLI.\n");
        return pdFAIL;
    }
    
    return pdPASS;
}

/**
 * @brief Main function for the CLI task
 *
 * The CLI task blocks on a queue into which data is posted by the UART ISR upon reception of data
 * on the serial interface.
 *
 * @param *p Nothing, but must adhere to the FreeRTOS task-function prototype
 */
void cli_task(void* p) {

    static const u8 newline = '\n';

    u8 recvd_char;
    u8 recv_buf[MAX_INPUT_LENGTH];
    u8 recv_buf_index = 0;
    u8 ret_buf[MAX_OUTPUT_LENGTH];

    BaseType_t more_data = pdTRUE;

    /*Replace this with your own UART-/socket initialization procedure, or use
    one of the two provided examples (UART16550/ARM UART) */
    uart_init(); 

    uart_buf = xQueueCreate(UART_BUF_SIZE, sizeof(u8));
    if (uart_buf != NULL) {
        xil_printf("UART stream-buffer initialized.\n");
    } else {
        xil_printf("Failed to initialize the UART stream-buffer.\n");
        vTaskDelete(NULL);
    }

    if (reg_cli_cmds() == pdPASS) {
        xil_printf("CLI task initialized. Type \"help\" to see a list of available commands.\n");
    } else {
        xil_printf("Failed to initialize the CLI task.\n");
        vTaskDelete(NULL);
    }

    xQueueReset(uart_buf);

    while (1) {
        xQueueReceive(uart_buf, &recvd_char, portMAX_DELAY); //Could get more than 1 and loop but w/e

        if (recvd_char == '\n') { //Received newline-char implies complete command received. Call command interpreter.
            do {
                more_data = FreeRTOS_CLIProcessCommand((char*) recv_buf, (char*) ret_buf, MAX_OUTPUT_LENGTH);

                u16 len_to_send = strlen((char*) ret_buf); //Any output now stored in ret_buf, so send it.
                if (len_to_send > 0) {
                    /*Replace this with your own UART-/socket send-procedure, or use
                    one of the two provided examples (UART16550/ARM UART) */
                    uart_send(ret_buf, strlen((char*) ret_buf));
                    uart_send(&newline, 1);
                }

            } while (more_data == pdTRUE);

            //Done with command, so send data, reset index and buffers.
            recv_buf_index = 0;
            memset(recv_buf, 0x00, MAX_INPUT_LENGTH);
            memset(ret_buf, 0x00, MAX_OUTPUT_LENGTH);

        } else { //Not done with command
            if (recvd_char == '\r') {
                //Don't care about carriage return
            } else if (recvd_char == '\b') { //Backspace. Remove last char in buffer, if any exist, and zero terminate new resulting string
                if (recv_buf_index > 0) {
                    recv_buf[--recv_buf_index] = '\0';
                }
            } else { //Add received character to receive buffer
                if (recv_buf_index < MAX_INPUT_LENGTH) {
                    recv_buf[recv_buf_index++] = recvd_char;
                } //Should handle the edge case where we receive data in excess of our max buffer size.
            }
        }

    } //while(1)
}

/**
 * @brief Implements the read-address command for the CLI.
 *
 * @param *wr_buf A buffer into which the reply (in this case the value at the provided address) is written
 * @param wr_buf_len The size of the buffer pointed to by *wr_buf
 * @param *cmd_str A string containing the received command
 *
 * @return pdTRUE if a partial reply was placed into wr_buf and the function should be called again.
 *         pdFALSE if the complete reply was placed into wr_buf
 */
BaseType_t rd_addr_cli(s8 *wr_buf, size_t wr_buf_len, const s8 *cmd_str) {
    u32 addr, val;
    const char *addr_param;
    u8 **param_err_ptr = NULL;
    BaseType_t addr_param_len;

    if (wr_buf_len < 5) { //4 bytes for value, 1 byte for zero termination
        xil_printf("rd_addr_cli(): Insufficient buffer size. Should be at least 5B but was %uB\n", wr_buf_len);
        return pdFALSE;
    }

    addr_param = FreeRTOS_CLIGetParameter((char*) cmd_str, 1, &addr_param_len);

    addr = strtoul((char*) addr_param, (char**) param_err_ptr, 0);

    if (param_err_ptr == NULL) {
        val = Xil_In32(addr);
    } else {
        val = 0;
    }

    sprintf((char*) wr_buf, "%lu", val);

    return pdFALSE; //Done
}

/**
 * @brief Implements the write-address command for the CLI.
 *
 * @param *wr_buf A buffer into which the reply (in this case nothing) is written
 * @param wr_buf_len The size of the buffer pointed to by *wr_buf
 * @param *cmd_str A string containing the received command
 *
 * @return pdTRUE if a partial reply was placed into wr_buf and the function should be called again.
 *         pdFALSE if the complete reply was placed into wr_buf
 *
 * @note No reply is generated by the write-address command, but the function prototype must still
 *       include a buffer and length in order to satisfy the requirements of a CLI command callback function.
 *       Since the buffer is not used, no checks are performed on it.
 */
BaseType_t wr_addr_cli(s8 *wr_buf, size_t wr_buf_len, const s8 *cmd_str) {
    u32 addr, val;
    const char *addr_param, *val_param;
    u8 **param_err_ptr = NULL;
    BaseType_t addr_param_len, val_param_len;

    addr_param = FreeRTOS_CLIGetParameter((char*) cmd_str, 1, &addr_param_len);
    val_param = FreeRTOS_CLIGetParameter((char*) cmd_str, 2, &val_param_len);

    addr = strtoul((char*) addr_param, (char**) param_err_ptr, 0);
    val = strtoul((char*) val_param, (char**) param_err_ptr, 0);

    if (param_err_ptr == NULL) {
        Xil_Out32(addr, val);
    }

    return pdFALSE; //Done
}

/**
 * @brief Checks if the input pointed to by data is on the form 0x<hex_chars>.
 *
 * @param *data Pointer to the data string to check
 * @len Length of the data string pointed to by *data
 *
 * @return 1 if string is hex-formatted, 0 otherwise
 */
u8 is_hex_enc(u8 *data, u8 len) {
    return (strstr((char*) data, "0x") == (char*) data && (data[strspn((char*) (data + 2), "0123456789abcdefABCDEF")] == 0));
}
