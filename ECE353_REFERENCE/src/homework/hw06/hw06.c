/**
 * @file ex03.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "FreeRTOSConfig.h"
#include "main.h"
#include "rtos_events.h"

#if defined(HW06)
#include "drivers.h"
#include "task_buttons.h"
#include "task_console.h"
#include "task_ipc.h"
#include "drivers.h"        
#include "task_lcd.h"
#include "task_joystick.h"
#include "battleship.h"

char APP_DESCRIPTION[] = "ECE353: ICE 11 - FreeRTOS IPC Rx/Tx";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
cyhal_i2c_t *I2C_Obj = NULL;
cyhal_spi_t *SPI_Obj = NULL;

SemaphoreHandle_t Semaphore_I2C = NULL;
SemaphoreHandle_t Semaphore_SPI = NULL;

QueueHandle_t Queue_System_Control_Responses = NULL;
QueueHandle_t Queue_Light_Monitor_Responses = NULL;
QueueHandle_t Queue_IMU_Monitor_Responses = NULL;

uint8_t player_id = 5; //starting with invalid ID

// Ship placement globals
volatile int8_t cursor_row = 0;
volatile int8_t cursor_col = 0;
volatile bool ship_orientation_horizontal = true;
volatile battleship_type_t current_ship_type = BATTLESHIP_TYPE_CARRIER;
volatile bool ship_placement_active = false;
uint8_t ship_board[10][10] = {0}; // Global board for ship placement tracking

// Ship tracking structure for sinking detection
typedef struct {
    uint8_t start_row;
    uint8_t start_col;
    uint8_t length;
    bool horizontal;
    uint8_t hits;  // Number of hits on this ship
    bool sunk;     // True if ship is sunk
} ship_info_t;

// Array to track all 5 ships
ship_info_t our_ships[5];
uint8_t num_ships_placed = 0;

// Gameplay globals
//solely used to track hits and misses not LCD use
int8_t hit_board[10][10]; // -2 = empty, 0 = occupied, 1 = hit, -1 = miss
int8_t opp_hits_miss_track[10][10]; // derived from hit board used for LCD display

int8_t our_hits_miss_track[10][10]; // used to track our hits and miss. Used to 
                                    // display on LCD during our turn for ATTACK phase
                                    // depends on IPC Rx from opponent for HIT or MISS
/////////////////////////
// Initialize IO Expander to show all 5 ships active
uint8_t active_ships;
uint8_t opponent_active_ships;  // Track opponent's remaining ships
uint16_t ship_border_color;

int iteration =0; 

int num_turns; //used to keep track of turns

int num_hits=0;
int num_misses=0;
//Task handles
bool my_turn;

TaskHandle_t imu_monitor = NULL;
TaskHandle_t gatekeeper = NULL;
TaskHandle_t system_control = NULL;
/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/
void task_light_monitor(void *arg);
void task_imu_monitor(void *arg);
void task_ship_orientation_monitor(void *arg);
void draw_game_stats(void);
int get_ship_length(battleship_type_t type);
bool is_ship_placement_valid(uint8_t col, uint8_t row, battleship_type_t type, bool horizontal, uint8_t board[10][10]);
void place_ship_on_board(uint8_t col, uint8_t row, battleship_type_t type, bool horizontal, uint8_t board[10][10]);
void save_ship_info(uint8_t col, uint8_t row, battleship_type_t type, bool horizontal);
int check_ship_sunk(uint8_t fire_row, uint8_t fire_col);
void copy_ships_to_hit_board(uint8_t src_board[10][10], int8_t dest_board[10][10]);
void update_active_ships_display(uint8_t active_ships);
void gameplay_init(void);  // Changed from task to regular function
void attack_phase(void);    // Our turn to attack
void observe_phase(void);   // Opponent's turn - we wait for their fire
void draw_attack_board(void);  // New function to draw attack phase board
void draw_defense_board(void); // New function to draw our defense board

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/
/**
 * @brief
 * Get the length of a ship based on its type
 */
int get_ship_length(battleship_type_t type)
{
    switch(type){
        case BATTLESHIP_TYPE_CARRIER:    return 5;
        case BATTLESHIP_TYPE_BATTLESHIP: return 4;
        case BATTLESHIP_TYPE_CRUISER:    return 3;
        case BATTLESHIP_TYPE_SUBMARINE:  return 3;
        case BATTLESHIP_TYPE_DESTROYER:  return 2;
        default: return 0;
    }
}

/**
 * @brief
 * Check if ship placement is valid (within bounds and no overlap)
 */
bool is_ship_placement_valid(uint8_t col, uint8_t row, battleship_type_t type, bool horizontal, uint8_t board[10][10])
{
    int ship_length = get_ship_length(type);
    
    // Check bounds
    if (horizontal) {
        if (col + ship_length > 10) {
            return false;
        }
    } else {
        if (row + ship_length > 10) {
            return false;
        }
    }
    
    // Check for overlap
    for (int i = 0; i < ship_length; i++) {
        uint8_t check_row = horizontal ? row : row + i;
        uint8_t check_col = horizontal ? col + i : col;
        
        if (board[check_row][check_col] != 0) {
            return false; // Overlap detected
        }
    }
    
    return true;
}

/**
 * @brief
 * Mark ship positions on the board array
 */
void place_ship_on_board(uint8_t col, uint8_t row, battleship_type_t type, bool horizontal, uint8_t board[10][10])
{
    int ship_length = get_ship_length(type);
    
    for (int i = 0; i < ship_length; i++) {
        uint8_t place_row = horizontal ? row : row + i;
        uint8_t place_col = horizontal ? col + i : col;
        board[place_row][place_col] = 1; // Mark as occupied
    }
}

/**
 * @brief
 * Save ship placement info for sinking detection
 */
void save_ship_info(uint8_t col, uint8_t row, battleship_type_t type, bool horizontal)
{
    task_console_printf("save_ship_info called: col=%d, row=%d, type=%d, horiz=%d, num_ships_placed=%d\r\n",
                       col, row, type, horizontal, num_ships_placed);
    
    if (num_ships_placed < 5)
    {
        our_ships[num_ships_placed].start_row = row;
        our_ships[num_ships_placed].start_col = col;
        our_ships[num_ships_placed].length = get_ship_length(type);
        our_ships[num_ships_placed].horizontal = horizontal;
        our_ships[num_ships_placed].hits = 0;
        our_ships[num_ships_placed].sunk = false;
        task_console_printf("Saved ship %d: row=%d, col=%d, len=%d, horiz=%d\r\n",
                           num_ships_placed, row, col, our_ships[num_ships_placed].length, horizontal);
        num_ships_placed++;
    }
    else
    {
        task_console_printf("ERROR: Tried to save more than 5 ships!\r\n");
    }
}

/**
 * @brief
 * Check if a hit at (fire_row, fire_col) sinks a ship
 * @return Index of sunk ship (0-4), or -1 if no ship sunk
 */
int check_ship_sunk(uint8_t fire_row, uint8_t fire_col)
{
    // Find which ship was hit
    for (int i = 0; i < 5; i++)
    {
        if (our_ships[i].sunk)
        {
            continue;  // Skip already sunk ships
        }
        
        // Check if this coordinate belongs to ship i
        bool belongs_to_ship = false;
        for (int j = 0; j < our_ships[i].length; j++)
        {
            uint8_t ship_row = our_ships[i].horizontal ? our_ships[i].start_row : our_ships[i].start_row + j;
            uint8_t ship_col = our_ships[i].horizontal ? our_ships[i].start_col + j : our_ships[i].start_col;
            
            if (ship_row == fire_row && ship_col == fire_col)
            {
                belongs_to_ship = true;
                break;
            }
        }
        
        if (belongs_to_ship)
        {
            // Increment hit count for this ship
            our_ships[i].hits++;
            
            task_console_printf("Ship %d hit! Hits: %d/%d\r\n", i, our_ships[i].hits, our_ships[i].length);
            
            // Check if ship is sunk
            if (our_ships[i].hits >= our_ships[i].length)
            {
                our_ships[i].sunk = true;
                task_console_printf("Ship %d SUNK!\r\n", i);
                return i;  // Return index of sunk ship
            }
            
            return -1;  // Hit but not sunk
        }
    }
    
    return -1;  // Should not reach here if hit_board is correct
}

