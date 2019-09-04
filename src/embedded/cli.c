/*
 * @file cli.c
 * 
 * Simple FreeRTOS CLI implementation providing address-space access
 *
 * @author Karl Emil Sandvik Bohne
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <FreeRTOS_CLI.h>

#include <cli.h>

#ifndef configCOMMAND_INT_MAX_OUTPUT_SIZE
#define configCOMMAND_INT_MAX_OUTPUT_SIZE MAX_OUTPUT_LENGTH
#endif

/**
 * As defined in FreeRTOSConfig.h
 */
#if configAPPLICATION_PROVIDES_cOutputBuffer == 1
#define configCOMMAND_INT_MAX_OUTPUT_SIZE MAX_OUTPUT_LENGTH * 2
char cli_tx_buf[configCOMMAND_INT_MAX_OUTPUT_SIZE];
#endif


static BaseType_t reg_cli_cmds(void);
static BaseType_t cli_rd_cmd_handler(char *wr_buf, size_t wr_buf_len, const char *cmd_str);
static BaseType_t cli_wr_cmd_handler(char *wr_buf, size_t wr_buf_len, const char *cmd_str);


/**
 * Defines the read-address command by its signature, description, callback function pointer,
 * and number of parameters
 */
static const CLI_Command_Definition_t rd_addr_cmd = {
    "rd",
    "rd <addr>: Returns the value stored at <addr>\r\n",
    cli_rd_cmd_handler,
    1
};

/**
 * Defines the write-address command by its signature, description, callback function pointer,
 * and number of parameters
 */
static const CLI_Command_Definition_t wr_addr_cmd = {
    "wr",
    "wr <addr> <val>: Writes the value <val> to the address <addr>\r\n",
    cli_wr_cmd_handler,
    2
};


QueueHandle_t cli_rx_queue; //<! Received characters are posted to this queue via the cli_receive() function


/**
 * Registers the defined commands with the CLI, allowing use.
 */
static BaseType_t reg_cli_cmds(void) {

    if (FreeRTOS_CLIRegisterCommand(&rd_addr_cmd) != pdPASS) {
        return pdFAIL;
    }

    if (FreeRTOS_CLIRegisterCommand(&wr_addr_cmd) != pdPASS) {
        return pdFAIL;
    }

    return pdPASS;
}


/**
 * Function to be called by user when data for the CLI is received
 * 
 * @param data Pointer to the received data
 * @param len The length of the data received
 * 
 * @return The number of chars posted to the CLI character receive queue
 */
uint32_t cli_receive(char* data, uint32_t len)
{
    BaseType_t ret = pdTRUE;
    uint32_t wrtn = 0;

    while (len-- && ret == pdTRUE) {
        #if CLI_INPUT_RECVD_FROM_ISR == 1
        ret = xQueueSendFromISR(cli_rx_queue, data++, pdFALSE);
        #else
        ret = xQueueSend(cli_rx_queue, data++, pdFALSE);
        #endif
        if (ret == pdTRUE) {
            wrtn++;
        }
    }

    return wrtn; 
}

/**
 * Main function for the CLI task. It initializes a reception-queue, registers CLI commands, afterwards
 * it blocks on the queue. Upon reception of a complete command, the appropriate handler is called
 * 
 * The interface responsible for RX/TX of CLI characters must be initialized before calling this function
 *
 * @param *p Nothing
 */
