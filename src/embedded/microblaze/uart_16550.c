/*
 * @file uart_16550.c
 * 
 * @brief Contains an initialization procedure and ISR for the
 *        Xilinx UART16550 core
 *
 * @author Karl Emil Sandvik Bohne
 */

#include "xparameters.h"
#include "xil_types.h"
#include "xuartns550.h"

#include "FreeRTOS.h"
#include "queue.h"

#include "sw_opts.h"

XUartNs550 uart_inst;
extern QueueHandle_t uart_buf;
static u8 volatile in_transit;

/**
 * @brief ISR/handler for all interrupts to the UART
 *
 * @param *callback_ref Not in use, but points to the UART instance
 * @param event denotes the type of interrupt that triggered. See xuartns550.h
 * @param event_data if tx-/rx/timeout-interrupt: The number of bytes written/read/in rx-FIFO
 */
static void uart_isr(void *callback_ref, u32 event, unsigned int event_data) {

    //rx- or timeout-interrupt: data in rx-FIFO
    if (event == XUN_EVENT_RECV_DATA || event == XUN_EVENT_RECV_TIMEOUT) {
        while (XUartNs550_IsReceiveData(XPAR_AXI_UART16550_0_BASEADDR) == TRUE) {
            u8 recvd_byte = XUartNs550_RecvByte(XPAR_AXI_UART16550_0_BASEADDR);
            xQueueSendFromISR(uart_buf, &recvd_byte, pdFALSE);
        }
    }

    //tx-interrupt: buffer fully sent.
    else if (event == XUN_EVENT_SENT_DATA) {
        in_transit = 0;
    }

}

/**
 * @brief Sends a reply via the UART.
 *
 * @param reply Pointer to the reply to be sent
 *
 * @return void
 */
void uart_send(const u8 *reply, u16 num_bytes) {
    in_transit = 1;
    XUartNs550_Send(&uart_inst, reply, num_bytes);

    while (in_transit == 1) {
        //Spin
    }
}

/**
 * @brief Initializes the UART: sets baud rate, interrupt-threshold, installs the ISR and enables interrupts
 *
 * @return XST_SUCCESS if successful. XST_FAILURE otherwise.
 */
int uart_init() {
    in_transit = 0;

    if (XUartNs550_Initialize(&uart_inst, XPAR_AXI_UART16550_0_DEVICE_ID) != XST_SUCCESS) {
        xil_printf("UART initialization failed\n"); //This might not print correctly as BR not yet set
        return XST_FAILURE;
    }

    XUartNs550_SetBaud(XPAR_AXI_UART16550_0_BASEADDR, XPAR_AXI_UART16550_0_CLOCK_FREQ_HZ, UARTBAUDRATE);

    if (XUartNs550_SelfTest(&uart_inst) != XST_SUCCESS) {
        xil_printf("UART self test failed\n");
        return XST_FAILURE;
    }

    //Set the rx-interrupt FIFO-threshold.
    XUartNs550_SetFifoThreshold(&uart_inst, XUN_FIFO_TRIGGER_08);

    //Set the ISR/handler
    XUartNs550_SetHandler(&uart_inst, uart_isr, &uart_inst);

    //Connect handlers/ISRs
    if (xPortInstallInterruptHandler(XPAR_INTC_0_UARTNS550_0_VEC_ID, (XInterruptHandler) XUartNs550_InterruptHandler,
            &uart_inst) != pdPASS) {
        xil_printf("Failed to install the UART ISR.\n");
        return XST_FAILURE;
    }

    //Enable the interrupt for the peripheral.
    XUartNs550_EnableIntr(XPAR_AXI_UART16550_0_BASEADDR);

    //Enable the interrupt in the interrupt controller.
    vPortEnableInterrupt(XPAR_INTC_0_UARTNS550_0_VEC_ID);

    return XST_SUCCESS;
}

