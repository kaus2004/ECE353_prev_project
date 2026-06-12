/**
 * @file ice08.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(ICE08)
#include "drivers.h"        
#include "rtos_events.h"
#include "task_buttons.h"
#include "task_lcd.h"
#include "task_joystick.h"
//Trying to prevent Miss/Hit printed before final ship is drawn
#define EVENT_LCD_READY (1 << 0)

char APP_DESCRIPTION[] = "ECE353: ICE 08 - FreeRTOS LCD Gatekeeper";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
extern EventGroupHandle_t ECE353_RTOS_Events;
/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/
void task_system_control(void *pvParameters)
{
    (void)pvParameters; // Unused parameter
    //clear lcd screen
    EventBits_t events;
    lcd_msg_t lcd_msg;
    int8_t counter = 0;
    int8_t row = 0;
    int8_t col = 0;
    int8_t prev_row = 0;
    int8_t prev_col = 0;

    //return message
    lcd_cmd_status_t return_message;

    int8_t button_presses = 0;
    // Clear the screen
    lcd_msg.command = LCD_CMD_CLEAR_SCREEN;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

    //Draw the initial game board
    // Draw the game board
    lcd_msg.command = LCD_CMD_DRAW_BOARD;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

     // Draw the initial cursor
    lcd_msg.command = LCD_CMD_DRAW_CURSOR;
    lcd_msg.payload.battleship.row = row;
    lcd_msg.payload.battleship.col = col;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

    // Entering infinite loop

    while(1)
    {
        // Sleep for 100 ms
        vTaskDelay(pdMS_TO_TICKS(40));
        // Remove old cursor position:
        // Store previous position
        prev_row = row;
        prev_col = col;

        //update cursor position without events just row and columns
        if(col < 9)
        {
            col++;
        }
        else
        {
            col = 0;
            if(row < 9)
            {
                row++;
            }
            else
            {
                row = 0;
            }
        }
        // Remove previous cursor
        lcd_msg.command = LCD_CMD_REM_CURSOR;
        lcd_msg.payload.battleship.row = prev_row;
        lcd_msg.payload.battleship.col = prev_col;
        xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

        // Draw the cursor
        lcd_msg.command = LCD_CMD_DRAW_CURSOR;
        lcd_msg.payload.battleship.col = col;
        lcd_msg.payload.battleship.row = row;
        xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
        counter++;

        if(counter >100){
            break;
        }
    }
    //We go to the game and draw the screen with ships

    // Clear the screen
    lcd_msg.command = LCD_CMD_CLEAR_SCREEN;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

    // Draw the game board
    lcd_msg.command = LCD_CMD_DRAW_BOARD;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

    // Sleep for 100 ms
    vTaskDelay(pdMS_TO_TICKS(100));

    //Draw the carrier (9,0)
    lcd_msg.command = LCD_CMD_DRAW_SHIP;
    lcd_msg.payload.battleship.col = 0;
    lcd_msg.payload.battleship.row = 9;
    lcd_msg.payload.battleship.border_color = BATTLESHIP_PLAYER_0_COLOR;
    lcd_msg.payload.battleship.fill_color = LCD_COLOR_GRAY;
    lcd_msg.payload.battleship.type = BATTLESHIP_TYPE_CARRIER;
    lcd_msg.payload.battleship.horizontal = true;

    /* IMPORTANT: attach the response queue handle */
    lcd_msg.response_queue = xQueue_LCD_rx;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

    xQueueReceive(xQueue_LCD_rx, &return_message, portMAX_DELAY);
    if(return_message == LCD_CMD_STATUS_SUCCESS){
        printf("Ship Printed Successfully\n\r");
    }else{
        printf("Task Failed to print\n\r");
    }

    vTaskDelay(pdMS_TO_TICKS(100));
    //Draw the battleship (0,0)
    lcd_msg.command = LCD_CMD_DRAW_SHIP;
    lcd_msg.payload.battleship.col = 0;
    lcd_msg.payload.battleship.row = 0;
    lcd_msg.payload.battleship.border_color = BATTLESHIP_PLAYER_0_COLOR;
    lcd_msg.payload.battleship.fill_color = LCD_COLOR_GRAY;
    lcd_msg.payload.battleship.type = BATTLESHIP_TYPE_BATTLESHIP;
    lcd_msg.payload.battleship.horizontal = true;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);


    xQueueReceive(xQueue_LCD_rx, &return_message, portMAX_DELAY);
    if(return_message == LCD_CMD_STATUS_SUCCESS){
        printf("Ship Printed Successfully\n\r");
    }else{
        printf("Task Failed to print\n\r");
    }

    vTaskDelay(pdMS_TO_TICKS(100));
    //Draw the Cruiser (7,7)
    lcd_msg.command = LCD_CMD_DRAW_SHIP;
    lcd_msg.payload.battleship.col = 7;
    lcd_msg.payload.battleship.row = 7;
    lcd_msg.payload.battleship.border_color = BATTLESHIP_PLAYER_0_COLOR;
    lcd_msg.payload.battleship.fill_color = LCD_COLOR_GRAY;
    lcd_msg.payload.battleship.type = BATTLESHIP_TYPE_CRUISER;
    lcd_msg.payload.battleship.horizontal = false;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

    xQueueReceive(xQueue_LCD_rx, &return_message, portMAX_DELAY);
    if(return_message == LCD_CMD_STATUS_SUCCESS){
        printf("Ship Printed Successfully\n\r");
    }else{
        printf("Task Failed to print\n\r");
    }

    vTaskDelay(pdMS_TO_TICKS(100));
    //Draw the Destroyer (2,2)
    lcd_msg.command = LCD_CMD_DRAW_SHIP;
    lcd_msg.payload.battleship.col = 2;
    lcd_msg.payload.battleship.row = 2;
    lcd_msg.payload.battleship.border_color = BATTLESHIP_PLAYER_0_COLOR;
    lcd_msg.payload.battleship.fill_color = LCD_COLOR_GRAY;
    lcd_msg.payload.battleship.type = BATTLESHIP_TYPE_DESTROYER;
    lcd_msg.payload.battleship.horizontal = false;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

    xQueueReceive(xQueue_LCD_rx, &return_message, portMAX_DELAY);
    if(return_message == LCD_CMD_STATUS_SUCCESS){
        printf("Ship Printed Successfully\n\r");
    }else{
        printf("Task Failed to print\n\r");
    }

    vTaskDelay(pdMS_TO_TICKS(100));
    //Draw the Submarine (5,5)
    lcd_msg.command = LCD_CMD_DRAW_SHIP;
    lcd_msg.payload.battleship.col = 5;
    lcd_msg.payload.battleship.row = 5;
    lcd_msg.payload.battleship.border_color = BATTLESHIP_PLAYER_0_COLOR;
    lcd_msg.payload.battleship.fill_color = LCD_COLOR_GRAY;
    lcd_msg.payload.battleship.type = BATTLESHIP_TYPE_SUBMARINE;
    lcd_msg.payload.battleship.horizontal = true;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

    xQueueReceive(xQueue_LCD_rx, &return_message, portMAX_DELAY);
    if(return_message == LCD_CMD_STATUS_SUCCESS){
        printf("Ship Printed Successfully\n\r");
    }else{
        printf("Task Failed to print\n\r");
    }

    vTaskDelay(pdMS_TO_TICKS(100)); //always add delay after a request

    //Draw the incorrect ship placements at
    //draw battleship at (row =0, col = 7) horizontal should be invalid
    lcd_msg.command = LCD_CMD_DRAW_SHIP;
    lcd_msg.payload.battleship.col = 7;
    lcd_msg.payload.battleship.row = 0;
    lcd_msg.payload.battleship.border_color = BATTLESHIP_PLAYER_0_COLOR;
    lcd_msg.payload.battleship.fill_color = LCD_COLOR_GRAY;
    lcd_msg.payload.battleship.type = BATTLESHIP_TYPE_BATTLESHIP;
    lcd_msg.payload.battleship.horizontal = true;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

    xQueueReceive(xQueue_LCD_rx, &return_message, portMAX_DELAY);
    if(return_message == LCD_CMD_STATUS_SUCCESS){
        printf("Ship Printed Successfully\n\r");
    }else{
        printf("Task Failed to print\n\r");
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    //draw submarine at (row= 8, col = 0) vertical should be invalid
    lcd_msg.command = LCD_CMD_DRAW_SHIP;
    lcd_msg.payload.battleship.col = 0;
    lcd_msg.payload.battleship.row = 8;
    lcd_msg.payload.battleship.border_color = BATTLESHIP_PLAYER_0_COLOR;
    lcd_msg.payload.battleship.fill_color = LCD_COLOR_GRAY;
    lcd_msg.payload.battleship.type = BATTLESHIP_TYPE_SUBMARINE;
    lcd_msg.payload.battleship.horizontal = false;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

    xQueueReceive(xQueue_LCD_rx, &return_message, portMAX_DELAY);
    if(return_message == LCD_CMD_STATUS_SUCCESS){
        printf("Ship Printed Successfully\n\r");
    }else{
        printf("Task Failed to print\n\r");
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    //draw carrier at ( row = 15, col 0) vertical
    lcd_msg.command = LCD_CMD_DRAW_SHIP;
    lcd_msg.payload.battleship.col = 0;
    lcd_msg.payload.battleship.row = 15;
    lcd_msg.payload.battleship.border_color = BATTLESHIP_PLAYER_0_COLOR;
    lcd_msg.payload.battleship.fill_color = LCD_COLOR_GRAY;
    lcd_msg.payload.battleship.type = BATTLESHIP_TYPE_CARRIER;
    lcd_msg.payload.battleship.horizontal = false;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
    
    xQueueReceive(xQueue_LCD_rx, &return_message, portMAX_DELAY);
    if(return_message == LCD_CMD_STATUS_SUCCESS){
        printf("Ship Printed Successfully\n\r");
    }else{
        printf("Task Failed to print\n\r");
    }
    vTaskDelay(pdMS_TO_TICKS(100));

    // Signal that LCD setup is complete
    xEventGroupSetBits(ECE353_RTOS_Events, EVENT_LCD_READY);
    
    vTaskDelay(pdMS_TO_TICKS(100));

    while(1){
        //Task does nothing and never returns

    }
}

void gatekeeper_task(void *pvParameters)
{
    (void)pvParameters;
    lcd_msg_t lcd_msg = {0};

    //wait till bits are ready
    // Wait until LCD is fully ready
    xEventGroupWaitBits(
        ECE353_RTOS_Events,
        EVENT_LCD_READY,
        pdFALSE,        // Don't clear the bit
        pdTRUE,         // Wait for all bits in the mask
        portMAX_DELAY   // Wait indefinitely
    );
    // -------- Message 1 --------
    const char *msg1 = "Hits: 5";
    size_t len1 = strlen(msg1);

    lcd_msg.payload.console.message = pvPortMalloc(len1 + 1);
    if (lcd_msg.payload.console.message == NULL)
    {
        printf("Failed to allocate memory for LCD message 1.\n");
        vTaskDelete(NULL);
    }

    strcpy(lcd_msg.payload.console.message, msg1);
    lcd_msg.payload.console.length = len1;
    lcd_msg.payload.console.x_offset = 230;
    lcd_msg.payload.console.y_offset = 40;
    lcd_msg.command = LCD_CONSOLE_DRAW_MESSAGE;

    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

    //vPortFree(lcd_msg.payload.console.message);

    vTaskDelay(pdMS_TO_TICKS(50));  // wait 0.5 seconds

    // -------- Message 2 --------
    const char *msg2 = "Miss: 3";
    size_t len2 = strlen(msg2);

    lcd_msg.payload.console.message = pvPortMalloc(len2 + 1);
    if (lcd_msg.payload.console.message == NULL)
    {
        printf("Failed to allocate memory for LCD message 2.\n");
        vTaskDelete(NULL);
    }

    strcpy(lcd_msg.payload.console.message, msg2);
    lcd_msg.payload.console.length = len2;
    lcd_msg.payload.console.x_offset = 230;
    lcd_msg.payload.console.y_offset = 80;  // Move down a bit
    lcd_msg.command = LCD_CONSOLE_DRAW_MESSAGE;

    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

    vTaskDelay(pdMS_TO_TICKS(50));

    //vPortFree(lcd_msg.payload.console.message);
    int count=0;
    while(1){
        vTaskDelay(pdMS_TO_TICKS(50));
        //printf("Reached here %i",count++);
        //never ends the task
    }
}


/**
 * @brief
 * This function will initialize all of the hardware resources for
 * the ICE
 */
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
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }
}

