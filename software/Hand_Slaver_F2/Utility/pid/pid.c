#include "pid.h"
#include "bsp_motor.h"
#include "bsp_encoder.h"


/**
  *@breif      参数初始化
  *@param      none
  *@retval     none
  */
static void pid_param_init(PID_TypeDef *pid,float max_output,uint16_t intergral_limit,float target,float kp,float ki,float kd)
{
	pid->target = target;
	
    pid->max_output = max_output;
    pid->intergral_limit = intergral_limit;

	pid->kp = kp;
	pid->ki = ki;
	pid->kd = kd;
	
    pid->error = 0;
    pid->error_last = 0;
    pid->integral = 0;
	pid->output = 0;
}

/**
  *@breif      中途更改参数设定
  *@param      none
  *@retval     none
  */
 static void pid_reset(PID_TypeDef * pid, float kp, float ki, float kd)
 {
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
 }
 
 /**
  *@breif      pid计算
  *@param      pid:pid结构体
  *@param      actual:实际值
  *@retval     pid->output
  */
static float pid_calculate(PID_TypeDef* pid, float actual)
{   
    pid->actual = actual;
    /* 计算当前误差 */
    pid->error = pid->target - pid->actual;
    /* 累加误差 */
    pid->integral += pid->error;
    /* 积分限幅 */
    if(pid->integral > pid->intergral_limit) pid->integral = pid->intergral_limit;
    if(pid->integral < -(pid->intergral_limit)) pid->integral = -pid->intergral_limit;
    /* 计算输出 */
    pid->output = pid->kp*pid->error + pid->ki*pid->integral + pid->kd*(pid->error-pid->error_last);
    /* 误差传递 */
    pid->error_last = pid->error;
    /* 输出限幅 */
    if(pid->output > pid->max_output) pid->output = pid->max_output;
    if(pid->output < -(pid->max_output)) pid->output = -(pid->max_output);
    /* 返回输出 */
    return pid->output;
}

/**
  *@breif      pid结构体初始化，每一个pid参数需要调用一次
  *@param      pid:pid结构体
  *@retval     none
  */
 void pid_init(PID_TypeDef* pid)
 {
    pid->f_pid_param_init = pid_param_init;
    pid->f_pid_reset = pid_reset;
    pid->f_cal_pid = pid_calculate;
 }

PID_TypeDef pid;

void Pid_Pos(Motor_Id_t motor_id,int16_t target_angle)
{

    pid_init(&pid);
    pid.f_pid_param_init(&pid,1000,0,target_angle,0.6,0,0);

    float angle = get_motor_angle(motor_id);
    pid.f_cal_pid(&pid,angle);
    Motor_Ctrl_Param_t motor = {
        .motor_id = motor_id,
        .speed = pid.output
    };
    motor_set_speed(&motor);

}

void Motor1_Pid_Pos(int16_t target_angle)
{

    pid_init(&pid);
    pid.f_pid_param_init(&pid,1000,0,target_angle,1.075,0.1,0);

    float angle = get_motor_angle(MOTOR_1);
    pid.f_cal_pid(&pid,angle);
    Motor_Ctrl_Param_t motor = {
        .motor_id = MOTOR_1,
        .speed = pid.output
    };
    motor_set_speed(&motor);

}

void Motor2_Pid_Pos(int16_t target_angle)
{

    pid_init(&pid);
    pid.f_pid_param_init(&pid,1000,0,target_angle,1.075,0.1,0);

    float angle = get_motor_angle(MOTOR_2);
    pid.f_cal_pid(&pid,angle);
    Motor_Ctrl_Param_t motor = {
        .motor_id = MOTOR_2,
        .speed = pid.output
    };
    motor_set_speed(&motor);

}

void Motor3_Pid_Pos(int16_t target_angle)
{

    pid_init(&pid);
    pid.f_pid_param_init(&pid,1000,0,target_angle,1.075,0.1,0);

    float angle = get_motor_angle(MOTOR_3);
    pid.f_cal_pid(&pid,angle);
    Motor_Ctrl_Param_t motor = {
        .motor_id = MOTOR_3,
        .speed = pid.output
    };
    motor_set_speed(&motor);

}