/**
 * @brief
 * Copy ship positions from ship_board to hit_board
 * Converts 1s (occupied) in ship_board to 0s (occupied/not hit) in hit_board
 * 
 * @param src_board Source board with ship placements (1 = occupied)
 * @param dest_board Destination hit tracking board (0 = occupied, -2 = empty)
 */
void copy_ships_to_hit_board(uint8_t src_board[10][10], int8_t dest_board[10][10])
{
    for (int row = 0; row < 10; row++)
    {
        for (int col = 0; col < 10; col++)
        {
            if (src_board[row][col] == 1)
            {
                // Ship cell - mark as occupied (not yet hit)
                dest_board[row][col] = 0;
            }
            else
            {
                // Empty cell
                dest_board[row][col] = -2;
            }
        }
    }
}

/**
 * @brief
 * Update IO Expander LEDs to show number of active ships
 * Uses lower 5 bits of IO Expander output port
 * 
 * @param active_ships Number of ships still alive (0-5)
 */
void update_active_ships_display(uint8_t active_ships)
{
    // Clamp to 0-5 range
    if (active_ships > 5)
    {
        active_ships = 5;
    }
    
    // Create LED pattern: active_ships number of 1s from LSB
    // 5 ships = 0b00011111 = 0x1F
    // 4 ships = 0b00001111 = 0x0F
    // 3 ships = 0b00000111 = 0x07
    // etc.
    uint8_t led_pattern = (1 << active_ships) - 1;
    
    // Write to IO Expander output port
    system_sensors_io_expander_write(Queue_System_Control_Responses, 
                                      TCA9534_OUTPUT_PORT_REG, 
                                      led_pattern);
    
    task_console_printf("Active ships: %d, LED pattern: 0x%02X\r\n", active_ships, led_pattern);
}

/**
 * @brief
 * Task that monitors SW1 presses to toggle ship orientation during placement
 * 
 * @param arg Unused parameter
 */
