#ifndef BSP_CAN_H
#define BSP_CAN_H

#include "main.h"
#include <stdint.h>

typedef struct
{
    uint16_t slider1_displacement;		//滑块1距离
    uint16_t slider2_displacement;		//滑块2距离
    uint16_t slider3_displacement;		//滑块3距离
    bool feedback;
} Can_Rec_t;

/* CAN消息类型 */
typedef enum {
    CAN_MSG_TYPE_CONTROL = 0x01,    /* 控制消息类型 */
    CAN_MSG_TYPE_REBOOT = 0x02,     /* 重启消息类型 */
    CAN_MSG_TYPE_TEST = 0x03        /* 测试消息类型 */
} Can_Msg_Type_t;

/* CAN错误类型 */
typedef enum {
    CAN_ERROR_NONE = 0,      /* 无错误 */
    CAN_ERROR_BUS_OFF,       /* 总线关闭错误 */
    CAN_ERROR_TX_FAILED      /* 发送失败错误 */
} Can_Error_t;

/* CAN消息ID */
typedef enum {
    CAN_MSG_ID_BROADCAST = 0x200,   /* 广播ID */
    CAN_MSG_ID_FINGER1 = 0x201,     /* 手指1 ID */
    CAN_MSG_ID_FINGER2 = 0x202,     /* 手指2 ID */
    CAN_MSG_ID_FINGER3 = 0x203,     /* 手指3 ID */
    CAN_MSG_ID_FINGER4 = 0x204,     /* 手指4 ID */
    CAN_MSG_ID_FINGER5 = 0x205,     /* 手指5 ID */
    CAN_MSG_ID_REBOOT = 0x700       /* 重启ID */
} Can_Msg_Id_t;

/* CAN消息结构体 */
typedef struct {
    uint32_t id;            /* 消息ID */
    Can_Msg_Type_t type;    /* 消息类型 */
    uint8_t length;         /* 数据长度 */
    uint8_t data[8];        /* 数据内容 */
} Can_Msg_t;

/* CAN句柄结构体 */
typedef struct {
    Can_Error_t error;      /* 错误状态 */
    uint8_t retryCount;     /* 重试次数 */
    uint32_t lastTxTime;    /* 最后发送时间 */
    uint8_t isBusy;         /* 忙标志 */
} Can_Handle_t;


/**
  * @brief  CAN初始化
  * @param  hcan: CAN句柄指针
  * @retval none
  * @note   初始化CAN通信模块，包括句柄初始化和过滤器配置
*/
void can_init(Can_Handle_t *hcan);

/**
  * @brief  发送电机控制角度
  * @param  motor1: 电机1角度值
  * @param  motor2: 电机2角度值
  * @param  motor3: 电机3角度值
  * @param  motor4: 电机4角度值
  * @retval none
  * @note   发送控制命令到所有电机，设置它们的角度
  *         角度范围为-32768到32767
*/
void can_ctrl_motors(int16_t motor1, int16_t motor2, int16_t motor3, int16_t motor4);

/**
  * @brief  重启所有电机
  * @retval none
  * @note   发送重启命令到所有电机，使它们恢复到初始状态
*/
void can_reboot_motors(void);

/**
  * @brief  CAN测试函数
  * @param  can_id: 测试ID
  * @retval none
  * @note   发送测试消息到指定ID，用于测试CAN通信是否正常
*/
void can_send_test(uint32_t can_id);

extern Can_Rec_t can_rec[5];

#endif
