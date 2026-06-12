/**
 * @file task_console_rx.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-21
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "task_console.h"

#ifdef ECE353_FREERTOS  
/**
 * @brief 
 * This function is the event handler for the console UART.
 *
 * The ISR will receive characters from the UART and store them in a console buffer
 * until the user presses the ENTER key.  At that point, the ISR will send a task
 * notification to the console task to process the received string.
 *
 * The ISR will also echo the received character back to the console.
 * //use of double buffers is very fast and efficient
 *
 * @param handler_arg 
 * @param event 
 */
void console_event_handler(void *handler_arg, cyhal_uart_event_t event)
{
    (void)handler_arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t c;

    if ((event & CYHAL_UART_IRQ_RX_NOT_EMPTY) == CYHAL_UART_IRQ_RX_NOT_EMPTY)
    {
        // read in the character
        cyhal_uart_getc(&cy_retarget_io_uart_obj, &c, 0);

        //echo the character back to the console FIFO
        cyhal_uart_putc(&cy_retarget_io_uart_obj, c);

        //if backspace/delete then remove last character from array
        if (c == 0x7F)
        {
            if (produce_console_buffer->index > 0)
            {
                produce_console_buffer->index--;
            }
        }
        //else if newline (\n or \r) null terminate the string and notify the task
        //swap the roles of the produce and consume buffers using temp pointer
        else if ((c == '\n') || (c == '\r'))
        {
            produce_console_buffer->data[produce_console_buffer->index] = '\0'; // Null-terminate the string
            console_buffer_t *temp = produce_console_buffer;
            produce_console_buffer = consume_console_buffer;
            consume_console_buffer = temp;
            produce_console_buffer->index = 0; // Reset the index for the new produce buffer

            // Defensive: ensure the console Rx task handle is valid before notifying
            if (TaskHandle_Console_Rx != NULL)
            {
                vTaskNotifyGiveFromISR(TaskHandle_Console_Rx, &xHigherPriorityTaskWoken);
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
            }
            else
            {
                // If the task handle is NULL this likely means the task wasn't created
                // yet or failed to be created. Echo a debug message back to the host
                // so it's easier to diagnose on the serial terminal.
                const char dbg[] = "\r\n[ISR] Warning: Console RX task handle NULL\r\n";
                for (size_t i = 0; i < (sizeof(dbg) - 1); ++i)
                {
                    cyhal_uart_putc(&cy_retarget_io_uart_obj, dbg[i]);
                }
            }
        }
        //else add character to the array and increment the index
        else
        {
            produce_console_buffer->data[produce_console_buffer->index] = c;
            produce_console_buffer->index++;
        }

    }
    if ((event & CYHAL_UART_IRQ_TX_EMPTY) == CYHAL_UART_IRQ_TX_EMPTY)
    {
        /* ADD CODE */
        //if the cb is empty then disable the TX_EMPTY interrupt
        if (circular_buffer_empty(circular_buffer_tx))
        {
            cyhal_uart_enable_event(&cy_retarget_io_uart_obj, CYHAL_UART_IRQ_TX_EMPTY, INT_PRIORITY_CONSOLE, false);
        }

        //if the cb is not empty then add the next character to the UART FIFO and begin transmitting
        else
        {
            char next_char;
            circular_buffer_remove(circular_buffer_tx, &next_char);
            cyhal_uart_putc(&cy_retarget_io_uart_obj, next_char);
        }
    }
    else
    {
    }
}


/**
 * @brief
 * This function initializes the console tasks and resources.
 * @return true
 * @return false
 */
bool task_console_init(void)
{
    /* Register a function for the UART ISR*/
    cyhal_uart_register_callback(
        &cy_retarget_io_uart_obj,           // UART object
        console_event_handler,         // Event handler
        NULL                       // Handler argument
    );

    /* Initialize UART Rx Resources */
    if (!task_console_resources_init_rx())
    {
        return false; // Initialization failed
    }

    /* Initialize UART Tx Resources */
    if (!task_console_resources_init_tx())
    {
        return false; // Initialization failed
    }
    else
    {
        // Enable UART Rx Interrupts
        cyhal_uart_enable_event(
            &cy_retarget_io_uart_obj, 
            CYHAL_UART_IRQ_RX_NOT_EMPTY, 
            7, 
            true);
    }
    
    return true; // Initialization successful
}
#endif  