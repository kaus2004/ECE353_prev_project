/**
 * @file battleship.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-20
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #include "battleship.h"

 #ifdef ECE353_FREERTOS

/* Ship tracking storage */
#define MAX_SHIPS 5

typedef struct {
    uint8_t row;
    uint8_t col;
    uint16_t border_color;
    uint16_t fill_color;
    battleship_type_t type;
    bool horizontal;
    bool is_active;  // Track if this slot is used
} ship_placement_t;

static ship_placement_t ships[MAX_SHIPS];
static uint8_t ship_count = 0;

/* Theme state */
typedef struct {
    uint16_t background_color;
    uint16_t board_border_color;
    uint16_t board_fill_color;
} theme_t;

static theme_t current_theme = {
    .background_color = LCD_COLOR_BLACK,
    .board_border_color = BATTLESHIP_PLAYER_0_COLOR,
    .board_fill_color = LCD_COLOR_BLACK
};

/**
 * @brief Get the coordinates of a box on the LCD screen.
 * 
 * @param coord Pointer to the lcd_coord_t structure to store the coordinates.
 * @param col The column of the box (0-9).
 * @param row The row of the box (0-9).
 * @return true if the coordinates were successfully calculated, false otherwise.
 */
bool battleship_get_box_coordinates(lcd_coord_t *coord, uint8_t col, uint8_t row)
{
    if( row < 10 && col < 10)
    {
        // Add +1 pixel spacing to prevent border overlap
        coord->x = BATTLE_SHIP_LEFT_MARGIN + (col *( BATTLESHIP_BOX_WIDTH+1));//before +1
        coord->y = BATTLE_SHIP_TOP_MARGIN + (row * (BATTLESHIP_BOX_HEIGHT+1));//before +1
        return true;
    }
    else {
        return false; // Invalid row or column
    }
}

/**
 * @brief 
 * Used to draw an empty game board for the specified player.
 * @param player_id 
 * @return true 
 * @return false 
 */
bool battleship_draw_game_board(uint8_t player_id)
{
    //we draw the square 20x20 and 16x16 inside it
    lcd_coord_t coord;
    uint16_t border_color;
    uint16_t fill_color;
    //for light and dark themes
    if(current_theme.background_color == LCD_COLOR_WHITE){
        fill_color = LCD_COLOR_WHITE;
    } else{
        fill_color = LCD_COLOR_BLACK;
    }


    //ignore player id for now
    if(player_id == 0){
        border_color = BATTLESHIP_PLAYER_0_COLOR;
        //fill_color = LCD_COLOR_BLACK;
    } else{
        border_color = BATTLESHIP_PLAYER_1_COLOR;
        //fill_color = LCD_COLOR_BLACK;
    }
    // border_color = BATTLESHIP_PLAYER_0_COLOR;
    // fill_color = LCD_COLOR_BLACK;  
    for(int row = 0; row < 10; row++)
    {
        for(int col = 0; col < 10; col++)
        {
            if(!battleship_get_box_coordinates(&coord, col, row))
            {
                return false; // Failed to get box coordinates
            }
            // Draw the outer border 20 x 20 pixels
            lcd_draw_rectangle(
                coord.x, 
                coord.y, 
                BATTLESHIP_BOX_WIDTH, 
                BATTLESHIP_BOX_HEIGHT, 
                border_color, 
                false); //true before
            
            // Draw the inner fill 16 x 16
            // lcd_draw_rectangle(
            //     coord.x + BATTLESHIP_BORDER_WIDTH/4, 
            //     coord.y + BATTLESHIP_BORDER_WIDTH/4, 
            //     BATTLESHIP_BOX_WIDTH - BATTLESHIP_BORDER_WIDTH, 
            //     BATTLESHIP_BOX_HEIGHT - BATTLESHIP_BORDER_WIDTH, 
            //     fill_color, 
            //     true);

            const uint16_t inner_x = coord.x + (BATTLESHIP_BORDER_WIDTH / 2);
            const uint16_t inner_y = coord.y + (BATTLESHIP_BORDER_WIDTH / 2);
            const uint16_t inner_w = BATTLESHIP_BOX_WIDTH - BATTLESHIP_BORDER_WIDTH;
            const uint16_t inner_h = BATTLESHIP_BOX_HEIGHT - BATTLESHIP_BORDER_WIDTH;

            // Draw the inner fill (top-left coordinates, not centered)
            lcd_draw_rectangle(
                inner_x,
                inner_y,
                inner_w,
                inner_h,
                fill_color,
                false);  // NOT centered: treat inner_x/inner_y as top-left
        }
    }
    return true;
}

