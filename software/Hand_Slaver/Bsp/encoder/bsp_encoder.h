#ifndef __bsp_encoder_h_
#define __bsp_encoder_h_
#include "main.h"
#include "bsp_motor.h"

#define ENCODER_LINES 16	/* 编码器线数 */
#define GEAR_RATIO 12.0f	/* 电机减速比 */
#define ANGLE_PER_PULSE (360.0f / (ENCODER_LINES * 4 * GEAR_RATIO))/* 每个脉冲对应的角度 */

/* 定义编码器配置结构体 */
typedef struct {
    TIM_HandleTypeDef* tim_handle;
    uint32_t cc1_channel;
    uint32_t cc2_channel;
} Encoder_Config;

/**
  * @brief   编码器初始化
*/
void encoder_init();

/**
  * @brief   获取当前角度（单位：度)
  * @param   motor_id: 电机编号
  *            @arg 1-3: 对应电机编号
  * @retval  angle:角度值
*/
float get_motor_angle(Motor_Id_t motor_id);


#endif