/**
 * @file task_console.h
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-15
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef __TASK_IPC_H__
#define __TASK_IPC_H__

#include "main.h"

#if defined(ECE353_FREERTOS)
#include <sys/types.h>
#include "cyhal_uart.h"
#include <stdint.h>
#include <stdarg.h>
#include "drivers.h"
#include "portmacro.h"
#include "rtos_events.h"

#define IPC_STACK_SIZE (5*configMINIMAL_STACK_SIZE)
#define IPC_PRIORITY (tskIDLE_PRIORITY + 1)

#define IPC_TX_CIRCULAR_BUFFER_SIZE 128
#define IPC_TX_QUEUE_LENGTH 10

#define INT_PRIORITY_IPC 5 // Priority for IPC tasks and events

#define IPC_PACKET_START 0xAA

#define PIN_IPC_UART_TX   P3_1
#define PIN_IPC_UART_RX   P3_0

// //Establishing a double buffer for standard practice
// typedef struct {
//     char *data; // Buffer to hold the input string
//     uint8_t index;    // Current index in the buffer
// } console_buffer_t;

// // Global variables used for receiving data
// extern console_buffer_t console_buffer1;
// extern console_buffer_t console_buffer2;
// extern console_buffer_t *produce_console_buffer;
// extern console_buffer_t *consume_console_buffer;
// extern TaskHandle_t TaskHandle_Console_Rx;


typedef enum
{
    IPC_CMD_FIRE,              // User fired at opponent
    IPC_CMD_RESULT,            // Result reported by opponent
    IPC_CMD_GAME_CONTROL,      // Game control command (e.g., new game, player ready, etc.) 
    IPC_CMD_ERROR,             // Error message
} ipc_cmd_t;

typedef struct {
    uint8_t row;
    uint8_t col;
} ipc_fire_payload_t;

typedef enum
{
    IPC_RESULT_MISS                 = 0xB0,
    IPC_RESULT_HIT                  = 0xB1,
    IPC_RESULT_SUNK                 = 0xB2,
} ipc_result_t;

// Used to indicate specific game control commands
typedef enum
{
    IPC_GAME_CONTROL_NEW_GAME       = 0xC0,
    IPC_GAME_CONTROL_PLAYER_READY   = 0xC1,
    IPC_GAME_CONTROL_PLAYER_ALIVE   = 0xC2,
    IPC_GAME_CONTROL_PASS_TURN      = 0xC3,
    IPC_GAME_CONTROL_ACK            = 0xC4,
    IPC_GAME_CONTROL_END_GAME       = 0xC5,
} ipc_game_control_t;

typedef enum
{
    IPC_ERROR_CHECKSUM              = 0xE0,
    IPC_ERROR_COORD_INVALID         = 0xE1,
    IPC_ERROR_COORD_OCCUPIED        = 0xE2,
    IPC_ERROR_SYSTEM_FAILURE        = 0xE3,
} ipc_error_t;

typedef struct
{
    uint8_t start_byte; // Should be IPC_PACKET_START
    ipc_cmd_t cmd;
    union {
        ipc_fire_payload_t fire;
        ipc_result_t result;
        ipc_game_control_t game_control;
        ipc_error_t error;
    };
    uint8_t checksum;
} ipc_packet_t;


/* IPC UART Globals*/
extern cyhal_uart_t IPC_Uart_Obj;
extern cyhal_uart_cfg_t IPC_Uart_Config;

/* Globals used for receiving data */
extern volatile ipc_packet_t* volatile IPC_Rx_Produce_Buffer;
extern volatile ipc_packet_t* volatile IPC_Rx_Consume_Buffer;
extern TaskHandle_t TaskHandle_IPC_Rx;

/* Globals used for receiving data */
extern QueueHandle_t Queue_IPC_Rx;

/* Globals used for transmitting data */
extern QueueHandle_t Queue_IPC_Tx;
extern TaskHandle_t TaskHandle_IPC_Tx;
extern circular_buffer_t *Circular_Buffer_IPC_TX;

/* Globals Debug Messages*/
bool task_ipc_resources_init_rx(void);
bool task_ipc_resources_init_tx(void);
bool task_ipc_init(void);

/**
 * @brief 
 * Validates the given IPC packet by checking the start byte and checksum
 * @param packet 
 * @return __inline 
 */
bool validate_packet(ipc_packet_t* packet);

bool ipc_send_fire(uint8_t row, uint8_t col);
bool ipc_send_result(ipc_result_t result);
bool ipc_send_game_control(ipc_game_control_t control);
bool ipc_send_error(ipc_error_t error);

#endif /* ECE353_FREER
TOS */

#endif /* __TASK_IPC_H__ */