/**
 * @brief 
 * Draws the cursor for the currently active location by changing
 * the color of the box border.
 * @param col 
 * @param row 
 * @return true 
 * @return false 
 */
bool battleship_draw_cursor(uint8_t col, uint8_t row, uint8_t player_id)
{
    //drawing single set of concentric boxes
    lcd_coord_t coord;
    uint16_t border_color = player_id == 0 ? BATTLESHIP_PLAYER_0_COLOR : BATTLESHIP_PLAYER_1_COLOR;
    uint16_t fill_color;
    
    // always
    fill_color = BATTLESHIP_CURSOR_COLOR;
    
    if(!battleship_get_box_coordinates(&coord, col, row))
    {
        return false; // Failed to get box coordinates
    }
    // Draw the cursor
    lcd_draw_rectangle(
        coord.x,
        coord.y,
        BATTLESHIP_BOX_WIDTH,
        BATTLESHIP_BOX_HEIGHT,
        border_color,
        false);
    lcd_draw_rectangle(
        coord.x + BATTLESHIP_BORDER_WIDTH,
        coord.y + BATTLESHIP_BORDER_WIDTH,
        BATTLESHIP_BOX_WIDTH - (2 * BATTLESHIP_BORDER_WIDTH),
        BATTLESHIP_BOX_HEIGHT - (2 * BATTLESHIP_BORDER_WIDTH),
        fill_color,
        false);
    
    return true;
}

bool battleship_draw_hit_marker(uint8_t col, uint8_t row, uint8_t player_id){
    // base it off draw cursor but use different color
    //drawing single set of concentric boxes
    lcd_coord_t coord;
    uint16_t border_color = player_id == 0 ? BATTLESHIP_PLAYER_0_COLOR : BATTLESHIP_PLAYER_1_COLOR;
    uint16_t fill_color;
    
    // always
    fill_color = BATTLESHIP_HIT_COLOR;
    
    if(!battleship_get_box_coordinates(&coord, col, row))
    {
        return false; // Failed to get box coordinates
    }
    // Draw the cursor
    lcd_draw_rectangle(
        coord.x,
        coord.y,
        BATTLESHIP_BOX_WIDTH,
        BATTLESHIP_BOX_HEIGHT,
        border_color,
        false);
    lcd_draw_rectangle(
        coord.x + BATTLESHIP_BORDER_WIDTH,
        coord.y + BATTLESHIP_BORDER_WIDTH,
        BATTLESHIP_BOX_WIDTH - (2 * BATTLESHIP_BORDER_WIDTH),
        BATTLESHIP_BOX_HEIGHT - (2 * BATTLESHIP_BORDER_WIDTH),
        fill_color,
        false);
    
    return true;

}

//for miss marker
bool battleship_draw_miss_marker(uint8_t col, uint8_t row, uint8_t player_id){
    // base it off draw cursor but use different color
    //drawing single set of concentric boxes
    lcd_coord_t coord;
    uint16_t border_color = player_id == 0 ? BATTLESHIP_PLAYER_0_COLOR : BATTLESHIP_PLAYER_1_COLOR;
    uint16_t fill_color;
    
    // always
    fill_color = BATTLESHIP_MISS_COLOR;
    
    if(!battleship_get_box_coordinates(&coord, col, row))
    {
        return false; // Failed to get box coordinates
    }
    // Draw the cursor
    lcd_draw_rectangle(
        coord.x,
        coord.y,
        BATTLESHIP_BOX_WIDTH,
        BATTLESHIP_BOX_HEIGHT,
        border_color,
        false);
    lcd_draw_rectangle(
        coord.x + BATTLESHIP_BORDER_WIDTH,
        coord.y + BATTLESHIP_BORDER_WIDTH,
        BATTLESHIP_BOX_WIDTH - (2 * BATTLESHIP_BORDER_WIDTH),
        BATTLESHIP_BOX_HEIGHT - (2 * BATTLESHIP_BORDER_WIDTH),
        fill_color,
        false);
    
    return true;

}

