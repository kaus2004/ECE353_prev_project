/**
 * @file hw02.c
 * @author Joe Krachey
 * @brief  ECE353 F25 HW02 -- LCD Gatekeeper
 * @version 0.1
 * @date 2025-10-08
 *
 * @copyright Copyright (c) 2025
 */

#include "hw02.h"
#include "battleship.h"
#include "main.h"
#include "projdefs.h"
#include "task_lcd.h"
#include "rtos_events.h"
#include <stdbool.h>
#include <string.h>

#if defined(HW02)

/*****************************************************************************/
/* Description                                                               */
/*****************************************************************************/
char APP_DESCRIPTION[] = "ECE353 F25 HW02 -- LCD Gatekeeper";

/*****************************************************************************/
/* Globals                                                                   */
/*****************************************************************************/
extern EventGroupHandle_t ECE353_RTOS_Events;
#define EVENT_LCD_READY (1 << 0)

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

void task_hw02_system_control(void *pvParameters)
{
    (void)pvParameters;

    EventBits_t events;
    lcd_msg_t lcd_msg;
    lcd_cmd_status_t return_message;

    int8_t counter = 0;
    int8_t row = 0;
    int8_t col = 0;
    int8_t prev_row = 0;
    int8_t prev_col = 0;
    player_id = 0; // Default to player 0

    // --- INITIAL LCD SETUP ---
    lcd_msg.command = LCD_CMD_CLEAR_SCREEN;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

    lcd_msg.command = LCD_CMD_DRAW_BOARD;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

    lcd_msg.command = LCD_CMD_DRAW_CURSOR;
    lcd_msg.payload.battleship.row = row;
    lcd_msg.payload.battleship.col = col;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

    // --- CURSOR DEMO ---
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(40));

        prev_row = row;
        prev_col = col;

        if (col < 9)
            col++;
        else
        {
            col = 0;
            if (row < 9)
                row++;
            else
                row = 0;
        }

        // Remove old cursor
        lcd_msg.command = LCD_CMD_REM_CURSOR;
        lcd_msg.payload.battleship.row = prev_row;
        lcd_msg.payload.battleship.col = prev_col;
        xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

        // Draw new cursor
        lcd_msg.command = LCD_CMD_DRAW_CURSOR;
        lcd_msg.payload.battleship.row = row;
        lcd_msg.payload.battleship.col = col;
        xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

        counter++;
        if (counter > 100)
            break;
    }

    // --- MAIN GAME SETUP ---
    lcd_msg.command = LCD_CMD_CLEAR_SCREEN;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

    lcd_msg.command = LCD_CMD_DRAW_BOARD;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

    vTaskDelay(pdMS_TO_TICKS(100));

    // Reuse response queue handle
    lcd_msg.response_queue = xQueue_LCD_rx;

    // Helper macro for cleaner sends + receive
