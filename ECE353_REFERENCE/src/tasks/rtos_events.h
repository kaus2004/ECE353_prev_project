/**
 * @file rtos_events.h
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-13
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #ifndef __RTOS_EVENTS_H__
 #define __RTOS_EVENTS_H__
 #include "main.h"
 
#ifdef ECE353_FREERTOS

/*******************************************************************************
* Event Group for system events.
 ******************************************************************************/
extern EventGroupHandle_t ECE353_RTOS_Events;

/*******************************************************************************
* Macros used to define the system events
******************************************************************************/
#define ECE353_RTOS_EVENTS_SW1 (1 << 0)
#define EVENT_LCD_READY (1 << 0) //alias for SW1 to enable
#define ECE353_RTOS_EVENTS_SW2 (1 << 1)
#define ECE353_RTOS_EVENTS_SW3 (1 << 2)
#define ECE353_RTOS_IPC_RX (1 << 3)
#define ECE353_RTOS_EVENTS_SHIP_PLACED (1 << 4) // used to go to next ship once a ship placed
#define ECE353_RTOS_ALL_PLAYERS_READY_ACK (1 << 5)  // set when sent and received PLAYER READY from both players
                                                // while sending ACK 
#define ECE353_RTOS_ALL_PLAYERS_READY_IPC (1 << 6) // set when both players ready received or game ended
#define ECE353_RTOS_START_GAME (1 << 7) // set when both players ready received to start game

//#define ECE353_RTOS_GAME_OVER_ACK (1 << 8) // set when endgame ack received
#define ECE353_RTOS_GAME_WON_ACK (1 << 9) // set when endgame won, I send you send IPC ack received

#endif // ECE353_FREERTOS

#endif // __RTOS_EVENTS_H__