#include "bsp_encoder.h"
#include "tim.h"

/* 编码器配置数组（索引0对应编码器1，依此类推） */
static const Encoder_Config encoder_configs[] = {
    {&htim1, TIM_CHANNEL_1, TIM_CHANNEL_2},
    {&htim2, TIM_CHANNEL_1, TIM_CHANNEL_2},
    {&htim3, TIM_CHANNEL_1, TIM_CHANNEL_2}
};

#define ENCODER_COUNT (sizeof(encoder_configs)/sizeof(encoder_configs[0]))

/**
  * @brief   编码器初始化
  * @param   none
  * @retval  none
*/
void encoder_init(){
    for(uint8_t i = 0; i < ENCODER_COUNT; i++) {
        HAL_TIM_Encoder_Start(encoder_configs[i].tim_handle, encoder_configs[i].cc1_channel);
        HAL_TIM_Encoder_Start(encoder_configs[i].tim_handle, encoder_configs[i].cc2_channel);
        __HAL_TIM_SET_COUNTER(encoder_configs[i].tim_handle, 0);
    }
}


/**
  * @brief   获取当前角度（单位：度)
  * @param   motor_id: 电机编号
  *            @arg 1-3: 对应电机编号
  * @retval  angle:角度值
*/
float get_motor_angle(Motor_Id_t motor_id){
    /* 检查电机ID是否有效 */
    if(motor_id < 1 || motor_id > ENCODER_COUNT) {
        return 0.0f;
    }

    /* 获取编码器计数值 */
    int16_t counter = __HAL_TIM_GET_COUNTER(encoder_configs[motor_id-1].tim_handle);
    /* 将计数值转换为角度 */
    float angle = counter * ANGLE_PER_PULSE;
    return angle;
}