/**
 * @file task_imu.h
 * @author Krishna Bharadwaj (kbharadwaj@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-09-16
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __TASK_SENSOR_FUSION_H__
#define __TASK_SENSOR_FUSION_H__

#include "cyhal_hw_types.h"
#include "main.h"
#if defined(ECE353_FREERTOS)
#include "cyhal_spi.h"
#include "task_imu.h"
#include "devices.h"

#endif /* ECE353_FREERTOS */

void task_sensor_fusion(int16_t* accel_data, int16_t* gyro_data);
void sensor_fusion_detect_tilt_event(float pitch, float roll);
void sensor_fusion_reset(void);
float sensor_fusion_get_pitch(void);
float sensor_fusion_get_roll(void);
int sensor_fusion_get_tilt_state(void);

#endif /* __TASK_SENSOR_FUSION_H__ */