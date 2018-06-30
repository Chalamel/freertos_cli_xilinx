/*
 * file uart_zynq.c
 * 
 * @brief Contains an initialization procedure and ISR for the
 *        Zynq / ARM UART core
 * 
 * @author Karl Emil Sandvik Bohne
 */

#include "xparameters.h"
#include "xil_types.h"
#include "xuartps.h"

#include "xscugic.h"

#include "FreeRTOS.h"
#include "queue.h"

#include "sw_opts.h"

XUartPs uart_inst;
extern QueueHandle_t uart_buf;
static u8 volatile in_transit;


/**
 * @brief ISR/handler for all interrupts to the UART
 *
 * @param *callback_ref Not in use, but points to the UART instance
 * @param event denotes the type of interrupt that triggered. See xuartns550.h
 * @param event_data if tx-/rx/timeout-interrupt: The number of bytes written/read/in rx-FIFO
 */
static void uart_ps_isr(void *callback_ref, u32 event, unsigned int event_data)
{

    //rx- or timeout-interrupt: data in rx-FIFO
	if (event == XUARTPS_EVENT_RECV_DATA || event == XUARTPS_EVENT_RECV_TOUT) {
        while ((XUartPs_IsReceiveData(XPAR_PS7_UART_1_BASEADDR)) == TRUE) {
            u8 recvd_byte = XUartPs_RecvByte(XPAR_PS7_UART_1_BASEADDR);
            xQueueSendFromISR(uart_buf, &recvd_byte, pdFALSE);
        }
	}

	//tx-interrupt: buffer fully sent.
	if (event == XUARTPS_EVENT_SENT_DATA) {
		in_transit = 0;
	}

	//Data was received with an error
	if (event == XUARTPS_EVENT_RECV_ERROR) {
		//TODO: Handle this if you want
	}

	//Data was received with an parity or frame or break error
	if (event == XUARTPS_EVENT_PARE_FRAME_BRKE) {
		//TODO: Handle this if you want
	}

	//Data was received with an overrun error
	if (event == XUARTPS_EVENT_RECV_ORERR) {
		//TODO: Handle this if you want
	}
}

void uart_send(const u8 *reply, u16 num_bytes) {
    in_transit = 1;
    XUartPs_Send(&uart_inst, reply, num_bytes);
    while (in_transit == 1) {
        //Spin
    }
}


int uart_init() {
	XUartPs_Config *config;
	u32 intr_mask;

	config = XUartPs_LookupConfig(XPAR_PS7_UART_1_DEVICE_ID);
	if (config == NULL) {
		return XST_FAILURE;
	}

	if (XUartPs_CfgInitialize(&uart_inst, config, config->BaseAddress) != XST_SUCCESS) {
		return XST_FAILURE;
	}

	if(XUartPs_SetBaudRate(&uart_inst, UARTBAUDRATE) != XST_SUCCESS) {
		xil_printf("Failed to set UART baudrate.\n");
		return XST_FAILURE;
	}

	if (XUartPs_SelfTest(&uart_inst) != XST_SUCCESS) {
		xil_printf("UART self test failed.\n");
		return XST_FAILURE;
	}

	XUartPs_SetHandler(&uart_inst, (XUartPs_Handler)uart_ps_isr, &uart_inst);

	intr_mask =
		XUARTPS_IXR_TOUT | XUARTPS_IXR_PARITY | XUARTPS_IXR_FRAMING |
		XUARTPS_IXR_OVER | XUARTPS_IXR_TXEMPTY | XUARTPS_IXR_RXFULL |
		XUARTPS_IXR_RXOVR;

	XUartPs_SetInterruptMask(&uart_inst, intr_mask);

    //Connect handlers/ISRs
    if (xPortInstallInterruptHandler(XPAR_XUARTPS_1_INTR, (XInterruptHandler) XUartPs_InterruptHandler,
            &uart_inst) != pdPASS) {
		xil_printf("Failed to install the UART ISR.\n");
        return XST_FAILURE;
    }

    //Enable the interrupt in the interrupt controller.
    vPortEnableInterrupt(XPAR_XUARTPS_1_INTR);

	XUartPs_SetOperMode(&uart_inst, XUARTPS_OPER_MODE_NORMAL);

	XUartPs_SetRecvTimeout(&uart_inst, 8);

	return XST_SUCCESS;
}
