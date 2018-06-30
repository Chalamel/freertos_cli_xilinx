# FreeRTOS + CLI providing address-space access to Xilinx devices
This repository contains a simple implementation of the FreeRTOS command line interface (CLI) for use with Xilinx devices, defining commands that provide access to the AXI address space of such a device via any terminal.

# Commands
  - **wr** <*addr*> <*val*>
    - Writes the value <*val*> to the address <*addr*> 
  - **rd** <*addr*>
    - Returns the value stored at <*addr*>
  - **help**
    - Display available commands 

Hex-encoded- and base-10 input is accepted for the parameters, and hex-encoded input
must be identified by the initial sequence *0x*, e.g 0xDEADBEEF. It is expected
that input is terminated with the linefeed character ```\n```, but a carriage
return character followed by a linefeed character (```\r\n```) is also valid.

# Implementing additional commands
..is easy. First define a command:

```c
static const CLI_Command_Definition_t my_cmd = {
    "<my_cmd signature>", //e.g "rd" for the read-command
    "<description of my_cmd>",
    <func_ptr_to_my_cmd_handler>,
    <number_of_arguments>
};

const CLI_Command_Definition_t* my_cmd_type_ptr = &my_cmd;
```

Then define the command handler function. The main function that is defined for the
CLI task automatically calls the appropriate handler function depending on the
received command signature. All handlers **must** take the parameters shown in
the following example function:

```c
/**
 * @brief Implements the my_cmd command
 *
 * @param *wr_buf A buffer into which any reply data is written
 * @param wr_buf_len The size of the buffer pointed to by *wr_buf
 * @param *cmd_str A string containing the received command
 */
BaseType_t my_cmd_handler(uint8_t *wr_buf, size_t wr_buf_len, const char *cmd_str) {
    //Parse command parameters (if any). In this case we pretend there is one:
    BaseType_t p1_len; //The length of the received parameter
    const char *p1 = FreeRTOS_CLIGetParameter(cmd_str, 1, &p1_len)
    
    /* Do whatever, as defined by the received parameters, and write (zero-terminated) reply to wr_buf.
    If no reply is to be generated, zero-terminate wr_buf at index 0. In this case we simply echo back
    the parameter by copying the received parameter to the reply buffer */
    
    strcpy(p1, wr_buf);

    /* Return pdFALSE if no more data is to be replied / no more actions should be taken. Return pdTRUE
    if the handler should be called again. In this case we are done, and so do the former */
    
    return pdFALSE;
}
```

Register the newly defined command with the CLI:
```c
#include "FreeRTOS_CLI.h"

. . .

FreeRTOS_CLIRegisterCommand(my_cmd_type_ptr)
```
After registering, the command can be used. For a more detailed description of
the FreeRTOS CLI functionality, see FreeRTOS.org.


# Including in a project
Include the source files in the base *src/embedded* directory in a
FreeRTOS-based project. It is assumed that there is an already-configured source
of input data (for instance a socket- or UART interface) that posts any data it
receives to the FreeRTOS queue ``` extern QueueHandle_t cli_input_buf ``` (this
should be replaced with a FreeRTOS StreamBuffer if using a FreeRTOS version past
v10.0), and an associated function ``` void cli_send(const u8 *reply, u16
num_bytes)```. The files uart_16500.c and uart_zynq.c along with their headers
can be used if a design includes the AXI UART16550 core, or if a Zynq device is
used, respectively, which contain initialization routines and ISRs for the two
UART types; these are contained in *src/embedded/zynq* and
*src/embedded/microblaze*. The FreeRTOS source file FreeRTOS_CLI.c and header
FreeRTOS_CLI.h are also required, which are available from FreeRTOS.org.

The CLI is handled by a task that is spawned like any other FreeRTOS task:

```c
TaskHandle_t cli_handle = NULL;
xTaskCreate(cli_task, "cli_main", THREAD_STACKSIZE, (void*) 0, CLI_THREAD_PRIO, &cli_handle); //We could allocate statically also.
```
