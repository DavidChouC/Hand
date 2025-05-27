#include "timer.h"
#include "tim.h"
#include "bsp_encoder.h"
#include "bsp_motor.h"
#include "bsp_can.h"
#include "pid.h"
#include "bsp_led.h"
/**
  * @brief  定时器初始化
*/
void timer_init()
{
    /* 启动定时器中断 */
    HAL_TIM_Base_Start_IT(&htim6);
}

/**
  * @brief  定时器中断回调函数
  * @param  htim: TIM句柄
  * @retval none
*/
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{   
    /* PID控制频率100HZ,10ms定时 */
    /* 电机旋转180°对于1mm */
    if (htim->Instance == TIM6) {
        
        Motor1_Pid_Pos(-180*can_rec[3].slider1_displacement);
        Motor2_Pid_Pos(-180*can_rec[3].slider2_displacement);
        Motor3_Pid_Pos(-180*can_rec[3].slider3_displacement);
    }
} 