/**
 * @brief 
 *  Restores a box border to the color of the game board 
 * @param col 
 * @param row 
 * @param player_id 
 * @return true 
 * @return false 
 */
bool battleship_clear_cursor(uint8_t col, uint8_t row, uint8_t player_id)
{
    //restores the previous square to previous color 
    lcd_coord_t coord;
    uint16_t border_color;
    uint16_t fill_color;
    
    //ignore player id for now
    border_color = player_id == 0 ? BATTLESHIP_PLAYER_0_COLOR : BATTLESHIP_PLAYER_1_COLOR;
    
    // Use theme-aware fill color
    if(current_theme.background_color == LCD_COLOR_WHITE){
        fill_color = LCD_COLOR_WHITE;
    } else {
        fill_color = LCD_COLOR_BLACK;
    }
    
    if(!battleship_get_box_coordinates(&coord, col, row))
    {
        return false; // Failed to get box coordinates
    }

    // Restore the box to the previous color
    lcd_draw_rectangle(
        coord.x,
        coord.y,
        BATTLESHIP_BOX_WIDTH,
        BATTLESHIP_BOX_HEIGHT,
        border_color,
        false);
    // Draw the inner fill 16 x 16
    lcd_draw_rectangle(
        coord.x + BATTLESHIP_BORDER_WIDTH/2,
        coord.y + BATTLESHIP_BORDER_WIDTH/2,
        BATTLESHIP_BOX_WIDTH - BATTLESHIP_BORDER_WIDTH,
        BATTLESHIP_BOX_HEIGHT - BATTLESHIP_BORDER_WIDTH,
        fill_color,
        false);
    return true;
}

// typedef struct{
//     uint8_t row;            // Row (0-9)
//     uint8_t col;            // Column (0-9)
//     uint16_t border_color;  // Border color
//     uint16_t fill_color;    // Fill color
//     battleship_type_t type; // Type of ship
//     bool horizontal;        // Orientation of ship
// } battleship_payload_t;
/**
 * @brief 
 * Used to draw a ship on the game board for the specified player.
 * @param player_id 
 * @return true 
 * @return false 
 */