#define SEND_SHIP_AND_WAIT()                                      \
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);              \
    xQueueReceive(xQueue_LCD_rx, &return_message, portMAX_DELAY); \
    if (return_message == LCD_CMD_STATUS_SUCCESS)                 \
        printf("Ship Printed Successfully\n\r");                  \
    else                                                          \
        printf("Task Failed to print\n\r");                       \
    vTaskDelay(pdMS_TO_TICKS(100))

    // Draw Carrier (9,0)
    lcd_msg.command = LCD_CMD_DRAW_SHIP;
    lcd_msg.payload.battleship.col = 0;
    lcd_msg.payload.battleship.row = 9;
    lcd_msg.payload.battleship.border_color = BATTLESHIP_PLAYER_0_COLOR;
    lcd_msg.payload.battleship.fill_color = LCD_COLOR_GRAY;
    lcd_msg.payload.battleship.type = BATTLESHIP_TYPE_CARRIER;
    lcd_msg.payload.battleship.horizontal = true;
    SEND_SHIP_AND_WAIT();

    // Draw Battleship (0,0)
    lcd_msg.payload.battleship.col = 0;
    lcd_msg.payload.battleship.row = 0;
    lcd_msg.payload.battleship.type = BATTLESHIP_TYPE_BATTLESHIP;
    lcd_msg.payload.battleship.horizontal = true;
    SEND_SHIP_AND_WAIT();

    // Draw Cruiser (7,7)
    lcd_msg.payload.battleship.col = 7;
    lcd_msg.payload.battleship.row = 7;
    lcd_msg.payload.battleship.type = BATTLESHIP_TYPE_CRUISER;
    lcd_msg.payload.battleship.horizontal = false;
    SEND_SHIP_AND_WAIT();

    // Draw Destroyer (2,2)
    lcd_msg.payload.battleship.col = 2;
    lcd_msg.payload.battleship.row = 2;
    lcd_msg.payload.battleship.type = BATTLESHIP_TYPE_DESTROYER;
    lcd_msg.payload.battleship.horizontal = false;
    SEND_SHIP_AND_WAIT();

    // Draw Submarine (5,5)
    lcd_msg.payload.battleship.col = 5;
    lcd_msg.payload.battleship.row = 5;
    lcd_msg.payload.battleship.type = BATTLESHIP_TYPE_SUBMARINE;
    lcd_msg.payload.battleship.horizontal = true;
    SEND_SHIP_AND_WAIT();

    // Invalid ship tests
    lcd_msg.payload.battleship.col = 7;
    lcd_msg.payload.battleship.row = 0;
    lcd_msg.payload.battleship.type = BATTLESHIP_TYPE_BATTLESHIP;
    lcd_msg.payload.battleship.horizontal = true;
    SEND_SHIP_AND_WAIT();

    lcd_msg.payload.battleship.col = 0;
    lcd_msg.payload.battleship.row = 8;
    lcd_msg.payload.battleship.type = BATTLESHIP_TYPE_SUBMARINE;
    lcd_msg.payload.battleship.horizontal = false;
    SEND_SHIP_AND_WAIT();

    lcd_msg.payload.battleship.col = 0;
    lcd_msg.payload.battleship.row = 15;
    lcd_msg.payload.battleship.type = BATTLESHIP_TYPE_CARRIER;
    lcd_msg.payload.battleship.horizontal = false;
    SEND_SHIP_AND_WAIT();

#undef SEND_SHIP_AND_WAIT

    // Signal that LCD setup is complete
    xEventGroupSetBits(ECE353_RTOS_Events, EVENT_LCD_READY);

    while (1)
        vTaskDelay(pdMS_TO_TICKS(100));
}

void task_hw02_gatekeeper(void *pvParameters)
{
    (void)pvParameters;
    lcd_msg_t lcd_msg = {0};

    // Wait until LCD ready
    xEventGroupWaitBits(
        ECE353_RTOS_Events,
        EVENT_LCD_READY,
        pdFALSE,
        pdTRUE,
        portMAX_DELAY
    );

    // Message 1: Hits
    const char *msg1 = "Hits: 5";
    size_t len1 = strlen(msg1);

    lcd_msg.payload.console.message = pvPortMalloc(len1 + 1);
    strcpy(lcd_msg.payload.console.message, msg1);
    lcd_msg.payload.console.length = len1;
    lcd_msg.payload.console.x_offset = 230;
    lcd_msg.payload.console.y_offset = 40;
    lcd_msg.command = LCD_CONSOLE_DRAW_MESSAGE;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

    vTaskDelay(pdMS_TO_TICKS(50));

    // Message 2: Misses
    const char *msg2 = "Miss: 3";
    size_t len2 = strlen(msg2);

    lcd_msg.payload.console.message = pvPortMalloc(len2 + 1);
    strcpy(lcd_msg.payload.console.message, msg2);
    lcd_msg.payload.console.length = len2;
    lcd_msg.payload.console.x_offset = 230;
    lcd_msg.payload.console.y_offset = 80;
    lcd_msg.command = LCD_CONSOLE_DRAW_MESSAGE;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

    while (1)
        vTaskDelay(pdMS_TO_TICKS(100));
}

/*************************************************
 * @brief Initialize all hardware resources
 ************************************************/
void app_init_hw(void)
{
    cy_rslt_t rslt;

    console_init();
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    rslt = lcd_initialize();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("LCD initialization failed!\n\r");
        for (int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }
}

/*****************************************************************************/
/* Application Entry Point                                                   */
/*****************************************************************************/
void app_main(void)
{
    ECE353_RTOS_Events = xEventGroupCreate();

    if (!task_lcd_init())
    {
        printf("Failed to initialize LCD task\n\r");
        for (int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    xTaskCreate(
        task_hw02_system_control,
        "HW02 System Control",
        configMINIMAL_STACK_SIZE * 5,
        NULL,
        tskIDLE_PRIORITY + 2,
        NULL
    );

    xTaskCreate(
        task_hw02_gatekeeper,
        "HW02 Gatekeeper",
        configMINIMAL_STACK_SIZE * 5,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL
    );

    vTaskStartScheduler();

    while (1)
    {
    }
}

#endif
