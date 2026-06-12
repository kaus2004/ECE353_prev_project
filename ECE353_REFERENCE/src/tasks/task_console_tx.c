/**
 * @file task_console.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-15
 * 
 * @copyright Copyright (c) 2025
 * 
 */
 #include "main.h"

#ifdef ECE353_FREERTOS
#include "drivers.h"
#include "task_console.h"
#include "cyhal_uart.h"

/**
 * @brief
 * This file contains the implementation of the console transmit (Tx) task.
 * The task is responsible for sending characters to the UART.
 * 
 * Tasks can print messages by sending the string to task_console_tx() using
 * a FreeRTOS queue.
 *
 * task_console_tx() will add the characters to a circular buffer that is
 * accessed by the UART interrupt service routine (ISR).
 *
 */

/* ADD CODE*/
/* Global Variables */

//Allocate space for the transmit queue
QueueHandle_t xQueue_Console_Tx = NULL; //Queue of console_buffer_t messages
circular_buffer_t *circular_buffer_tx  = NULL;
//Create Task handle
TaskHandle_t TaskHandle_Console_Tx = NULL;

//Allocate Space for the Circular Buffer


/**
 * @brief 
 * This task is used to transmit characters to the UART
 * @param param 
 */
void task_console_tx(void *param)
{
    (void)param; // Unused parameter
    console_buffer_t tx_msg;

    while (1)
    {
        /* ADD CODE */
        //wait for console_buffer_t message from the queue.
        xQueueReceive(xQueue_Console_Tx, &tx_msg, portMAX_DELAY);

        //A for loop that examines each message and adds each byte into the circular buffer
        for (int i = 0; i < strlen(tx_msg.data); i++)
        {

            //If the cirucular buffer is full, vTaskDelay(5);
            //there is no circular_buffer_is_full(circular_buffer_tx)
            //use other way to check if full
            while (circular_buffer_get_num_bytes(circular_buffer_tx) == circular_buffer_tx->max_size) //check if full
            {
                vTaskDelay(5);
            }
            //Add the next byte to the Circ Buff(CB)
            circular_buffer_add(circular_buffer_tx, tx_msg.data[i]);

            //Enable the Transmit Empty Interrupts
            cyhal_uart_enable_event(&cy_retarget_io_uart_obj, CYHAL_UART_IRQ_TX_EMPTY, INT_PRIORITY_CONSOLE, true);

        }
        
        //Free the data that was sent from the console_buffer_t
        vPortFree(tx_msg.data);
    }
}

/**
 * @brief 
 * This function initializes the resources for the console Tx task. 
 * @return true  if initialization is successful
 * @return false if initialization fails
 * @return false 
 */
bool task_console_resources_init_tx(void)
{
    BaseType_t rslt;

    /* ADD CODE */
    //Initialize the Tx FreeRTOS queue
    xQueue_Console_Tx = xQueueCreate(10, sizeof(console_buffer_t));
    if (xQueue_Console_Tx == NULL)
    {
        // Failed to create the queue
        printf("Failed to create console Tx queue\n");
        CY_ASSERT(0);
    }

    //Initialize the circular Buffer
    circular_buffer_tx = circular_buffer_init(128); //128 byte buffer
    if (circular_buffer_tx == NULL)
    {
        // Failed to create the circular buffer
        printf("Failed to create console Tx circular buffer\n");
        CY_ASSERT(0);
    }

    //Create the FreertOS task for the console Tx
    rslt = xTaskCreate(
        task_console_tx,                     // Function that implements the task.
        "Task Console Tx",                   // Text name for the task.
        configMINIMAL_STACK_SIZE * 4,       // Stack size in words, not bytes.
        NULL,                                // Parameter passed into the task.
        tskIDLE_PRIORITY + 1,               // Priority at which the task is created.
        &TaskHandle_Console_Tx               // Used to pass out the created task's handle.
    );


    // Resources initialized successfully

    if (rslt != pdPASS)
    {
        return false; // Initialization failed
    }

    return true; // Resources initialized successfully
}

/**
 * @brief
 * This function sends formatted messages to task_console_tx. It acts as a wrapper around the FreeRTOS queue
 * to send messages so other tasks can use it easily.
 *
 * Example usage: 
 * task_console_printf("Send Message");
 * task_console_printf("Formatted number: %d", 42);
 *
 * @param str_ptr Pointer to the format string.
 * @param ...     Additional arguments for formatting.
 */
void task_console_printf(char *str_ptr, ...)
{
    console_buffer_t console_buffer;
    char *message_buffer;
    char *task_name;
    uint32_t length = 0;
    va_list args;

    /* ADD CODE */
    /* Allocate the message buffer */
    message_buffer = pvPortMalloc(CONSOLE_MAX_MESSAGE_LENGTH);
    if (message_buffer == NULL)
    {
        /* pvPortMalloc failed. Handle error */
        CY_ASSERT(0); // Halt the processor
    }

    if (message_buffer)
    {
        va_start(args, str_ptr);
        task_name = pcTaskGetName(xTaskGetCurrentTaskHandle());
        length = snprintf(message_buffer, CONSOLE_MAX_MESSAGE_LENGTH, "%-16s : ",
                              task_name);

        vsnprintf((message_buffer + length), (CONSOLE_MAX_MESSAGE_LENGTH - length),
                  str_ptr, args);

        va_end(args);

        /* ADD CODE */
        /* Initialize the console buffer */
        console_buffer.data = message_buffer;
        console_buffer.index = 0; // Not used in this context

        /* Send the console buffer to the Tx queue */
        xQueueSend(xQueue_Console_Tx, &console_buffer, portMAX_DELAY);

        /* The receiver task is responsible to free the memory from here on */

    }
    else
    {
        /* pvPortMalloc failed. Handle error */
        CY_ASSERT(0); // Halt the processor
    }
}
#endif