bool battleship_draw_ship(uint8_t col, uint8_t row, uint16_t border_color,uint16_t fill_color, battleship_type_t type, bool horizontal, uint8_t player_id)
{
    //check row and column to see they within the board 0 to 9 valid entries
    if( row >=10 || col >=10){
        printf("Invalid starting position for ship at: (%i,%i)",row, col);
        return false;
    }
    //otherwise start point is outside board

    //since row/col inside, check if orientation is inside board for ship type
    //get length of battleship then add that to row/column to see if its inside,
    //else return if col exceed height or row exceed width
    int ship_length = 0;
    switch(type){
        case BATTLESHIP_TYPE_CARRIER:
            ship_length = 5;
            break;
        case BATTLESHIP_TYPE_BATTLESHIP:
            ship_length = 4;
            break;
        case BATTLESHIP_TYPE_CRUISER:
            ship_length = 3;
            break;
        case BATTLESHIP_TYPE_SUBMARINE:
            ship_length = 3;
            break;
        case BATTLESHIP_TYPE_DESTROYER:
            ship_length = 2;
            break;
        default:
            printf("Invalid ship type for ship at: (%i,%i)\n\r",row, col);
            return false; //invalid ship type
    }
    if(horizontal){
        //check if col + ship length exceeds 9
        if( (col + ship_length -1) >=10){
            printf("Ship exceeds board boundaries at: (%i,%i)\n\r",row, col);
            return false;
        }
    } else {
        //vertical check if row + ship length exceeds 9
        if( (row + ship_length -1) >=10){
            printf("Ship exceeds board boundaries at: (%i,%i)\n\r",row, col);
            return false;
        }
    }
    //printf("Successfully Drawn the ship at: (%i,%i)\n\r",row, col);

    //if valid draw the ship then return success
    lcd_coord_t coord;
    for(int i = 0; i < ship_length; i++){
        uint8_t draw_row = row;
        uint8_t draw_col = col;
        if(horizontal){
            draw_col = col + i;
        } else {
            draw_row = row + i;
        }
        if(!battleship_get_box_coordinates(&coord, draw_col, draw_row))
        {
            return false; // Failed to get box coordinates
        }
        // Draw the outer border 20 x 20 pixels
        lcd_draw_rectangle(
            coord.x, 
            coord.y, 
            BATTLESHIP_BOX_WIDTH, 
            BATTLESHIP_BOX_HEIGHT, 
            border_color, 
            false); //true before

        // // Draw the inner fill 16 x 16
        // lcd_draw_rectangle(
        //     coord.x + BATTLESHIP_BORDER_WIDTH/4, 
        //     coord.y + BATTLESHIP_BORDER_WIDTH/4, 
        //     BATTLESHIP_BOX_WIDTH - BATTLESHIP_BORDER_WIDTH, 
        //     BATTLESHIP_BOX_HEIGHT - BATTLESHIP_BORDER_WIDTH, 
        //     fill_color, 
        //     true);

        const uint16_t inner_x = coord.x + (BATTLESHIP_BORDER_WIDTH / 2);
        const uint16_t inner_y = coord.y + (BATTLESHIP_BORDER_WIDTH / 2);
        const uint16_t inner_w = BATTLESHIP_BOX_WIDTH - BATTLESHIP_BORDER_WIDTH;
        const uint16_t inner_h = BATTLESHIP_BOX_HEIGHT - BATTLESHIP_BORDER_WIDTH;

        // Draw the inner fill (top-left coordinates, not centered)
        lcd_draw_rectangle(
            inner_x,
            inner_y,
            inner_w,
            inner_h,
            fill_color,
            false);  // NOT centered: treat inner_x/inner_y as top-left
    }

    return true;
}
/*The battleship_clear_ship function erases the battleship.
*/
bool battleship_clear_ship(battleship_payload_t *battleship, uint8_t player_id){
    //we have ship row and column, orientation, and length
    int ship_length = 0;
    uint8_t row = battleship->row;
    uint8_t col = battleship->col;
    battleship_type_t type = battleship->type;
    bool horizontal = battleship -> horizontal;
    switch(type){
        case BATTLESHIP_TYPE_CARRIER:
            ship_length = 5;
            break;
        case BATTLESHIP_TYPE_BATTLESHIP:
            ship_length = 4;
            break;
        case BATTLESHIP_TYPE_CRUISER:
            ship_length = 3;
            break;
        case BATTLESHIP_TYPE_SUBMARINE:
            ship_length = 3;
            break;
        case BATTLESHIP_TYPE_DESTROYER:
            ship_length = 2;
            break;
        default:
            printf("Invalid ship type for ship at: (%i,%i)\n\r", battleship->row, battleship->col);
            return false; //invalid ship type
    }
    if(horizontal){
        //check if col + ship length exceeds 9
        if( (col + ship_length -1) >=10){
            printf("Ship exceeds board boundaries at: (%i,%i)\n\r",row, col);
            return false;
        }
    } else {
        //vertical check if row + ship length exceeds 9
        if( (row + ship_length -1) >=10){
            printf("Ship exceeds board boundaries at: (%i,%i)\n\r",row, col);
            return false;
        }
    }
    // we have the length.
    lcd_coord_t coord;
    for(int i=0; i<ship_length; i++){
        uint8_t draw_row = row;
        uint8_t draw_col = col;
        if(horizontal){
            draw_col = col + i;
        } else {
            draw_row = row + i;
        }
        if(!battleship_get_box_coordinates(&coord,draw_col, draw_row))
        {
            return false; // Failed to get box coordinates
        }

        lcd_draw_rectangle(
            coord.x, 
            coord.y, 
            BATTLESHIP_BOX_WIDTH, 
            BATTLESHIP_BOX_HEIGHT, 
            battleship->border_color, 
            false); //true before

        const uint16_t inner_x = coord.x + (BATTLESHIP_BORDER_WIDTH / 2);
        const uint16_t inner_y = coord.y + (BATTLESHIP_BORDER_WIDTH / 2);
        const uint16_t inner_w = BATTLESHIP_BOX_WIDTH - BATTLESHIP_BORDER_WIDTH;
        const uint16_t inner_h = BATTLESHIP_BOX_HEIGHT - BATTLESHIP_BORDER_WIDTH;

        // Use theme-aware fill color
        uint16_t fill_color;
        if(current_theme.background_color == LCD_COLOR_WHITE){
            fill_color = LCD_COLOR_WHITE;
        } else {
            fill_color = LCD_COLOR_BLACK;
        }

        // Draw the inner fill (top-left coordinates, not centered)
        lcd_draw_rectangle(
            inner_x,
            inner_y,
            inner_w,
            inner_h,
            fill_color,
            false);  // NOT centered: treat inner_x/inner_y as top-left
    }
    return true;
    
}

