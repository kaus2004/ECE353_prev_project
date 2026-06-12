/**
 * @file buttons.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
//compiler tries to find
 #include "buttons.h"    //The PSoC 6 Hardware Abstraction Layer (HAL) provides a high-level, portable interface for interacting 
#include "cyhal_timer.h" //with the hardware peripherals on PSoC 6 MCUs. It simplifies the process of configuring and using various
#include "ece353-events.h" // hardware blocks by offering a set of generic API functions. //Psoc 6 is the board name
#include "ece353-pins.h"

// depending on the previous state that the button was low or high, compare it with the current and send out a message indicating.

// Track previous levels for edge detection
static uint8_t prev_button_level[3] = {1, 1, 1}; // assume idle (HIGH)

button_state_t buttons_get_state(ece353_button_t button){
    button_state_t pin_state;//Declare a function that is used to return the current state of the button.  Which button is being read will be passed into the function using the enum you defined.
    uint8_t current_level = 1;
    button_state_t state = BUTTON_STATE_HIGH;

    // Read pin level depending on button
    switch (button)
    {
        case BUTTON_SW1:
            current_level = cyhal_gpio_read(PIN_BUTTON_SW1);
            break;
        case BUTTON_SW2:
            current_level = cyhal_gpio_read(PIN_BUTTON_SW2);
            break;
        case BUTTON_SW3:
            current_level = cyhal_gpio_read(PIN_BUTTON_SW3);
            break;
    }

    // Compare with previous reading
    if (current_level == 0 && prev_button_level[button] == 1) {
        state = BUTTON_STATE_FALLING_EDGE ;  // just pressed
    }
    else if (current_level == 1 && prev_button_level[button] == 0) {
        state = BUTTON_STATE_RISING_EDGE;   // just released
    }
    else if (current_level == 0) {
        state = BUTTON_STATE_LOW;       // held down
    }
    else {
        state = BUTTON_STATE_HIGH;      // idle
    }

    // Save for next call
    prev_button_level[button] = current_level;

    return state;
}
cy_rslt_t buttons_init_gpio(void){

    cy_rslt_t rslt;
    // Initialize the GPIO pins for the buttons as inputs with pull-up resistors
    rslt = cyhal_gpio_init(PIN_BUTTON_SW1,
                    CYHAL_GPIO_DIR_INPUT,
                    CYHAL_GPIO_DRIVE_PULLUP,
                    1);
    if(rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to initialize SW1 button GPIO\n");
        return rslt; // Return if initialization fails
    }
    rslt = cyhal_gpio_init(PIN_BUTTON_SW2, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, 1);
    if(rslt != CY_RSLT_SUCCESS)
    { 
        printf("Failed to initialize SW2 button GPIO\n");
        return rslt; // Return if initialization fails
    }
    rslt = cyhal_gpio_init(PIN_BUTTON_SW3, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, 1);
    if(rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to initialize SW3 button GPIO\n");
        return rslt; // Return if initialization fails
    }
}

static cyhal_timer_t button_timer; //global variable
static cyhal_timer_cfg_t button_timer_cfg; //global variable

static void button_timer_handler(void *arg, cyhal_timer_event_t event) {
    // 5 concecutive time the interrupt occurs results in the button being pressed
    static int8_t button_counts[3] = {0, 0, 0}; // Array to hold counts for each button //acts as a global variable
                                                //we enter interrupt service routine, and we want count to persist
                                                //through the different calls to the interrupt service routine
                                                //line 51 is not called again and again, setting happens only once.
    uint8_t sw1 = PORT_BUTTON_SW1->IN & MASK_BUTTON_PIN_SW1; //these buttons are active low
    uint8_t sw2 = PORT_BUTTON_SW2->IN & MASK_BUTTON_PIN_SW2;
    uint8_t sw3 = PORT_BUTTON_SW3->IN & MASK_BUTTON_PIN_SW3;

    if(sw1 == 0) { // Button SW1 is pressed (active low)
        button_counts[0]++; // Increment count for SW1
        if(button_counts[0] == 5) {
            {
                ECE353_Events.sw1=1;
            }
        }
    } else { // Button SW1 is not pressed
        button_counts[0] = 0; // Reset count
    }

    if(sw2 == 0) { // Button SW2 is pressed (active low)
        button_counts[1]++; // Increment count for SW2
        if(button_counts[1] == 5) {
            {
                ECE353_Events.sw2=1;
            }
        }
    } else { // Button SW2 is not pressed
        button_counts[1] = 0; // Reset count
    }

    if(sw3 == 0) { // Button SW3 is pressed (active low)
        button_counts[2]++; // Increment count for SW3
        if(button_counts[2] == 5) {
            {
                ECE353_Events.sw3=1;
            }
        }
    } else { // Button SW3 is not pressed
        button_counts[2] = 0; // Reset count
    }
}

// Function to initialize the timer for button debouncing
cy_rslt_t buttons_init_timer(void){
    //Initialize the timer for button debouncing for a period of 5ms
    return timer_init(&button_timer, &button_timer_cfg, 500000, button_timer_handler); //10ms
}