void cli_task(void *p)
{
    char recvd_char;

    char rx_buf[MAX_INPUT_LENGTH];
    uint32_t rx_buf_index = 0;

    #if configAPPLICATION_PROVIDES_cOutputBuffer == 0
    char *tx_buf = FreeRTOS_CLIGetOutputBuffer();
    #else
    char *tx_buf = cli_tx_buf;
    #endif

    if (tx_buf == NULL) {
        puts("Failed to get CLI output buffer via FreeRTOS_CLIGetOutputBuffer()");
        vTaskDelete(NULL);
    }


    BaseType_t is_more = pdTRUE;

    cli_rx_queue = xQueueCreate(CLI_RECV_QUEUE_LEN, sizeof(uint8_t));
    if (cli_rx_queue == NULL) {
        puts("Failed to initialize the CLI queue.");
        vTaskDelete(NULL);
    }

    if (reg_cli_cmds() == pdPASS) {
        puts("CLI task initialized. Type \"help\" to see a list of available commands.");
    } else {
        puts("Failed to initialize the CLI task.");
        vTaskDelete(NULL);
    }

    xQueueReset(cli_rx_queue);

    while (1) {
        xQueueReceive(cli_rx_queue, &recvd_char, portMAX_DELAY);

        if (recvd_char == '\n') { //Complete command received
            do {
                is_more = FreeRTOS_CLIProcessCommand(rx_buf, tx_buf, MAX_OUTPUT_LENGTH);

                uint16_t tx_len = strlen(tx_buf);
                if (tx_len > 0) {
                    rx_buf[rx_buf_index] = '\n';
                    cli_send(rx_buf, strlen(tx_buf));
                }

            } while (is_more == pdTRUE);

            rx_buf_index = 0;
            memset(rx_buf, 0x00, MAX_INPUT_LENGTH);
            memset(tx_buf, 0x00, MAX_OUTPUT_LENGTH);

        } else { //Not done with command
            if (recvd_char == '\b') { //Backspace
                if (rx_buf_index > 0) {
                    rx_buf[--rx_buf_index] = '\0';
                }
            } else if (recvd_char != '\r') { //If not carriage return, then append the received character to buffer
                if (rx_buf_index < MAX_INPUT_LENGTH) {
                    rx_buf[rx_buf_index++] = recvd_char;
                }
            }
        }

    } //while(1)
}

/**
 * Implements the read-address command for the CLI.
 *
 * @param *wr_buf A buffer into which the reply is written
 * @param wr_buf_len The size of the buffer pointed to by *wr_buf
 * @param *cmd_str A string containing the received command
 *
 * @return pdTRUE if a partial reply was placed into wr_buf and the function should be called again.
 *         pdFALSE if the complete reply was placed into wr_buf
 */
static BaseType_t cli_rd_cmd_handler(char *wr_buf, size_t wr_buf_len, const char *cmd_str)
{
    uint32_t addr, val;
    const char *addr_param;
    BaseType_t addr_param_len;

    if (wr_buf_len < 5) { //4 bytes for value, 1 byte for zero termination
        puts("cli_rd_cmd_handler(): Insufficient buffer size. Must be at least 5B");
        return pdFALSE;
    }

    addr_param = FreeRTOS_CLIGetParameter(cmd_str, 1, &addr_param_len);
    addr = strtoul(addr_param, NULL, 0);

    if (addr != 0) {
        val = cli_rd_addr(addr);
    } else {
        puts("Error parsing provided address");
        return pdFALSE;
    }

    sprintf((char*) wr_buf, "%lu", val);
    return pdFALSE; //Done
}

/**
 * Implements the write-address command for the CLI.
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
static BaseType_t cli_wr_cmd_handler(char *wr_buf, size_t wr_buf_len, const char *cmd_str)
{
    uint32_t addr = 0;
    uint32_t val = 0;

    const char *addr_param, *val_param;
    char **str_end_pp = NULL;
    BaseType_t addr_param_len, val_param_len;

    addr_param = FreeRTOS_CLIGetParameter(cmd_str, 1, &addr_param_len);
    val_param = FreeRTOS_CLIGetParameter(cmd_str, 2, &val_param_len);

    addr = strtoul(addr_param, str_end_pp, 0);
    if (addr == 0) {
        puts("Error parsing provided address");
        return pdFALSE;
    }

    val = strtoul(val_param, str_end_pp, 0);
    if (val == 0 && (str_end_pp == NULL || *str_end_pp == wr_buf)) {
        puts("Error parsing provided write-value");
    } else {
        cli_wr_addr(addr, val);
    }

    return pdFALSE; //Done
}