/**
 * @brief
 * Saves a ship placement to the tracking array.
 * 
 * @param col Column position
 * @param row Row position  
 * @param border_color Border color of the ship
 * @param fill_color Fill color of the ship
 * @param type Type of ship
 * @param horizontal Orientation
 * @return true if ship was saved successfully
 * @return false if ship array is full
 */
bool battleship_save_ship(uint8_t col, uint8_t row, uint16_t border_color,
                          uint16_t fill_color, battleship_type_t type, bool horizontal)
{
    if (ship_count >= MAX_SHIPS) {
        printf("Error: Maximum ships reached\n\r");
        return false;
    }

    ships[ship_count].col = col;
    ships[ship_count].row = row;
    ships[ship_count].border_color = border_color;
    ships[ship_count].fill_color = fill_color;
    ships[ship_count].type = type;
    ships[ship_count].horizontal = horizontal;
    ships[ship_count].is_active = true;
    ship_count++;

    return true;
}

/**
 * @brief
 * Redraws all saved ships. Useful after theme change.
 * 
 * @param player_id Player ID
 * @return true if all ships were redrawn successfully
 * @return false if any ship failed to redraw
 */
bool battleship_redraw_all_ships(uint8_t player_id)
{
    for (uint8_t i = 0; i < ship_count; i++) {
        if (ships[i].is_active) {
            if (!battleship_draw_ship(
                ships[i].col,
                ships[i].row,
                ships[i].border_color,
                ships[i].fill_color,
                ships[i].type,
                ships[i].horizontal,
                player_id))
            {
                printf("Failed to redraw ship %d\n\r", i);
                return false;
            }
        }
    }
    return true;
}

/**
 * @brief
 * Clears all saved ship placements.
 */
void battleship_clear_saved_ships(void)
{
    for (uint8_t i = 0; i < MAX_SHIPS; i++) {
        ships[i].is_active = false;
    }
    ship_count = 0;
}

/**
 * @brief
 * Changes the theme (background and board colors) and redraws everything.
 * 
 * @param bg_color New background color
 * @param board_border New board border color
 * @param board_fill New board fill color
 * @param player_id Player ID
 * @return true if theme was changed successfully
 * @return false if redraw failed
 */
bool battleship_change_theme(uint16_t bg_color, uint16_t board_border,
                             uint16_t board_fill, uint8_t player_id)
{
    // Update theme
    current_theme.background_color = bg_color;
    current_theme.board_border_color = board_border;
    current_theme.board_fill_color = board_fill;

    // Clear screen with new background
    lcd_clear_screen(bg_color);

    // Redraw board with new colors
    if (!battleship_draw_game_board(player_id)) {
        return false;
    }

    // Redraw all ships
    if (!battleship_redraw_all_ships(player_id)) {
        return false;
    }

    return true;
}

/**
 * @brief
 * Gets the current theme settings.
 * 
 * @param theme Pointer to store theme settings
 */
void battleship_get_theme(uint16_t *bg_color, uint16_t *board_border, uint16_t *board_fill)
{
    if (bg_color) *bg_color = current_theme.background_color;
    if (board_border) *board_border = current_theme.board_border_color;
    if (board_fill) *board_fill = current_theme.board_fill_color;
}

#endif