void task_ship_orientation_monitor(void *arg)
{
    (void)arg;
    lcd_msg_t lcd_msg;
    
    while(1)
    {
        // Only process orientation changes during active ship placement
        if (ship_placement_active)
        {
            // Wait for SW1 press
            EventBits_t events = xEventGroupWaitBits(
                ECE353_RTOS_Events,
                ECE353_RTOS_EVENTS_SW1,
                pdTRUE,  // Clear bit on exit
                pdFALSE,
                pdMS_TO_TICKS(100)  // Check every 100ms
            );
            
            if (events & ECE353_RTOS_EVENTS_SW1)
            {
                // Try toggling orientation
                bool new_orientation = !ship_orientation_horizontal;
                int ship_length = get_ship_length(current_ship_type);
                
                // Check if new orientation would violate bounds
                bool valid = true;
                if (new_orientation) {
                    // Switching to horizontal
                    if (cursor_col + ship_length > 10) {
                        valid = false;
                        //task_console_printf("Cannot toggle: horizontal ship exceeds board\r\n");
                    }
                } else {
                    // Switching to vertical
                    if (cursor_row + ship_length > 10) {
                        valid = false;
                        //task_console_printf("Cannot toggle: vertical ship exceeds board\r\n");
                    }
                }
                
                // Check if new orientation would cause overlap
                if (valid && !is_ship_placement_valid(cursor_col, cursor_row, current_ship_type, 
                                                      new_orientation, ship_board)) {
                    valid = false;
                    //task_console_printf("Cannot toggle: ship would overlap\r\n");
                }
                
                if (valid)
                {
                    // Erase ship with old orientation
                    lcd_msg.command = LCD_CMD_ERASE_SHIP;
                    lcd_msg.payload.battleship.col = cursor_col;
                    lcd_msg.payload.battleship.row = cursor_row;
                    lcd_msg.payload.battleship.border_color = ship_border_color;
                    lcd_msg.payload.battleship.fill_color = LCD_COLOR_GRAY;
                    lcd_msg.payload.battleship.type = current_ship_type;
                    lcd_msg.payload.battleship.horizontal = ship_orientation_horizontal;
                    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
                    
                    vTaskDelay(pdMS_TO_TICKS(100));
                    
                    // Toggle orientation
                    ship_orientation_horizontal = new_orientation;
                    //task_console_printf("Orientation: %s\r\n", 
                                       //ship_orientation_horizontal ? "HORIZONTAL" : "VERTICAL");
                    
                    // Draw ship with new orientation
                    lcd_msg.command = LCD_CMD_DRAW_SHIP;
                    lcd_msg.payload.battleship.col = cursor_col;
                    lcd_msg.payload.battleship.row = cursor_row;
                    lcd_msg.payload.battleship.border_color = ship_border_color;
                    lcd_msg.payload.battleship.fill_color = LCD_COLOR_GRAY;
                    lcd_msg.payload.battleship.type = current_ship_type;
                    lcd_msg.payload.battleship.horizontal = ship_orientation_horizontal;
                    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
                }
            }
        }
        else
        {
            // Not placing ships, just wait
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}

/**
 * @brief
 * Task that periodically polls the IMU sensor and moves cursor
 * 
 * @param arg Unused parameter
 */
void task_imu_monitor(void *arg)
{
  while(1){
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    (void)arg;
    int16_t accel[3] = {0};
    static int8_t prev_row = 0;
    static int8_t prev_col = 0;
    lcd_msg_t lcd_msg;
    
    // Array of all 5 ship types to place
    battleship_type_t ship_types[5] = {
        BATTLESHIP_TYPE_CARRIER,
        BATTLESHIP_TYPE_BATTLESHIP,
        BATTLESHIP_TYPE_CRUISER,
        BATTLESHIP_TYPE_SUBMARINE,
        BATTLESHIP_TYPE_DESTROYER
    };
    uint8_t current_ship_index = 0;
    
    // Activate ship placement monitoring
    ship_placement_active = true;
    current_ship_type = ship_types[current_ship_index];
    
    // Wait for game to start (same event as system control)
    // xEventGroupWaitBits(
    //     ECE353_RTOS_Events,
    //     ECE353_RTOS_EVENTS_SW1 | ECE353_RTOS_IPC_RX,
    //     pdTRUE,  // clear on exit as used for later stuff
    //     pdFALSE,
    //     portMAX_DELAY
    // );
    
    // Additional delay to ensure board is fully drawn

    vTaskDelay(pdMS_TO_TICKS(500));
    
    // // Draw initial cursor at (0,0)
    // lcd_msg.command = LCD_CMD_DRAW_CURSOR;
    // lcd_msg.payload.battleship.col = cursor_col;
    // lcd_msg.payload.battleship.row = cursor_row;
    // xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
    
    //now draw the initial ship at that position
    vTaskDelay(pdMS_TO_TICKS(500));
    lcd_msg.command = LCD_CMD_DRAW_SHIP;
    lcd_msg.payload.battleship.col = cursor_col;
    lcd_msg.payload.battleship.row = cursor_row;
    lcd_msg.payload.battleship.border_color = ship_border_color;
    lcd_msg.payload.battleship.fill_color = LCD_COLOR_GRAY;
    lcd_msg.payload.battleship.type = ship_types[current_ship_index];
    lcd_msg.payload.battleship.horizontal = ship_orientation_horizontal;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

    
    while(1)
    {
        // Poll IMU every second
        if(system_sensors_get_imu(Queue_IMU_Monitor_Responses, accel))
        {
            int16_t accel_x = accel[0];
            int16_t accel_y = accel[1];
            
            //task_console_printf("IMU Accel - X: %d, Y: %d, Z: %d\r\n", 
            //                   accel_x, accel_y, accel[2]);
            
            // Save previous position
            prev_row = cursor_row;
            prev_col = cursor_col;
            
            // Y-axis control (Vertical movement)
            if (accel_y > 5000)
            {
                // Move DOWN
                cursor_row++;
            }
            else if (accel_y < 2000)
            {
                // Move UP
                cursor_row--;
            }
            
            // X-axis control (Horizontal movement)
            if (accel_x > 2000)
            {
                // Move LEFT
                cursor_col--;
            }
            else if (accel_x < -2000)
            {
                // Move RIGHT
                cursor_col++;
            }
            
            // Clamp cursor position to 0-9 range
            if (cursor_row < 0) cursor_row = 0;
            if (cursor_row > 9) cursor_row = 9;
            if (cursor_col < 0) cursor_col = 0;
            if (cursor_col > 9) cursor_col = 9;

            // Clamp cursor based on ship length to prevent ship from exiting board
            int ship_length = get_ship_length(ship_types[current_ship_index]);
            if (ship_orientation_horizontal) {
                // Horizontal ship: limit column so ship doesn't exceed right edge
                if (cursor_col + ship_length > 10) {
                    cursor_col = 10 - ship_length;
                }
            } else {
                // Vertical ship: limit row so ship doesn't exceed bottom edge
                if (cursor_row + ship_length > 10) {
                    cursor_row = 10 - ship_length;
                }
            }
            
            // Check for overlap with existing ships and revert to previous position if invalid
            if (!is_ship_placement_valid(cursor_col, cursor_row, ship_types[current_ship_index], 
                                        ship_orientation_horizontal, ship_board)) {
                // Invalid placement - revert to previous position
                cursor_row = prev_row;
                cursor_col = prev_col;
                //task_console_printf("Ship overlap detected - position reverted\r\n");
            }
            
            
            // If cursor moved, update display
            if (cursor_row != prev_row || cursor_col != prev_col)
            {
               //task_console_printf("Cursor moved: (%d,%d) -> (%d,%d)\r\n", 
                                  // prev_col, prev_row, cursor_col, cursor_row);
                
                //remove the old ship also
                lcd_msg.command = LCD_CMD_ERASE_SHIP;
                lcd_msg.payload.battleship.col = prev_col;
                lcd_msg.payload.battleship.row = prev_row;
                lcd_msg.payload.battleship.border_color = ship_border_color;
                lcd_msg.payload.battleship.fill_color = LCD_COLOR_GRAY;
                lcd_msg.payload.battleship.type = ship_types[current_ship_index];
                lcd_msg.payload.battleship.horizontal = ship_orientation_horizontal;
                xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
            }
            
            // Draw the ship at current position (regardless of whether cursor moved)
            // This allows SW2 placement even when stationary
            lcd_msg.command = LCD_CMD_DRAW_SHIP;
            lcd_msg.payload.battleship.col = cursor_col;
            lcd_msg.payload.battleship.row = cursor_row;
            lcd_msg.payload.battleship.border_color = ship_border_color;
            lcd_msg.payload.battleship.fill_color = LCD_COLOR_GRAY;
            lcd_msg.payload.battleship.type = ship_types[current_ship_index];
            lcd_msg.payload.battleship.horizontal = ship_orientation_horizontal;
            xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
        }
        else
        {
            //task_console_printf("IMU Monitor: Failed to read IMU\r\n");
        }
        
        // Check if ship was placed (SW2 pressed in LCD task)
        EventBits_t ship_placed_event = xEventGroupWaitBits(
            ECE353_RTOS_Events,
            ECE353_RTOS_EVENTS_SHIP_PLACED,
            pdTRUE,  // Clear bit on exit
            pdFALSE,
            0 // Don't wait, just check
        );
        
        if (ship_placed_event & ECE353_RTOS_EVENTS_SHIP_PLACED)
        {
            //task_console_printf("Ship %d placed!\r\n", current_ship_index);
            
            // Mark ship position on board array
            place_ship_on_board(cursor_col, cursor_row, ship_types[current_ship_index], 
                               ship_orientation_horizontal, ship_board);
            
            // Save ship info for sinking detection
            save_ship_info(cursor_col, cursor_row, ship_types[current_ship_index],
                          ship_orientation_horizontal);
            
            // Move to next ship
            current_ship_index++;
            
            // Check if all ships placed
            if (current_ship_index >= 5)
            {
                task_console_printf("All 5 placed\\r\\n");
                ship_placement_active = false;  // Disable orientation monitoring
                
                break;
            }
            
            // Update current ship type for orientation monitor
            current_ship_type = ship_types[current_ship_index];
            
            // Reset cursor to (0,0) for next ship
            cursor_row = 0;
            cursor_col = 0;
            prev_row = 0;
            prev_col = 0;
            
            
            // // Draw cursor at reset position
            // lcd_msg.command = LCD_CMD_DRAW_CURSOR;
            // lcd_msg.payload.battleship.col = cursor_col;
            // lcd_msg.payload.battleship.row = cursor_row;
            // xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
            
            // Draw new ship type at reset position
            lcd_msg.command = LCD_CMD_DRAW_SHIP;
            lcd_msg.payload.battleship.col = cursor_col;
            lcd_msg.payload.battleship.row = cursor_row;
            lcd_msg.payload.battleship.border_color = ship_border_color;
            lcd_msg.payload.battleship.fill_color = LCD_COLOR_GRAY;
            lcd_msg.payload.battleship.type = ship_types[current_ship_index];
            lcd_msg.payload.battleship.horizontal = ship_orientation_horizontal;
            xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
        }
        
        // Poll every 1 second (rate limiting)
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    // Synchronization: Both players must be ready before starting game
    bool received_opponent_ready = false;
    bool received_ack = false;
    
    // Send PLAYER READY via IPC
    ipc_send_game_control(IPC_GAME_CONTROL_PLAYER_READY);
    task_console_printf("Sent PLAYER_READY, waiting for opponent...\r\n");
    
    while(!(received_opponent_ready && received_ack))
    {
        // First check event bits (non-blocking quick check)
        EventBits_t events = xEventGroupWaitBits(
            ECE353_RTOS_Events,
            ECE353_RTOS_ALL_PLAYERS_READY_ACK | ECE353_RTOS_ALL_PLAYERS_READY_IPC,
            pdTRUE,   // Clear bits on exit
            pdFALSE,  // Wait for any bit
            pdMS_TO_TICKS(500)  // Timeout to allow re-checking
        );
        
        if (events & ECE353_RTOS_ALL_PLAYERS_READY_IPC) {
            // Received PLAYER_READY from opponent
            if (!received_opponent_ready) {
                received_opponent_ready = true;
                task_console_printf("Received opponent's PLAYER_READY, sending ACK\r\n");
                //xQueueReceive(Queue_IPC_Rx, &drain_packet, 0) == pdPASS);
                ipc_send_game_control(IPC_GAME_CONTROL_ACK);
            }
        }
        
        if (events & ECE353_RTOS_ALL_PLAYERS_READY_ACK) {
            // Received ACK from opponent (they got our PLAYER_READY)
            if (!received_ack) {
                received_ack = true;
                task_console_printf("Received ACK from opponent\r\n");
            }
        }
        
        // Debug: Print current state
        if (events != 0) {
            task_console_printf("Handshake state: opponent_ready=%d, ack=%d\r\n", 
                               received_opponent_ready, received_ack);
        }
    }
    
    task_console_printf("Both players ready - starting game!\r\n");
    
    // Drain any leftover packets from the IPC queue (handshake packets like PLAYER_READY, ACK)
    ipc_packet_t drain_packet;
    while(xQueueReceive(Queue_IPC_Rx, &drain_packet, 0) == pdPASS)
    {
        task_console_printf("Drained leftover packet from queue: cmd=%d\r\n", drain_packet.cmd);
    }
    
    xEventGroupSetBits(ECE353_RTOS_Events, ECE353_RTOS_START_GAME);
    
  }
    // //enter new loop
    // while(1){
    //     vTaskDelay(portMAX_DELAY);
    // }
}

/**
 * @brief
 * Helper function to draw game statistics on the LCD
 */
void draw_game_stats(void)
{
    lcd_msg_t lcd_msg = {0};
    char msg_buffer[20];  // Buffer for formatted string
    
    // Small delay to ensure board/background is fully rendered
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // -------- Message 1: Hits --------
    snprintf(msg_buffer, sizeof(msg_buffer), "Hits:%d", num_hits);
    size_t len1 = strlen(msg_buffer);

    lcd_msg.payload.console.message = pvPortMalloc(len1 + 1);
    if (lcd_msg.payload.console.message == NULL)
    {
        printf("Failed to allocate memory for Hits message\n");
        return;
    }

    strcpy(lcd_msg.payload.console.message, msg_buffer);
    lcd_msg.payload.console.length = len1;
    lcd_msg.payload.console.x_offset = 230;
    lcd_msg.payload.console.y_offset = 40;
    lcd_msg.command = LCD_CONSOLE_DRAW_MESSAGE;

    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

    // Wait for LCD to process the message
    vTaskDelay(pdMS_TO_TICKS(100));

    // -------- Message 2: Misses --------
    snprintf(msg_buffer, sizeof(msg_buffer), "Miss:%d", num_misses);
    size_t len2 = strlen(msg_buffer);

    lcd_msg.payload.console.message = pvPortMalloc(len2 + 1);
    if (lcd_msg.payload.console.message == NULL)
    {
        printf("Failed to allocate memory for Miss message\n");
        return;
    }

    strcpy(lcd_msg.payload.console.message, msg_buffer);
    lcd_msg.payload.console.length = len2;
    lcd_msg.payload.console.x_offset = 230;
    lcd_msg.payload.console.y_offset = 80;
    lcd_msg.command = LCD_CONSOLE_DRAW_MESSAGE;

    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
    // Wait for LCD to process the message
    vTaskDelay(pdMS_TO_TICKS(100));

    //-----------Message 3: Turn Indicator --------
    const char *turn_msg = "Turn:";
    size_t len3 = strlen(turn_msg);
    lcd_msg.payload.console.message = pvPortMalloc(len3 + 1);
    if (lcd_msg.payload.console.message == NULL)
    {
        printf("Failed to allocate memory for Turn Indicator message\n");
        return;
    }
    strcpy(lcd_msg.payload.console.message, turn_msg);
    lcd_msg.payload.console.length = len3;
    lcd_msg.payload.console.x_offset = 230;
    lcd_msg.payload.console.y_offset = 120;
    lcd_msg.command = LCD_CONSOLE_DRAW_MESSAGE;

    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

    // Wait for LCD to process the message
    vTaskDelay(pdMS_TO_TICKS(100));

    //continued as space limited on LCD
    //will change according to turn later
    const char *player_turn = (my_turn) ? "Yours":"ENEMY";
    size_t len4 = strlen(player_turn);
    lcd_msg.payload.console.message = pvPortMalloc(len4 + 1);
    if (lcd_msg.payload.console.message == NULL)
    {
        printf("Failed to allocate memory for Player Turn message\n");
        return;
    }
    strcpy(lcd_msg.payload.console.message, player_turn);
    lcd_msg.payload.console.length = len4;
    lcd_msg.payload.console.x_offset = 230;
    lcd_msg.payload.console.y_offset = 160;
    lcd_msg.command = LCD_CONSOLE_DRAW_MESSAGE;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

    // Wait for LCD to process the message
    vTaskDelay(pdMS_TO_TICKS(100));


}

/**
 * @brief
 * Draws the attack phase board with any previous hits/misses
 */
void draw_attack_board(void)
{
    lcd_msg_t lcd_msg;
    
    // Clear the screen first
    lcd_msg.command = LCD_CMD_CLEAR_SCREEN;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Draw fresh empty board
    lcd_msg.command = LCD_CMD_DRAW_BOARD;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // Draw any previous hits/misses from our_hits_miss_track
    for(int row = 0; row < 10; row++)
    {
        for(int col = 0; col < 10; col++)
        {
            if(our_hits_miss_track[row][col] == 1)  // Hit
            {
                // Draw hit marker (red)
                lcd_msg.command = LCD_CMD_DRAW_HIT;
                lcd_msg.payload.battleship.col = col;
                lcd_msg.payload.battleship.row = row;
                xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
                vTaskDelay(pdMS_TO_TICKS(20));
            }
            else if(our_hits_miss_track[row][col] == -1)  // Miss
            {
                // Draw miss marker (white)
                lcd_msg.command = LCD_CMD_DRAW_MISS;
                lcd_msg.payload.battleship.col = col;
                lcd_msg.payload.battleship.row = row;
                xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
                vTaskDelay(pdMS_TO_TICKS(20));
            }
            // -2 means not attacked yet, don't draw anything
        }
    }
    
    // Draw game stats
    draw_game_stats();
}

/**
 * @brief
 * Draws our defense board showing our ships and where opponent has attacked
 */
void draw_defense_board(void)
{
    lcd_msg_t lcd_msg;
    
    // Clear the screen first
    lcd_msg.command = LCD_CMD_CLEAR_SCREEN;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Draw the game board using existing function
    battleship_draw_game_board(player_id);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Redraw all our ships on the board
    battleship_redraw_all_ships(player_id);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Draw opponent's hits/misses on our board from opp_hits_miss_track
    for(int row = 0; row < 10; row++)
    {
        for(int col = 0; col < 10; col++)
        {
            if(opp_hits_miss_track[row][col] == 1)  // Opponent hit our ship
            {
                lcd_msg.command = LCD_CMD_DRAW_HIT;
                lcd_msg.payload.battleship.col = col;
                lcd_msg.payload.battleship.row = row;
                xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
                vTaskDelay(pdMS_TO_TICKS(20));
            }
            else if(opp_hits_miss_track[row][col] == -1)  // Opponent missed
            {
                lcd_msg.command = LCD_CMD_DRAW_MISS;
                lcd_msg.payload.battleship.col = col;
                lcd_msg.payload.battleship.row = row;
                xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
                vTaskDelay(pdMS_TO_TICKS(20));
            }
        }
    }
    
    // Draw game stats
    draw_game_stats();
}

/*
 * @brief
 * Initializes gameplay state - called from task_system_control, not a separate task
 */
void gameplay_init(void)
{
    active_ships = 5;
    opponent_active_ships = 5;  // Initialize opponent's ship count
    
    // Reset ship hit counters (positions already saved during placement)
    for(int i = 0; i < 5; i++)
    {
        our_ships[i].hits = 0;
        our_ships[i].sunk = false;
    }
    
    // Debug: Print saved ship info
    task_console_printf("=== Ship Info at Gameplay Start ===\r\n");
    for(int i = 0; i < num_ships_placed; i++)
    {
        task_console_printf("Ship %d: row=%d, col=%d, len=%d, horiz=%d\r\n",
                           i, our_ships[i].start_row, our_ships[i].start_col, 
                           our_ships[i].length, our_ships[i].horizontal);
    }
    task_console_printf("Total ships placed: %d\r\n", num_ships_placed);
    
    // Copy ship positions from ship_board to hit_board
    copy_ships_to_hit_board(ship_board, hit_board);
    task_console_printf("Ships copied to hit_board for gameplay\r\n");
    
    // Configure IO Expander: set lower 5 pins as outputs (0 = output)
    system_sensors_io_expander_write(Queue_System_Control_Responses,
                                      TCA9534_CONFIG_REG,
                                      0xE0);
    
    // Display initial ship count
    update_active_ships_display(active_ships);
    
    // Initialize BOTH tracking boards to all empty (-2)
    for(int r = 0; r < 10; r++) {
        for(int c = 0; c < 10; c++) {
            our_hits_miss_track[r][c] = -2;      // Track our attacks on opponent
            opp_hits_miss_track[r][c] = -2;      // Track opponent's attacks on us
        }
    }
    
    // Reset cursor for attack phase
    cursor_row = 0;
    cursor_col = 0;
    
    // Determine who goes first: Player 0 attacks first
    my_turn = (player_id == 0 );
    
    task_console_printf("Gameplay started! Player %d goes first\r\n", my_turn ? player_id : 1 - player_id);
    
    // Main gameplay loop
    while(active_ships > 0 && opponent_active_ships > 0)
    {
        if(my_turn)
        {
            task_console_printf("Your turn - Attack!\r\n");
            
            // Draw fresh attack board before each attack phase
            draw_attack_board();
            
            attack_phase();
            my_turn = false;
        }
        else
        {
            task_console_printf("Enemy turn - Observing...\r\n");
            observe_phase();
            my_turn = true;
        }
        
        // Check if game is over (either we lost all ships or opponent did)
        // This would be signaled via IPC_GAME_CONTROL_END_GAME
    }
    
    // Clear the screen for end game message
    lcd_msg_t lcd_msg;
    lcd_msg.command = LCD_CMD_CLEAR_SCREEN;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Display end game message
    const char *end_msg;
    if(active_ships > 0)
    {
        // We won - opponent has no ships left
        end_msg = "!!YOU WON!!";
        task_console_printf("All opponent ships sunk! You win!\r\n");
    }
    else
    {
        // We lost - we have no ships left
        end_msg = "GAME OVER...YOU LOST...";
        task_console_printf("All your ships have been sunk! You lose!\r\n");
    }
    
    // Draw the end game message on LCD
    size_t len = strlen(end_msg);
    lcd_msg.payload.console.message = pvPortMalloc(len + 1);
    if (lcd_msg.payload.console.message != NULL)
    {
        strcpy(lcd_msg.payload.console.message, end_msg);
        lcd_msg.payload.console.length = len;
        lcd_msg.payload.console.x_offset = 50;
        lcd_msg.payload.console.y_offset = 110;
        lcd_msg.command = LCD_CONSOLE_DRAW_MESSAGE;
        xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
        //allow it to display for some time.
        //vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    // Handle IPC handshake for game end
    if(active_ships == 0)
    {
        // We lost - send END_GAME and wait for ACK
        ipc_send_game_control(IPC_GAME_CONTROL_END_GAME);
        xEventGroupWaitBits(ECE353_RTOS_Events, ECE353_RTOS_ALL_PLAYERS_READY_ACK, pdTRUE, pdTRUE, portMAX_DELAY);
    }
    else
    {
        // We won - wait for opponent's END_GAME then send ACK
        xEventGroupWaitBits(ECE353_RTOS_Events, ECE353_RTOS_GAME_WON_ACK, pdTRUE, pdTRUE, portMAX_DELAY);
        ipc_send_game_control(IPC_GAME_CONTROL_ACK);
    }

    xTaskNotifyGive(system_control);
    
    task_console_printf("Game Over!\r\n");
}

/**
 * @brief
 * Attack phase - use joystick to move cursor and SW1 to fire
 */
void attack_phase(void)
{
    lcd_msg_t lcd_msg;
    int8_t prev_row = cursor_row;
    int8_t prev_col = cursor_col;
    bool attack_complete = false;
    
    // Read joystick center values for calibration (optional)
    uint16_t joy_x, joy_y;
    
    // Joystick thresholds for 16-bit ADC (0-65535, center ~32768)
    const uint16_t JOY_HIGH_THRESHOLD = 45000;  // Above this = pushed in positive direction
    const uint16_t JOY_LOW_THRESHOLD = 20000;   // Below this = pushed in negative direction
    
    task_console_printf("=== ATTACK PHASE STARTED ===\r\n");
    task_console_printf("Drawing cursor at (%d, %d)\r\n", cursor_col, cursor_row);
    
    // Draw initial cursor at current position using battleship function directly
    battleship_draw_cursor(cursor_col, cursor_row, player_id);
    
    task_console_printf("Cursor drawn. Use joystick to move, SW1 to fire.\r\n");
    
    // Give LCD time to process
    vTaskDelay(pdMS_TO_TICKS(200));
    
    while(!attack_complete)
    {
        // === STEP 1: Poll joystick for cursor movement ===
        joy_x = joystick_read_x();
        joy_y = joystick_read_y();
        
        prev_row = cursor_row;
        prev_col = cursor_col;
        
        // Move cursor based on joystick position (INVERTED)
        // Y-axis: HIGH = UP, LOW = DOWN
        if(joy_y > JOY_HIGH_THRESHOLD && cursor_row > 0)  // Joystick pushed up
        {
            cursor_row--;
        }
        else if(joy_y < JOY_LOW_THRESHOLD && cursor_row < 9)  // Joystick pushed down
        {
            cursor_row++;
        }
        
        // X-axis: HIGH = LEFT, LOW = RIGHT
        if(joy_x > JOY_HIGH_THRESHOLD && cursor_col > 0)  // Joystick pushed left
        {
            cursor_col--;
        }
        else if(joy_x < JOY_LOW_THRESHOLD && cursor_col < 9)  // Joystick pushed right
        {
            cursor_col++;
        }
        
        // === STEP 2: Update display if cursor moved ===
        if(cursor_row != prev_row || cursor_col != prev_col)
        {
            task_console_printf("Cursor moved: (%d,%d) -> (%d,%d) Joy X=%d Y=%d\r\n", 
                               prev_col, prev_row, cursor_col, cursor_row, joy_x, joy_y);
            
            // Check if previous position had a hit/miss marker - restore it
            if(our_hits_miss_track[prev_row][prev_col] == 1)
            {
                // Restore hit marker
                lcd_msg.command = LCD_CMD_DRAW_HIT;
                lcd_msg.payload.battleship.col = prev_col;
                lcd_msg.payload.battleship.row = prev_row;
                xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
            }
            else if(our_hits_miss_track[prev_row][prev_col] == -1)
            {
                // Restore miss marker
                lcd_msg.command = LCD_CMD_DRAW_MISS;
                lcd_msg.payload.battleship.col = prev_col;
                lcd_msg.payload.battleship.row = prev_row;
                xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
            }
            else
            {
                // Clear old cursor (empty cell) - use battleship function
                battleship_clear_cursor(prev_col, prev_row, player_id);
            }
            
            // Draw new cursor using battleship function
            battleship_draw_cursor(cursor_col, cursor_row, player_id);
        }
        
        // === STEP 3: Check for SW1 press (fire) - NON-BLOCKING ===
        EventBits_t events = xEventGroupWaitBits(
            ECE353_RTOS_Events,
            ECE353_RTOS_EVENTS_SW1,
            pdTRUE,
            pdFALSE,
            0
        );
        
        if(events & ECE353_RTOS_EVENTS_SW1)
        {
            task_console_printf("SW1 pressed! Attempting to fire at (%d, %d)\r\n", cursor_col, cursor_row);
            
            // Check if this position was already attacked
            if(our_hits_miss_track[cursor_row][cursor_col] != -2)
            {
                task_console_printf("Already attacked this position!\r\n");
            }
            else
            {
                // Fire at this position!
                task_console_printf("FIRE at (%d, %d)!\r\n", cursor_col, cursor_row);
                
                // Send fire command via IPC
                ipc_send_fire(cursor_row, cursor_col);
                
                // Wait for RESULT from opponent - loop until we get IPC_CMD_RESULT
                ipc_packet_t rx_packet;
                task_console_printf("Waiting for result from opponent...\r\n");
                
                bool result_received = false;
                while(!result_received)
                {
                    if(xQueueReceive(Queue_IPC_Rx, &rx_packet, pdMS_TO_TICKS(10000)) == pdPASS)
                    {
                        task_console_printf("Received packet, cmd=%d\r\n", rx_packet.cmd);
                        
                        if(rx_packet.cmd == IPC_CMD_RESULT)
                        {
                            result_received = true;  // Exit the while loop
                            
                            if(rx_packet.result == IPC_RESULT_HIT)
                            {
                                our_hits_miss_track[cursor_row][cursor_col] = 1;  // Hit
                                num_hits++;
                                task_console_printf("HIT!\r\n");
                                
                                // Draw hit marker on LCD (red)
                                lcd_msg.command = LCD_CMD_DRAW_HIT;
                                lcd_msg.payload.battleship.col = cursor_col;
                                lcd_msg.payload.battleship.row = cursor_row;
                                xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
                            }
                            else if(rx_packet.result == IPC_RESULT_SUNK)
                            {
                                our_hits_miss_track[cursor_row][cursor_col] = 1;  // Hit (sunk is also a hit)
                                num_hits++;
                                task_console_printf("HIT and SUNK an enemy ship!\r\n");
                                
                                // Draw hit marker on LCD (red)
                                lcd_msg.command = LCD_CMD_DRAW_HIT;
                                lcd_msg.payload.battleship.col = cursor_col;
                                lcd_msg.payload.battleship.row = cursor_row;
                                xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
                                
                                // Decrement opponent's ship count and update IO expander
                                if(opponent_active_ships > 0)
                                {
                                    opponent_active_ships--;
                                    // Update IO expander to show opponent's remaining ships
                                    update_active_ships_display(opponent_active_ships);
                                    task_console_printf("Opponent has %d ships remaining\r\n", opponent_active_ships);
                                    
                                    // Check if we won
                                    if(opponent_active_ships == 0)
                                    {
                                        task_console_printf("All enemy ships destroyed! YOU WIN!\r\n");
                                    }
                                }
                            }
                            else if(rx_packet.result == IPC_RESULT_MISS)
                            {
                                our_hits_miss_track[cursor_row][cursor_col] = -1;  // Miss
                                num_misses++;
                                task_console_printf("MISS!\r\n");
                                
                                // Draw miss marker on LCD (white)
                                lcd_msg.command = LCD_CMD_DRAW_MISS;
                                lcd_msg.payload.battleship.col = cursor_col;
                                lcd_msg.payload.battleship.row = cursor_row;
                                xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
                            }
                            else
                            {
                                task_console_printf("Received unknown result from opponent: %d\r\n", rx_packet.result);
                            }
                            
                            // Update stats display
                            vTaskDelay(pdMS_TO_TICKS(100));
                            draw_game_stats();
                        }
                        else
                        {
                            // Not the RESULT we're waiting for, log and continue waiting
                            task_console_printf("Ignoring non-RESULT packet (cmd=%d) while waiting for result\r\n", rx_packet.cmd);
                        }
                    }
                    else
                    {
                        task_console_printf("Timeout waiting for result from opponent, retrying...\r\n");
                    }
                }
    
                attack_complete = true;  // End our turn
            }
        }
        
        // === STEP 4: Rate limiting - prevents busy-waiting ===
        vTaskDelay(pdMS_TO_TICKS(150));  // Slightly longer delay for better responsiveness
    }
    
    task_console_printf("=== ATTACK PHASE ENDED ===\r\n");
}

/**
 * @brief
 * Observe phase - wait for opponent's fire command and respond
 */
void observe_phase(void)
{
    lcd_msg_t lcd_msg;
    ipc_packet_t rx_packet;
    
    // Draw our defense board showing our ships and previous opponent attacks
    draw_defense_board();
    
    // Wait for opponent's FIRE command - loop until we get IPC_CMD_FIRE
    bool fire_received = false;
    while(!fire_received)
    {
        if(xQueueReceive(Queue_IPC_Rx, &rx_packet, portMAX_DELAY) == pdPASS)
        {
            task_console_printf("Received packet, cmd=%d\r\n", rx_packet.cmd);
            
            if(rx_packet.cmd == IPC_CMD_FIRE)
            {
                fire_received = true;  // Exit the while loop
                
                uint8_t fire_row = rx_packet.fire.row;
                uint8_t fire_col = rx_packet.fire.col;
                
                task_console_printf("Enemy fired at (%d, %d)\r\n", fire_col, fire_row);
                
                // Check our hit_board to see if they hit
                if(hit_board[fire_row][fire_col] == 0)  // Ship present, not yet hit
                {
                    hit_board[fire_row][fire_col] = 1;  // Mark as hit
                    opp_hits_miss_track[fire_row][fire_col] = 1;  // Track for LCD display
                    
                    // Check if this hit sinks a ship
                    int sunk_ship_index = check_ship_sunk(fire_row, fire_col);
                    
                    if(sunk_ship_index >= 0)
                    {
                        // Ship was sunk!
                        ipc_send_result(IPC_RESULT_SUNK);
                        task_console_printf("They SUNK our ship #%d!\r\n", sunk_ship_index);
                        
                        // Decrement active ships (DO NOT update IO expander - it shows opponent's ships)
                        active_ships--;
                        
                        // Check if game over (all ships sunk)
                        if(active_ships == 0)
                        {
                            task_console_printf("All our ships are sunk! Game Over!\r\n");
                        }
                    }
                    else
                    {
                        // Hit but not sunk
                        ipc_send_result(IPC_RESULT_HIT);
                        task_console_printf("They HIT our ship!\r\n");
                    }
                    
                    // Draw hit marker on our board (red)
                    lcd_msg.command = LCD_CMD_DRAW_HIT;
                    lcd_msg.payload.battleship.col = fire_col;
                    lcd_msg.payload.battleship.row = fire_row;
                    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
                }
                else  // Empty water or already hit
                {
                    if(hit_board[fire_row][fire_col] == -2)  // Empty
                    {
                        hit_board[fire_row][fire_col] = -1;  // Mark as miss
                        opp_hits_miss_track[fire_row][fire_col] = -1;  // Track for LCD display
                    }
                    ipc_send_result(IPC_RESULT_MISS);
                    task_console_printf("They MISSED!\r\n");
                    
                    // Draw miss marker on our board (white)
                    lcd_msg.command = LCD_CMD_DRAW_MISS;
                    lcd_msg.payload.battleship.col = fire_col;
                    lcd_msg.payload.battleship.row = fire_row;
                    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
                }
            }
            else
            {
                // Not the FIRE we're waiting for, log and continue waiting
                task_console_printf("Ignoring non-FIRE packet (cmd=%d) while waiting for fire\r\n", rx_packet.cmd);
            }
        }
    }
}

/**
 * @brief
 * Task that periodically polls the light sensor and triggers theme changes
 * based on ambient light level.
 * 
 * @param arg Unused parameter
 */
void task_light_monitor(void *arg)
{
    (void)arg;
    uint16_t ambient_light = 0;
    static bool theme_is_white = false;
    const uint16_t LIGHT_THRESHOLD = 1000;
    
    // Wait a bit for system to initialize
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    while(1)
    {
        // Poll light sensor every second
        if(system_sensors_get_light(Queue_Light_Monitor_Responses, &ambient_light))
        {
            task_console_printf("Light Monitor: Read light = %d\r\n", ambient_light);
            // Check light threshold and change theme directly
            if (ambient_light > LIGHT_THRESHOLD && !theme_is_white)
            {
                // Light level high - switch to white theme
                battleship_change_theme(
                    LCD_COLOR_WHITE,              // Background
                    LCD_COLOR_BLACK,              // Doesn't matter player ID matters
                    LCD_COLOR_WHITE,              // Board fill
                    player_id);
                theme_is_white = true;
                //task_console_printf("Light Monitor: Light > %d, switched to WHITE theme\r\n", LIGHT_THRESHOLD);
                
                // Redraw game stats after theme change
                vTaskDelay(pdMS_TO_TICKS(100));
                draw_game_stats();
            }
            else if (ambient_light <= LIGHT_THRESHOLD && theme_is_white)
            {
                // Light level low - switch to black theme
                battleship_change_theme(
                    LCD_COLOR_BLACK,              // Background
                    BATTLESHIP_PLAYER_0_COLOR,    // Board border
                    LCD_COLOR_BLACK,              // Board fill
                    player_id);
                theme_is_white = false;
                task_console_printf("Light Monitor: Light <= %d, switched to BLACK theme\r\n", LIGHT_THRESHOLD);
                
                // Redraw game stats after theme change
                vTaskDelay(pdMS_TO_TICKS(100));
                draw_game_stats();
            }
        }
        else
        {
            task_console_printf("Light Monitor: Failed to read light sensor\r\n");
        }
        
        // Check every 5 seconds
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
/**
 * @brief
 * Experimental task to test if space is available
 * 
 * @param arg Unused parameter
 */
void game_monitor_task(void *arg){
    (void)arg;
    while(1){
        size_t free_heap = xPortGetFreeHeapSize();
        task_console_printf("Free Heap Size: %d bytes\r\n", free_heap);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/**
 * @brief
 * This task will be used to verify the functionality of the IPC UART specification
 * by .... we will do everything here then separate into different functions
 *
 * @param arg 
 * Unused parameter
 */
void task_system_control(void *arg)
{
    //initializing the board 
    EventBits_t events;
    lcd_msg_t lcd_msg;
    // Clear the screen
    lcd_msg.command = LCD_CMD_CLEAR_SCREEN;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

    //waiting for sw1 
    const char *msg1 = "Press SW1 to Start";
    size_t len1 = strlen(msg1);

    lcd_msg.payload.console.message = pvPortMalloc(len1 + 1);
    if (lcd_msg.payload.console.message == NULL)
    {
        printf("Failed to allocate memory for LCD message 1.\n");
        vTaskDelete(NULL);
    }

    strcpy(lcd_msg.payload.console.message, msg1);
    lcd_msg.payload.console.length = len1;
    lcd_msg.payload.console.x_offset = 50;
    lcd_msg.payload.console.y_offset = 110;
    lcd_msg.command = LCD_CONSOLE_DRAW_MESSAGE;

    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);
  while(1){
    (void)arg; // Unused parameter
    ////////////////////////////////////////////////////////////////////////
    // *** RESET ALL GLOBAL VARIABLES ***
    task_console_printf("=== RESETTING GAME STATE ===\r\n");

    //toggle player id
    if(player_id == 0){
        player_id = 1;
        ship_border_color = BATTLESHIP_PLAYER_1_COLOR;
    } else if(player_id ==1){
        player_id = 0;
        ship_border_color = BATTLESHIP_PLAYER_0_COLOR;
    }
    
    // Reset cursor position
    cursor_row = 0;
    cursor_col = 0;
    // Set num_turns to 1;
    
    // Reset ship placement globals
    ship_orientation_horizontal = true;
    current_ship_type = BATTLESHIP_TYPE_CARRIER;
    ship_placement_active = false;
    num_ships_placed = 0;
    
    // Clear ship_board
    for(int r = 0; r < 10; r++) {
        for(int c = 0; c < 10; c++) {
            ship_board[r][c] = 0;
        }
    }
    
    // Clear our_ships array
    for(int i = 0; i < 5; i++) {
        our_ships[i].start_row = 0;
        our_ships[i].start_col = 0;
        our_ships[i].length = 0;
        our_ships[i].horizontal = true;
        our_ships[i].hits = 0;
        our_ships[i].sunk = false;
    }
    
    // Clear hit tracking boards
    for(int r = 0; r < 10; r++) {
        for(int c = 0; c < 10; c++) {
            hit_board[r][c] = -2;
            opp_hits_miss_track[r][c] = -2;
            our_hits_miss_track[r][c] = -2;
        }
    }
    
    // Reset gameplay counters
    active_ships = 5;
    opponent_active_ships = 5;
    num_turns = 0;
    num_hits = 0;
    num_misses = 0;
    
    // Clear any stale event bits
    xEventGroupClearBits(ECE353_RTOS_Events, 
        ECE353_RTOS_EVENTS_SHIP_PLACED | 
        ECE353_RTOS_START_GAME |
        ECE353_RTOS_ALL_PLAYERS_READY_ACK |
        ECE353_RTOS_ALL_PLAYERS_READY_IPC |
        ECE353_RTOS_GAME_WON_ACK);
    
    // Drain IPC queue
    ipc_packet_t drain_packet;
    while(xQueueReceive(Queue_IPC_Rx, &drain_packet, 0) == pdPASS)
    {
        task_console_printf("Drained stale IPC packet: cmd=%d\r\n", drain_packet.cmd);
    }
    
    // Clear saved ships in battleship module (if applicable)
    battleship_clear_saved_ships();

    /////////////////////////////////////////////////////////////////////////////////////////////


    


    events = xEventGroupWaitBits(ECE353_RTOS_Events,
                                    ECE353_RTOS_EVENTS_SW1 | ECE353_RTOS_IPC_RX,
                                    pdTRUE,
                                    pdFALSE,
                                    portMAX_DELAY);
    
    if(events & ECE353_RTOS_EVENTS_SW1)
    {
        if(iteration ==0){
            //also we are player 0 here
            player_id = 0;//test
            ship_border_color = BATTLESHIP_PLAYER_0_COLOR;
        }

        // We initiated - send NEW_GAME and wait for ACK
        printf("SW1 Pressed - Sending NEW_GAME request\n\r");
        ipc_send_game_control(IPC_GAME_CONTROL_NEW_GAME);
        
        // Wait for opponent's ACK
        ipc_packet_t rx_packet;
        if(xQueueReceive(Queue_IPC_Rx, &rx_packet, portMAX_DELAY) == pdPASS)
        {
            if(rx_packet.cmd == IPC_CMD_GAME_CONTROL && 
               rx_packet.game_control == IPC_GAME_CONTROL_ACK)
            {
                printf("Received ACK - Starting Game\n\r");
            }
            else
            {
                printf("Unexpected response, starting anyway\n\r");
            }
        }
        else
        {
            printf("Timeout waiting for ACK, starting anyway\n\r");
        }
    }
    else if(events & ECE353_RTOS_IPC_RX)
    {
        // we are player 1 here
        if(iteration == 0){
            player_id = 1;
            ship_border_color = BATTLESHIP_PLAYER_1_COLOR;
        }

        // Opponent initiated - receive their NEW_GAME and send ACK
        printf("Received NEW_GAME request from opponent\n\r");
        
        // Consume the NEW_GAME packet from queue
        ipc_packet_t rx_packet;
        xQueueReceive(Queue_IPC_Rx, &rx_packet, 0); // Non-blocking, should be there
        
        // Send ACK
        ipc_send_game_control(IPC_GAME_CONTROL_ACK);
        printf("Sent ACK - Starting Game\n\r");
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Clear the screen plus we start again after game ends from here
    lcd_msg.command = LCD_CMD_CLEAR_SCREEN;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

    //Draw the initial game board
    // Draw the game board
    lcd_msg.command = LCD_CMD_DRAW_BOARD;
    xQueueSend(xQueue_LCD, &lcd_msg, portMAX_DELAY);

    // Wait for board to finish drawing
    vTaskDelay(pdMS_TO_TICKS(200));

    xTaskNotifyGive(gatekeeper); //notify gatekeeper to start
    xTaskNotifyGive(imu_monitor);

    // Signal gatekeeper task to draw stats
    //xEventGroupSetBits(ECE353_RTOS_Events, ECE353_RTOS_EVENTS_SW1);
    /////////////////////////////////////////////////////////////////////
    // use set bits to signify end of ship placement and start of gameplay
    // assume ship placement is done for me
    /////////////////////////////////////////////////////////////////////
    xEventGroupWaitBits(ECE353_RTOS_Events, ECE353_RTOS_START_GAME,
                         pdTRUE, pdTRUE, portMAX_DELAY);
    
    ////////////////////////////////////////////////////////////////////
    // Initialize gameplay - call directly instead of creating new task
    ///////////////////////////////////////////////////////////////////
    gameplay_init();

    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); //wait for gameplay to finish

    iteration++;
    
    }
    // ////////////////////////////////////////////////////////////////////////
    // // Gameplay phase
    // ipc_result_t result = IPC_RESULT_MISS;
    // ipc_game_control_t game_control = IPC_GAME_CONTROL_NEW_GAME;
    // ipc_error_t error = IPC_ERROR_CHECKSUM;

    // while(1)
    // {
    //     // Wait for SW1 or SW2 to be pressed
    //     events = xEventGroupWaitBits(ECE353_RTOS_Events,
    //                                  ECE353_RTOS_EVENTS_SW3,
    //                                 pdTRUE,
    //                                 pdFALSE,
    //                                 portMAX_DELAY);

        
    //     if(events & ECE353_RTOS_EVENTS_SW3)
    //     {
    //         if(!ipc_send_error(error))
    //         {
    //             // Handle send failure (optional)
    //         }
    //         error++;

    //         if(error > IPC_ERROR_SYSTEM_FAILURE)
    //         {
    //             error = IPC_ERROR_CHECKSUM;
    //         }       
    //     }

    // }
}

bool task_system_control_resources_init(void)
{
    /* Create the I2C Semaphore */
    Semaphore_I2C = xSemaphoreCreateMutex();
    if (Semaphore_I2C == NULL)
    {
        return false;
    }

    /* Create the SPI Semaphore */
    Semaphore_SPI = xSemaphoreCreateMutex();
    if (Semaphore_SPI == NULL)
    {
        return false;
    }

    //initialize the response queues
    Queue_System_Control_Responses = xQueueCreate(20, sizeof(device_response_msg_t));
    if (Queue_System_Control_Responses == NULL)
    {
        return false;
    }

    Queue_Light_Monitor_Responses = xQueueCreate(10, sizeof(device_response_msg_t));
    if (Queue_Light_Monitor_Responses == NULL)
    {
        return false;
    }

    Queue_IMU_Monitor_Responses = xQueueCreate(10, sizeof(device_response_msg_t));
    if (Queue_IMU_Monitor_Responses == NULL)
    {
        return false;
    }


    /* Create the System Control Task */
    if (xTaskCreate(
            task_system_control,
            "System Control Task",
            configMINIMAL_STACK_SIZE*20, //changed it to 40 //checking if it works
            NULL,
            tskIDLE_PRIORITY + 1,
            &system_control) != pdPASS)
    {
        return false;
    }

    return true;
}

void gatekeeper_task(void *pvParameters)
{
    (void)pvParameters;

    while(1)
    {
        // Wait for notification from system_control to draw stats
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        // Draw initial stats for this game
        draw_game_stats();
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

    // Initialize the LEDs
    rslt = leds_init_gpio();
    if(rslt != CY_RSLT_SUCCESS)
    {
        printf("LED initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    rslt = buttons_init_gpio();

    if(rslt != CY_RSLT_SUCCESS)
    {
        printf("Button initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    // Initialize joystick
    rslt = joystick_init();
    if(rslt != CY_RSLT_SUCCESS)
    {
        printf("Joystick initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    //ex08
    rslt = lcd_initialize();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("LCD initialization failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    // HW05 files 

    /* Initialize the i2c interface */
    I2C_Obj = i2c_init(PIN_I2C_SDA, PIN_I2C_SCL);
    if (I2C_Obj == NULL)
    {
        printf("I2C initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    /* Initialize the spi interface */
    SPI_Obj = spi_init(PIN_SPI_MOSI, PIN_SPI_MISO, PIN_SPI_CLK);
    if (SPI_Obj == NULL)
    {
        printf("SPI initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }
    cyhal_gpio_init(PIN_IMU_CS, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, true);
    //we will only use IMU

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
    // Initialize the EventGroup
    ECE353_RTOS_Events = xEventGroupCreate();
    
    //IPC INIT from ex 08
    if(!task_button_init())
    {
        printf("Button initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    if(!task_console_init())
    {
        printf("Console initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    if(!task_ipc_init())
    {
        printf("IPC initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    //EX 08 INIT and ICE 08 file
    /* Initialize LCD resources */
    if (!task_lcd_init())
    {
        printf("Failed to initialize lcd task\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    // HW05 code segments
    if(!task_system_control_resources_init())
    {
        printf("System Control Task initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    if(!task_io_expander_resources_init(I2C_Obj, &Semaphore_I2C))
    {
        printf("IO Expander Task initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    if(!task_light_sensor_resources_init(I2C_Obj, &Semaphore_I2C))
    {
        printf("Light Sensor Task initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    if(!task_imu_resources_init(&Semaphore_SPI, SPI_Obj, PIN_IMU_CS))
    {
        printf("IMU Task initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    /* Create the LCD gatekeeper task */
    xTaskCreate(
        gatekeeper_task,
        "Task LCD Gatekeeper",
        configMINIMAL_STACK_SIZE * 5,
        NULL,
        tskIDLE_PRIORITY + 1,
        &gatekeeper
    );

    /* Create the light monitor task for automatic theme switching */
    xTaskCreate(
        task_light_monitor,
        "Light Monitor",
        configMINIMAL_STACK_SIZE * 3,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL
    );

    /* Create the IMU monitor task */
    xTaskCreate(
        task_imu_monitor,
        "IMU Monitor",
        configMINIMAL_STACK_SIZE * 3,
        NULL,
        tskIDLE_PRIORITY + 1,
        &imu_monitor
    );

    /* Create the ship orientation monitor task */
    xTaskCreate(
        task_ship_orientation_monitor,
        "Ship Orientation Monitor",
        configMINIMAL_STACK_SIZE * 2,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL
    );

    xTaskCreate(
        game_monitor_task,
        "Game Monitor Task",
        configMINIMAL_STACK_SIZE * 10,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL
    );

    /* Start the scheduler*/
    vTaskStartScheduler();

    /* Will never reach this loop once the scheduler starts */
    while (1)
    {
    }
}
#endif