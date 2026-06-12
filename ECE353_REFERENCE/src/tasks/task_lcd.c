/**
 * @file task_lcd.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-18
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #include "task_lcd.h"

#ifdef ECE353_FREERTOS  
/* FreeRTOS Queue for LCD messages */
QueueHandle_t xQueue_LCD;
QueueHandle_t xQueue_LCD_rx; //return queue
/* LCD Task */
void task_lcd(void *pvParameters)
{
    (void)pvParameters; // Unused parameter

    lcd_msg_t lcd_msg;
    lcd_cmd_status_t lcd_rtrn_msg;

    while(1)
    {
        if(xQueueReceive(xQueue_LCD, &lcd_msg, portMAX_DELAY)==pdTRUE){
            // Process the received LCD message
            switch(lcd_msg.command)
            {
                case LCD_CMD_CLEAR_SCREEN:
                    lcd_clear_screen(LCD_COLOR_BLACK);
                    break;

                case LCD_CONSOLE_DRAW_MESSAGE:
                    if(!lcd_console_draw_string(&lcd_msg.payload.console, 1))
                    {
                        // Failed to draw string
                        printf("Failed to draw console message: %s\n", lcd_msg.payload.console.message);
                    }
                    break;
                case LCD_CMD_DRAW_BOARD:
                    if(!battleship_draw_game_board(player_id))
                    {
                        // Failed to draw game board
                        printf("Failed to draw battleship game board\n");
                    }
                    break;
                case LCD_CMD_DRAW_CURSOR:
                    // Draw LCD rectangle writing
                    if(!battleship_draw_cursor(lcd_msg.payload.battleship.col, lcd_msg.payload.battleship.row, player_id))
                    {
                        // Failed to draw cursor
                        printf("Failed to draw battleship cursor\n");
                    }
                    break;
                case LCD_CMD_DRAW_HIT:
                    // Draw hit marker
                    if(!battleship_draw_hit_marker(lcd_msg.payload.battleship.col, lcd_msg.payload.battleship.row, player_id))
                    {
                        // Failed to draw hit marker
                        printf("Failed to draw battleship hit marker\n");
                    }
                    break;
                case LCD_CMD_DRAW_MISS:
                    // Draw miss marker
                    if(!battleship_draw_miss_marker(lcd_msg.payload.battleship.col, lcd_msg.payload.battleship.row, player_id))
                    {
                        // Failed to draw miss marker
                        printf("Failed to draw battleship miss marker\n");
                    }
                    break;
                case LCD_CMD_REM_CURSOR:
                    if(!battleship_clear_cursor(lcd_msg.payload.battleship.col, lcd_msg.payload.battleship.row, player_id))
                    {
                        // Failed to draw cursor
                        printf("Failed to erase battleship cursor\n");
                    }
                    break;
                case LCD_CMD_DRAW_SHIP:
                    if(!battleship_draw_ship(
                        lcd_msg.payload.battleship.col,
                        lcd_msg.payload.battleship.row,
                        lcd_msg.payload.battleship.border_color,
                        lcd_msg.payload.battleship.fill_color,
                        lcd_msg.payload.battleship.type,
                        lcd_msg.payload.battleship.horizontal,
                        player_id))
                    {
                        // Failed to draw ship
                        printf("Failed to draw battleship ship\n");
                        lcd_rtrn_msg = LCD_CMD_STATUS_ERROR;
                        xQueueSend(lcd_msg.response_queue, &lcd_rtrn_msg, portMAX_DELAY);
                    }else{
                        // Ship drawn successfully - check for SW2 to save
                        // Use non-blocking check to avoid blocking the LCD task
                        EventBits_t events;

                        events = xEventGroupWaitBits(ECE353_RTOS_Events,
                                    ECE353_RTOS_EVENTS_SW2,
                                    pdTRUE,
                                    pdFALSE,
                                    pdMS_TO_TICKS(100));  // Reduced timeout to 100ms
                        if(events & ECE353_RTOS_EVENTS_SW2){
                            // Save the ship at the CURRENT message position (not cursor position)
                            if(!battleship_save_ship(
                                lcd_msg.payload.battleship.col,
                                lcd_msg.payload.battleship.row,
                                lcd_msg.payload.battleship.border_color,
                                lcd_msg.payload.battleship.fill_color,
                                lcd_msg.payload.battleship.type,
                                lcd_msg.payload.battleship.horizontal)){
                                printf("Failed to save battleship ship\n");
                                lcd_rtrn_msg = LCD_CMD_STATUS_ERROR;
                                xQueueSend(lcd_msg.response_queue, &lcd_rtrn_msg, portMAX_DELAY);
                            }else{
                                printf("Ship saved at col=%d, row=%d, type=%d, horiz=%d\n",
                                       lcd_msg.payload.battleship.col,
                                       lcd_msg.payload.battleship.row,
                                       lcd_msg.payload.battleship.type,
                                       lcd_msg.payload.battleship.horizontal);
                                // Notify task_imu_monitor that ship was placed
                                xEventGroupSetBits(ECE353_RTOS_Events, ECE353_RTOS_EVENTS_SHIP_PLACED);
                            }

                        }
                        // Removed the else print - it's too noisy
                    }
                    break;
                case LCD_CMD_ERASE_SHIP:
                    if(!battleship_clear_ship(&lcd_msg.payload.battleship, player_id)){
                         // Failed to draw ship
                        printf("Failed to erase battleship ship\n");
                        lcd_rtrn_msg = LCD_CMD_STATUS_ERROR;
                        xQueueSend(lcd_msg.response_queue, &lcd_rtrn_msg, portMAX_DELAY);
                    }else{
                        //continue do nothing
                    }
                    break;
                default:
                    // Unknown command
                    lcd_rtrn_msg = LCD_CMD_STATUS_ERROR;
                    //xQueueSend(lcd_msg.response_queue, &lcd_rtrn_msg, portMAX_DELAY);
                    break;
            }
        }

        
    }
}

/* LCD Task Initialization */
bool task_lcd_init(void){

    BaseType_t result;
    xQueue_LCD = xQueueCreate(10, sizeof(lcd_msg_t));
    if(xQueue_LCD == NULL)
    {
        return false;
    }
    
    //create a queue for receiving messages from the LCD gatekeeper
    xQueue_LCD_rx = xQueueCreate(10, sizeof(lcd_msg_t));
    if(xQueue_LCD_rx == NULL)
    {
        return false;
    }

    result= xTaskCreate(
        task_lcd,                       // Task function
        "LCD Task",                     // Task name
        5*configMINIMAL_STACK_SIZE,    // Stack size
        NULL,                           // Task parameters
        tskIDLE_PRIORITY+1,              // Task priority
        NULL                            // Task handle
    );

    if(result != pdPASS)
    {
        return false;
    }   

    return true;
}
#endif