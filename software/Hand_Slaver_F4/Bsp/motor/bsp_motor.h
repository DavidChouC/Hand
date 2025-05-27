#ifndef __bsp_motor_h_
#define __bsp_motor_h_
#include "main.h"

/* 电机数量 */
#define MOTOR_COUNT 3

/* 电机编号 */
typedef enum {
    MOTOR_ALL = 0,
    MOTOR_1,
    MOTOR_2,
    MOTOR_3,
} Motor_Id_t;

/* 电机配置结构体 */
typedef struct {
    TIM_HandleTypeDef* tim_handle;  /* 定时器句柄 */
    uint32_t in1_channel;           /* IN1通道 */
    uint32_t in2_channel;           /* IN2通道 */
    uint16_t max_compare;           /* 最大比较值 */
} Motor_Config_t;

/* 电机控制参数结构体 */
typedef struct {
    Motor_Id_t motor_id;    /* 电机编号 */
    int16_t speed;          /* 速度值 (-1000 ~ 1000) */
} Motor_Ctrl_Param_t;

/**
  * @brief   初始化所有电机的PWM通道
  * @retval  HAL_StatusTypeDef: HAL状态
*/
HAL_StatusTypeDef motor_init(void);

/**
  * @brief   设置电机速度
  * @param   param: 电机控制参数
  * @retval  HAL_StatusTypeDef: HAL状态
  * @note    速度范围为-1000到1000，负值表示反转
*/
HAL_StatusTypeDef motor_set_speed(const Motor_Ctrl_Param_t* param);

/**
  * @brief   电机刹车
  * @param   motor_id: 电机编号
  * @retval  HAL_StatusTypeDef: HAL状态
*/
HAL_StatusTypeDef motor_brake(Motor_Id_t motor_id);

/**
  * @brief   设置电机滑行模式（停止PWM输出）
  * @param   motor_id: 电机编号
  * @retval  HAL_StatusTypeDef: HAL状态
*/
HAL_StatusTypeDef motor_coast(Motor_Id_t motor_id);

#endif
