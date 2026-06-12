/**
 * @file task_buttons.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-13
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #include "task_buttons.h"
 #include "task_console.h" //never include a c file in another a c file

 

 #ifdef ECE353_FREERTOS

 EventGroupHandle_t ECE353_RTOS_Events;

 // contents of the header file
//  #define ECE353_RTOS_EVENTS_SW1 (1 << 0)
//  #define ECE353_RTOS_EVENTS_SW2 (1 << 1)
//  #define ECE353_RTOS_EVENTS_SW3 (1 << 2)
 
 /**
  * @brief 
  * Task used to debounce button presses (SW1, SW2, SW3).  
  * The falling edge of the button press is detected by de-bouncing
  * the button for 30mS. Each button should be sampled every 15mS.
  *
  * When a button press is detected, the corresponding event is set in
  * in the event group ECE353_RTOS_Events.
  *
  * @param arg 
  * Unused parameter
  */
 void task_buttons(void *arg)
 {
    (void)arg; // Unused parameter
    bool button_SW1_curr = false;
    bool button_SW1_prev = false;

    bool button_SW2_curr = false;
    bool button_SW2_prev = false;

    bool button_SW3_curr = false;
    bool button_SW3_prev = false;

    while (1)
    {
        uint8_t in_reg = PORT_BUTTON_SW1 -> IN;
        button_SW1_curr = (!(in_reg & MASK_BUTTON_PIN_SW1))? true : false ;
        button_SW2_curr = (!(in_reg & MASK_BUTTON_PIN_SW2))? true : false ;
        button_SW3_curr = (!(in_reg & MASK_BUTTON_PIN_SW3))? true : false ;

        if((button_SW1_curr == true) && (button_SW1_prev==false)){
            xEventGroupSetBits(ECE353_RTOS_Events,ECE353_RTOS_EVENTS_SW1);
        }
        if((button_SW2_curr == true) && (button_SW2_prev==false)){
            xEventGroupSetBits(ECE353_RTOS_Events,ECE353_RTOS_EVENTS_SW2);
        }
        if((button_SW3_curr == true) && (button_SW3_prev==false)){
            xEventGroupSetBits(ECE353_RTOS_Events,ECE353_RTOS_EVENTS_SW3);
            // Theme change now handled by GPIO interrupt instead
        }

        vTaskDelay(15);
        
        //give prev values curr values
        button_SW1_prev = button_SW1_curr;
        button_SW2_prev = button_SW2_curr;
        button_SW3_prev = button_SW3_curr;
        
    }
    
}

 /* Button Task Initialization */
bool task_button_init(void){

    BaseType_t result;

    // Create the button task
    result = xTaskCreate(
        task_buttons, 
        "Button Task", 
        configMINIMAL_STACK_SIZE, 
        NULL, 
        tskIDLE_PRIORITY + 1, 
        NULL
    );

    if(result != pdPASS)
    {
        return false;
    }

    // Register taskP

    return true;
}
#endif