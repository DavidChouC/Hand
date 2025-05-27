#ifndef __pid_h
#define __pid_h
#include <stdint.h>
#include "bsp_motor.h"

typedef struct _PID_TypeDef
{
	
	float target;       /* 目标值 */
    float actual;       /* 实际值 */
    float output;       /* 输出值 */

    uint16_t max_output;        /* 输出限幅 */
    uint16_t intergral_limit;   /* 积分限幅 */
    
    float kp;           /* 比例项 */
    float ki;           /* 积分项 */
    float kd;           /* 微分项 */

    float error;        /* 本次误差 */
    float error_last;   /* 上次误差 */
    float integral;     /* 误差积分 */

    void (*f_pid_param_init)(struct _PID_TypeDef *pid,float max_output,uint16_t intergral_limit,float target,float kp,float ki,float kd);/* pid参数初始化 */
	void (*f_pid_reset)(struct _PID_TypeDef *pid, float kp,float ki, float kd);     /* pid参数修改 */
	float (*f_cal_pid)(struct _PID_TypeDef *pid, float actual);                     /* pid计算 */

}PID_TypeDef;

void Pid_Pos(Motor_Id_t motor_id,int16_t target_angle);


void Motor1_Pid_Pos(int16_t target_angle);

void Motor2_Pid_Pos(int16_t target_angle);

void Motor3_Pid_Pos(int16_t target_angle);

#endif
