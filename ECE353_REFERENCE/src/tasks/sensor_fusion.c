/**
 * @file sensor_fusion.c
 * @author Krishna Bharadwaj (kbharadwaj@wisc.edu)
 * @brief Sensor fusion task that processes accelerometer and gyroscope data
 * @version 0.1
 * @date 2025-12-02
 * 
 * @copyright Copyright (c) 2025 Krishna Bharadwaj
 * 
 */

#include "sensor_fusion.h"

#if defined(ECE353_FREERTOS)
#include "task_console.h"
#include <math.h>

// Define M_PI if not already defined (not part of C standard)
#ifndef M_PI
#define M_PI 3.1416f
#endif

// Complementary filter coefficient (alpha)
// Higher value gives more weight to gyroscope, lower to accelerometer
#define ALPHA 0.98f

// Time step for integration (in seconds)
// Assuming 250ms delay between readings
#define DT 0.25f

// Tilt detection thresholds (in degrees)
#define TILT_THRESHOLD_ENTER    10.0f   // Degrees to trigger tilt event (lowered for easier testing)
#define TILT_THRESHOLD_EXIT     5.0f    // Degrees to clear tilt state (hysteresis)

// Tilt state enumeration
typedef enum {
    TILT_STATE_LEVEL,
    TILT_STATE_TILTED_FORWARD,
    TILT_STATE_TILTED_BACKWARD,
    TILT_STATE_TILTED_LEFT,
    TILT_STATE_TILTED_RIGHT
} tilt_state_t;

// Global variables to store filtered angles
static float pitch = 0.0f;
static float roll = 0.0f;

// Tilt detection state
static tilt_state_t current_tilt_state = TILT_STATE_LEVEL;

/**
 * @brief 
 * Performs sensor fusion using complementary filter to combine 
 * to calculate final square to place the ship in
 * 
 * The complementary filter combines:
 * - Gyroscope: Good for short-term accuracy, drifts over time
 * - Accelerometer: Good for long-term accuracy, noisy in short-term
 * 
 * @param accel_data Array containing [accel_x, accel_y, accel_z] in raw ADC values
 * @param gyro_data Array containing [gyro_x, gyro_y, gyro_z] in raw ADC values
 */
void task_sensor_fusion(int16_t* accel_data, int16_t* gyro_data)
{

    

    // Print the fused sensor data (uncomment for debugging)
    task_console_printf("Pitch: %.2f deg, Roll: %.2f deg\r\n", pitch, roll);
}

#endif /* ECE353_FREERTOS */