/*****************************************************************************/
/* Application Code                                                          */
/*****************************************************************************/
/**
 * @brief
 * This function implements the behavioral requirements for the ICE
 */
void app_main(void)
{
    
    /* Register the tasks with FreeRTOS*/

    ECE353_RTOS_Events = xEventGroupCreate();

    /* Initialize LCD resources */
    if (!task_lcd_init())
    {
        printf("Failed to initialize joystick task\n\r");
        for(int i = 0; i < 100000; i++) {}
       CY_ASSERT(0); // If the task initialization fails, assert
    }

    /* Create the system control task */
    xTaskCreate(
        task_system_control,
        "Task System Control",
        configMINIMAL_STACK_SIZE * 5,
        NULL,
        tskIDLE_PRIORITY + 2,
        NULL
    );

    /* Create the LCD gatekeeper task */
    xTaskCreate(
        gatekeeper_task,
        "Task LCD Gatekeeper",
        configMINIMAL_STACK_SIZE * 5,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL
    );

    /* Start the buttons task*/
    // xTaskCreate(
    //     task_buttons, 
    //     "Task Buttons", 
    //     configMINIMAL_STACK_SIZE, 
    //     NULL, 
    //     tskIDLE_PRIORITY + 2, 
    //     NULL
    // );


    /* Start the scheduler*/
    vTaskStartScheduler();

    /* Will never reach this loop once the scheduler starts */
    while (1)
    {
    }
}
#endif
