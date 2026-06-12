/**
 * @file task_joystick.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-14
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "main.h"

#ifdef ECE353_FREERTOS  
#include "drivers.h"
 #include "task_joystick.h"

 QueueHandle_t Queue_Joystick = NULL; //allocated global variable

/* Message lookup table for joystick positions */
const char * const joystick_pos_names[] = {
    "Center",
    "Left",
    "Right",
    "Up",
    "Down",
    "Upper Left",
    "Upper Right",
    "Lower Left",
    "Lower Right"
};

 /**
  * @brief 
  *  Task used to monitor the joystick
  * @param arg 
  */
 void task_joystick(void *arg)
{
    (void)arg; // Unused parameter

    uint16_t x_value, y_value;
    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(500)); //Delay for 500ms

        x_value = joystick_read_x();
        y_value = joystick_read_y();

        //Convert to ADC values to a voltage
        float x_voltage = (x_value / 65535.0) * 3.3;
        float y_voltage = (y_value / 65535.0) * 3.3;

        //Add a series of positions to detect where joystick at
        joystick_position_t position = joystick_get_pos();

        //current position != prev position add to queue
        if(Queue_Joystick != NULL)
        {
            xQueueOverwrite(Queue_Joystick, &position);
        }

        printf("Joystick X: %.2f V, Y: %.2f V\n\r", x_voltage, y_voltage);
    }
}


bool task_joystick_init(void)
{
    /* Create the Queue used to send Joystick Positions*/
    Queue_Joystick = xQueueCreate(1, sizeof(joystick_position_t));

    /* Create the joystick task */
    
    return true;
}
#endif