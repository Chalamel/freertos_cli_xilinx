# FreeRTOS + CLI for remote access to device address-space 
This repository contains an implementation of the FreeRTOS command line interface (CLI) as well as a user-implementable interface that together provide remote address-space access to any device on the condition that a dedicated host-device communication interface, such as a UART or socket, and available.

## Commands
  - **wr** <*addr*> <*val*>
    - Writes the value <*val*> to the address <*addr*> 
  - **rd** <*addr*>
    - Returns the value stored at <*addr*>
  - **help**
    - Display available commands 

Hex-encoded- and base-10 input is accepted, and hex-encoded input must be identified by the initial sequence *0x*, e.g 0xDEADBEEF. Input shall be terminated with the linefeed character ```\n```, but a carriage return character followed by a linefeed character (```\r\n```) is also valid.

## Including and using in a project
- Include in a project the source- and header files from this repository, located in the *src/embedded* directory

- Provide the three functions specified in cli.h that implement the interface between CLI and communication-interface (UART/socket, etc), that respectively transmits data using the communication-interface, and performs the actual write- and read operations:
    ```c
    extern uint32_t cli_send(char *data, uint32_t len);
    
    extern void cli_wr_addr(uint32_t addr, uint32_t val);
    
    extern uint32_t cli_rd_addr(uint32_t addr);
    ```


- The CLI is handled by a task that is spawned like any other FreeRTOS task. For instance:
    ```c
    TaskHandle_t cli_handle;
    xTaskCreate(cli_task, "cli_task", CLI_TASK_STACKSIZE, NULL, CLI_TASK_PRIO, &cli_handle);
    ```


## Implementing additional commands
Define a command:

```c
static const CLI_Command_Definition_t my_cmd = {
    "<my_cmd signature>", //An identifier in the form of a string, e.g "rd" for the read-command
    "<description of my_cmd>", //String-descrption of the command
    <func_ptr_to_my_cmd_handler>,
    <number_of_arguments>
};

const CLI_Command_Definition_t* my_cmd_type_ptr = &my_cmd;
```

Then define the command handler function. The main CLI task function automatically calls the appropriate handler function depending on the received command signature. All handlers must take the parameters shown in the following example function:

```c
/**
 * Implements the my_cmd command
 *
 * @param *wr_buf A buffer into which any reply data is written
 * @param wr_buf_len The size of the buffer pointed to by *wr_buf
 * @param *cmd_str A string containing the received command
 */
BaseType_t my_cmd_handler(uint8_t *wr_buf, size_t wr_buf_len, const char *cmd_str) {
    //Parse command parameters (if any). In this case pretend there is one:
    BaseType_t p1_len; //The length of the received parameter
    const char *p1 = FreeRTOS_CLIGetParameter(cmd_str, 1, &p1_len)
    
    /* Do whatever, as defined by the received parameters, and write (zero-terminated) reply to wr_buf.
    If no reply is to be generated, zero-terminate wr_buf at index 0. In this case we simply echo back
    the parameter by copying the received parameter to the reply buffer. The reply is sent in the
    cli_task() function */
    
    strcpy(p1, wr_buf);

    /* Return pdFALSE if no more data is to be replied / no more actions should be taken. Return pdTRUE
    if the handler should be called again. In this case we are done, and so do the former */
    
    return pdFALSE;
}
```

Register the newly defined command with the CLI:
```c
#include <FreeRTOS_CLI.h>
. . .
//Somewhere before using the command
FreeRTOS_CLIRegisterCommand(my_cmd_type_ptr)
```
For a more detailed description of the FreeRTOS CLI functionality, see FreeRTOS.org.
