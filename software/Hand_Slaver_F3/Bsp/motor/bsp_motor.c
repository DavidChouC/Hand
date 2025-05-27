#include "bsp_motor.h"
#include "tim.h"
#include <stdlib.h>

/* 电机配置数组（索引0对应电机1，依此类推） */
static Motor_Config_t motor_configs[] = {
    {&htim8, TIM_CHANNEL_1, TIM_CHANNEL_2, 0},
    {&htim5, TIM_CHANNEL_1, TIM_CHANNEL_2, 0},
    {&htim4, TIM_CHANNEL_1, TIM_CHANNEL_2, 0}
};

/* 内部函数声明 */
static inline HAL_StatusTypeDef motor_validate_param(const Motor_Ctrl_Param_t* param);
static inline void motor_set_compare(uint8_t motor_idx, uint16_t in1_compare, uint16_t in2_compare);

/**
  * @brief   初始化所有电机的PWM通道
  * @retval  HAL_StatusTypeDef: HAL状态
*/
HAL_StatusTypeDef motor_init(void) {
    for(uint8_t i = 0; i < MOTOR_COUNT; i++) {
        motor_configs[i].max_compare = __HAL_TIM_GetAutoreload(motor_configs[i].tim_handle);
        HAL_TIM_PWM_Start(motor_configs[i].tim_handle, motor_configs[i].in1_channel);
        HAL_TIM_PWM_Start(motor_configs[i].tim_handle, motor_configs[i].in2_channel);
    }
    return HAL_OK;
}

/**
  * @brief   设置电机速度
  * @param   param: 电机控制参数
  * @retval  HAL_StatusTypeDef: HAL状态
*/
HAL_StatusTypeDef motor_set_speed(const Motor_Ctrl_Param_t* param) {
    if(motor_validate_param(param) != HAL_OK) {
        return HAL_ERROR;
    }

    uint8_t start_idx = (param->motor_id == MOTOR_ALL) ? 0 : (param->motor_id - 1);
    uint8_t end_idx = (param->motor_id == MOTOR_ALL) ? MOTOR_COUNT : param->motor_id;

    for(uint8_t i = start_idx; i < end_idx; i++) {
        /* 将速度值(-1000~1000)转换为PWM比较值 */
        uint16_t compare = (uint16_t)((abs(param->speed) * motor_configs[i].max_compare) / 1000);
        
        if(param->speed > 0) {
            /* 正转 */
            motor_set_compare(i, compare, 0);
        } else if(param->speed < 0) {
            /* 反转 */
            motor_set_compare(i, 0, compare);
        } else {
            /* 停止 */
            motor_brake(param->motor_id);
        }
    }
    return HAL_OK;
}

/**
  * @brief   电机刹车
  * @param   motor_id: 电机编号
  * @retval  HAL_StatusTypeDef: HAL状态
*/
HAL_StatusTypeDef motor_brake(Motor_Id_t motor_id) {
    if(motor_id > MOTOR_COUNT) {
        return HAL_ERROR;
    }

    uint8_t start_idx = (motor_id == MOTOR_ALL) ? 0 : (motor_id - 1);
    uint8_t end_idx = (motor_id == MOTOR_ALL) ? MOTOR_COUNT : motor_id;

    for(uint8_t i = start_idx; i < end_idx; i++) {
        motor_set_compare(i, motor_configs[i].max_compare, motor_configs[i].max_compare);
    }
    return HAL_OK;
}

/**
  * @brief   设置电机滑行模式（停止PWM输出）
  * @param   motor_id: 电机编号
  * @retval  HAL_StatusTypeDef: HAL状态
*/
HAL_StatusTypeDef motor_coast(Motor_Id_t motor_id) {
    if(motor_id > MOTOR_COUNT) {
        return HAL_ERROR;
    }

    uint8_t start_idx = (motor_id == MOTOR_ALL) ? 0 : (motor_id - 1);
    uint8_t end_idx = (motor_id == MOTOR_ALL) ? MOTOR_COUNT : motor_id;

    for(uint8_t i = start_idx; i < end_idx; i++) {
        motor_set_compare(i, 0, 0);
    }
    return HAL_OK;
}

/* 内部函数实现 */
static inline HAL_StatusTypeDef motor_validate_param(const Motor_Ctrl_Param_t* param) {
    if(param == NULL || param->motor_id > MOTOR_COUNT || 
       abs(param->speed) > 1000) {
        return HAL_ERROR;
    }
    return HAL_OK;
}

static inline void motor_set_compare(uint8_t motor_idx, uint16_t in1_compare, uint16_t in2_compare) {
    __HAL_TIM_SET_COMPARE(motor_configs[motor_idx].tim_handle, 
                         motor_configs[motor_idx].in1_channel, in1_compare);
    __HAL_TIM_SET_COMPARE(motor_configs[motor_idx].tim_handle, 
                         motor_configs[motor_idx].in2_channel, in2_compare);
}

