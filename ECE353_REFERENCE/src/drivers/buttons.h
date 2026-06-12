/**
 * @file buttons.h
 * @author  Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __BUTTONS_H__
#define __BUTTONS_H__

#include <stdio.h>
#include "cybsp.h" //cybsp infenion microprocessor. How to start up the clocks, interrupt controller. Start-up routines: Manufacture.
#include "cyhal_gpio.h"
#include "cyhal_timer.h" //clangd will give a lot of warnings, we want a timer gpio, we use these header files, do includes in the C file
#include "ece353-pins.h" // so we import in advance.
#include "ece353-events.h"
#include "timer.h"

typedef enum {//so hardware wise there are only 3 buttons. Limited to hardware
    BUTTON_SW1 = 0,//ece353_button_t //enum type says button tells me which button its assigned to
    BUTTON_SW2,//increment automatically switch 2 is 1 and so on. Encode unique values to different states.
    BUTTON_SW3, //transistors are used to check for signals going from high to low, and then the code is checked and compared with signals.
} ece353_button_t; //use to determine current state, used to parametrize ece353_button_t

typedef enum {
    BUTTON_STATE_FALLING_EDGE = 0,// hardware detects low high, rising, falling edge so 0, 1 ,2, 3
    BUTTON_STATE_LOW,
    BUTTON_STATE_HIGH,
    BUTTON_STATE_RISING_EDGE,
} button_state_t;

typedef enum {
    INIT=0,
    SW1_DET,
    SW2_DET_1,
    SW2_DET_2,
    SW3_DET
} state03_t;

// Function to initialize the buttons
cy_rslt_t buttons_init_gpio(void);

// Function to initialize the timer for button debouncing
cy_rslt_t buttons_init_timer(void);

// Function to read the state of a specific button
button_state_t buttons_get_state(ece353_button_t button);

#endif