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
#include "main.h"
#include <stdbool.h>

#if defined(ECE353_FREERTOS)
#include "task_ipc.h"
#include "task_console.h"

/* Globals */
TaskHandle_t TaskHandle_IPC_Rx = NULL;

//create Queuehandle
QueueHandle_t Queue_IPC_Rx = NULL;

/* Use a double buffering strategy for IPC packets */
static volatile ipc_packet_t IPC_Rx_Buffer0;
static volatile ipc_packet_t IPC_Rx_Buffer1;

volatile ipc_packet_t* volatile IPC_Rx_Produce_Buffer = &IPC_Rx_Buffer0;
volatile ipc_packet_t* volatile IPC_Rx_Consume_Buffer = &IPC_Rx_Buffer1;

/**
 * @brief
 *
 * This task is used to process received IPC packets.  The task will block
 * on a FreeRTOS Task Notification.  When a notification is received,
 * the task will process the IPC packet stored in the consume buffer.
 *
 * For validation purposes, the task will print out the contents of the
 * received IPC packet to the console.
 * 
 * @param arg
 * Unused parameter
 */
void task_ipc_rx(void *param)
{
    while(1)
    {
        // Wait for a FreeRTOS Task Notification
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Print out a message to the console indicating what type of packet and any
        // useful information contained in the packet.
        if(validate_packet(IPC_Rx_Consume_Buffer))
        {
            switch(IPC_Rx_Consume_Buffer->cmd)
            {
                case IPC_CMD_FIRE:
                    printf("IPC Rx: FIRE Packet Received: Row=%d, Col=%d\r\n",
                        IPC_Rx_Consume_Buffer->fire.row,
                        IPC_Rx_Consume_Buffer->fire.col);
                    break;
                case IPC_CMD_RESULT:
                    printf("IPC Rx: RESULT Packet Received: Result=%d\r\n",
                        IPC_Rx_Consume_Buffer->result);
                    break;
                case IPC_CMD_GAME_CONTROL:
                {
                    uint8_t ctrl = (uint8_t)IPC_Rx_Consume_Buffer->game_control;
                    printf("IPC Rx: GAME CONTROL Packet Received: Control=%d (0x%02X)\r\n", ctrl, ctrl);
                    
                    if(ctrl == (uint8_t)IPC_GAME_CONTROL_NEW_GAME)  // 0xC0
                    {
                        printf("-> NEW_GAME, setting ECE353_RTOS_IPC_RX\r\n");
                        xEventGroupSetBits(ECE353_RTOS_Events, ECE353_RTOS_IPC_RX);
                    }
                    else if(ctrl == (uint8_t)IPC_GAME_CONTROL_PLAYER_READY)  // 0xC1
                    {
                        printf("-> PLAYER_READY, setting ECE353_RTOS_ALL_PLAYERS_READY_IPC\r\n");
                        xEventGroupSetBits(ECE353_RTOS_Events, ECE353_RTOS_ALL_PLAYERS_READY_IPC);
                    }
                    else if(ctrl == (uint8_t)IPC_GAME_CONTROL_ACK)  // 0xC4
                    {
                        printf("-> ACK, setting ECE353_RTOS_ALL_PLAYERS_READY_ACK or game ended ACK\r\n");
                        xEventGroupSetBits(ECE353_RTOS_Events, ECE353_RTOS_ALL_PLAYERS_READY_ACK);
                    }
                    else if(ctrl == (uint8_t)IPC_GAME_CONTROL_END_GAME)  // 0xC5
                    {
                        printf("-> END_GAME received\r\n");
                        // Additional handling for END_GAME can be added here
                        xEventGroupSetBits(ECE353_RTOS_Events, ECE353_RTOS_GAME_WON_ACK);
                    }
                    else
                    {
                        printf("-> Unknown control value!\r\n");
                    }
                    break;
                }
                case IPC_CMD_ERROR:
                    printf("IPC Rx: ERROR Packet Received: Error Code=%d\r\n",
                        IPC_Rx_Consume_Buffer->error);
                    break;
                default:
                    printf("IPC Rx: Unknown Packet Received\r\n");
                    break;
                
            }
            // Send the valid packet to the Queue for further processing by other tasks
            xQueueSend(Queue_IPC_Rx, (void*)IPC_Rx_Consume_Buffer, portMAX_DELAY);
            
        }
        else
        {
            task_console_printf("IPC Rx: Invalid Packet Received\r\n");
        }
    }
}

bool task_ipc_resources_init_rx(void)
{
    //create the freertos queue
    Queue_IPC_Rx = xQueueCreate(10, sizeof(ipc_packet_t));
    if(Queue_IPC_Rx == NULL)
    {
        return false;
    }

    // Create the IPC Rx Task
    BaseType_t task_ipc_rx_status = xTaskCreate(
        task_ipc_rx,                 // Function that implements the task.
        "IPC Rx Task",               // Text name for the task.
        IPC_STACK_SIZE,    // Stack size in words, not bytes.
        NULL,                       // Parameter passed into the task.
        IPC_PRIORITY,       // Priority at which the task is created.
        &TaskHandle_IPC_Rx          // Used to pass out the created task's handle.
    );

    if(task_ipc_rx_status != pdPASS)
    {
        return false;
    }

    return true;    
}

#endif