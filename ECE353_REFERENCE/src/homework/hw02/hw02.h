/**
 * @file hw02.h
 * @author 
 * @brief Header file for ECE353 HW02 LCD Gatekeeper
 * @version 0.2
 * @date 2025-10-21
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __HW02_H__
#define __HW02_H__

#include "main.h"

#if defined(HW02)

#include "drivers.h"
#include "rtos_events.h"
#include "task_lcd.h"
#include "task_buttons.h"
#include "task_joystick.h"

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/
#define EVENT_LCD_READY   (1 << 0)   // Bit flag indicating LCD setup complete

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
extern EventGroupHandle_t ECE353_RTOS_Events;
extern QueueHandle_t xQueue_LCD_response;

//declare
uint8_t player_id;

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/**
 * @brief System control task for HW02
 * 
 * This task handles all LCD-related behavior such as
 * clearing the screen, drawing the game board, ships,
 * and printing the hits/misses.
 */
void task_hw02_system_control(void *pvParameters);

/**
 * @brief LCD gatekeeper task
 * 
 * This task waits until the LCD is ready and then
 * handles console messages (e.g., printing "Hits" and "Misses").
 */
void gatekeeper_task(void *pvParameters);

/**
 * @brief Initialize all hardware for HW02
 * 
 * Sets up the LCD and console peripherals.
 */
void app_init_hw(void);

/**
 * @brief Application entry point for HW02
 * 
 * Creates tasks and starts the FreeRTOS scheduler.
 */
void app_main(void);

#endif /* defined(HW02) */

#endif /* __HW02_H__ */
