/**
 * @file task_ipc_tx.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-09-03
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "circular_buffer.h"
#include "main.h"

#if defined(ECE353_FREERTOS)
#include "task_ipc.h"

/* Global Variables */
TaskHandle_t TaskHandle_IPC_Tx = NULL;

/* FreeRTOS Queue used to send packets to the IPC Tx Task */
QueueHandle_t Queue_IPC_Tx; 

//circular_buffer
circular_buffer_t *Circular_Buffer_IPC_TX;

/**
 * @brief
 * This task is used to process outgoing IPC packets.
 * @param param
 * Unused parameter
 */
void task_ipc_tx(void *param)
{
    ipc_packet_t packet;

    while(1)
    {
        /* ADD CODE */
        //wait for an ipc_packet_t message from the queue.
        xQueueReceive(Queue_IPC_Tx, &packet, portMAX_DELAY);
        //send each byte of the packet using cyhal_uart_putc
        //uint8_t *data = (uint8_t *)&packet;
        for (int i = 0; i < sizeof(ipc_packet_t); i++)
        {
            while(circular_buffer_full(Circular_Buffer_IPC_TX))
            {
                // Wait for space in the circular buffer
                vTaskDelay(pdMS_TO_TICKS(5));
            }
            //use critical section
            taskENTER_CRITICAL();
            circular_buffer_add(Circular_Buffer_IPC_TX, ((uint8_t *)&packet)[i]);
            //cyhal_uart_putc(&IPC_Uart_Obj, data[i]);
            taskEXIT_CRITICAL();
        }
        //Enable the Transmit Empty Interrupts
        cyhal_uart_enable_event(&IPC_Uart_Obj, CYHAL_UART_IRQ_TX_EMPTY, INT_PRIORITY_IPC, true);

    }
}

bool task_ipc_resources_init_tx(void)
{
    /* Create the FreeRTOS Queue */
    Queue_IPC_Tx = xQueueCreate(IPC_TX_QUEUE_LENGTH, sizeof(ipc_packet_t));
    if(Queue_IPC_Tx == NULL)
    {
        return false;
    }

    /* Create the Circular Buffer for IPC Tx */
    Circular_Buffer_IPC_TX = circular_buffer_init(IPC_TX_CIRCULAR_BUFFER_SIZE);
    if(Circular_Buffer_IPC_TX == NULL)
    {
        return false;
    }

    /* Start the IPC Tx Task */
    BaseType_t task_ipc_tx_status = xTaskCreate(
        task_ipc_tx,                 // Function that implements the task.
        "IPC Tx Task",               // Text name for the task.
        IPC_STACK_SIZE,    // Stack size in words, not bytes.
        NULL,                       // Parameter passed into the task.
        IPC_PRIORITY,       // Priority at which the task is created.
        &TaskHandle_IPC_Tx          // Used to pass out the created task's handle.
    );

    if(task_ipc_tx_status != pdPASS)
    {
        return false;
    }
    else {
        return true; // Resources initialized successfully
    }
}
#endif