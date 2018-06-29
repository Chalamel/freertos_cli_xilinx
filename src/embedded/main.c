/**
 * @file main.c
 * @brief Contains main()
 *
 * @author Karl Emil Sandvik Bohne
 */

#include "sw_opts.h"
#include "main.h"

#include <stdio.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "xparameters.h"

#include "main.h"

extern void cli_task(void* p);

/**
 * @brief main
 *
 * Initializes the application
 */
int main() {

    TaskHandle_t cli_handle = NULL;
    xTaskCreate(cli_task, "cli_main", THREAD_STACKSIZE, (void*) 0, DEFAULT_THREAD_PRIO, &cli_handle);

    vTaskStartScheduler();

    while (1) {
        //Loop forever
    }

    return 0;
}
