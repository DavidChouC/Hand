#include "bsp_can.h"
#include "can.h"
#include "bsp_led.h"
#include <string.h>

/* 全局变量 */
Can_Rec_t can_rec[5];

static Can_Handle_t can_handle;
static CAN_TxHeaderTypeDef tx_header;
static uint8_t tx_data[8];
static uint32_t tx_mailbox;

/* 私有函数声明 */
static void can_filter_init();
static void can_error_handler(Can_Error_t error);
static HAL_StatusTypeDef can_send_message(uint32_t id, uint8_t *data, uint8_t len);
static void can_process_rx_message(CAN_RxHeaderTypeDef *rx_header, uint8_t *rx_data);


/**
  *@breif      解码电调反馈的电机数据
  *@param      none
  *@retval     none
  */
#define get_motor_measure(ptr, rx_data)                                 \
    {                                                                   \
        (ptr)->slider1_displacement = (rx_data[0] << 8) | rx_data[1];   \
        (ptr)->slider2_displacement = (rx_data[2] << 8) | rx_data[3];   \
        (ptr)->slider3_displacement = (rx_data[4] << 8) | rx_data[5];   \
        (ptr)->feedback = rx_data[6];                                   \
    }

/**
  * @brief   CAN初始化
  * @param   hcan: CAN句柄
  * @retval  none
*/
void can_init(Can_Handle_t *hcan)
{
    if (hcan == NULL) {
        return;
    }
    /* 初始化CAN句柄 */
    memset(&can_handle, 0, sizeof(Can_Handle_t));
    can_handle.error = CAN_ERROR_NONE;
    can_handle.retryCount = 0;
    can_handle.lastTxTime = 0;
    can_handle.isBusy = 0;
    
    /* 初始化CAN过滤器 */
    can_filter_init();
}


/**
  * @brief   发送电机控制角度
  * @param   motor1: 电机1角度
  * @param   motor2: 电机2角度
  * @param   motor3: 电机3角度
  * @param   motor4: 电机4角度
  * @retval  none
*/
void can_ctrl_motors(int16_t motor1, int16_t motor2, int16_t motor3, int16_t motor4)
{
    Can_Msg_t msg;
    
    /* 设置消息参数 */
    msg.id = CAN_MSG_ID_BROADCAST;
    msg.type = CAN_MSG_TYPE_CONTROL;
    msg.length = 8;
    
    /* 填充数据 */
    msg.data[0] = (uint8_t)(motor1 >> 8);
    msg.data[1] = (uint8_t)(motor1 & 0xFF);
    msg.data[2] = (uint8_t)(motor2 >> 8);
    msg.data[3] = (uint8_t)(motor2 & 0xFF);
    msg.data[4] = (uint8_t)(motor3 >> 8);
    msg.data[5] = (uint8_t)(motor3 & 0xFF);
    msg.data[6] = (uint8_t)(motor4 >> 8);
    msg.data[7] = (uint8_t)(motor4 & 0xFF);
    
    /* 发送消息 */
    can_send_message(msg.id, msg.data, msg.length);
}

/**
  * @brief   重启所有电机
  * @retval  none
*/
void can_reboot_motors(void)
{
    Can_Msg_t msg;
    
    /* 设置消息参数 */
    msg.id = CAN_MSG_ID_REBOOT;
    msg.type = CAN_MSG_TYPE_REBOOT;
    msg.length = 8;
    
    /* 填充数据 */
    memset(msg.data, 0, 8);
    
    /* 发送消息 */
    can_send_message(msg.id, msg.data, msg.length);
}

/**
  * @brief   CAN测试函数
  * @param   can_id: 测试ID
  * @retval  none
*/
void can_send_test(uint32_t can_id)
{
    Can_Msg_t msg;
    
    /* 设置消息参数 */
    msg.id = can_id;
    msg.type = CAN_MSG_TYPE_TEST;
    msg.length = 8;
    
    /* 填充数据 */
    memset(msg.data, 0xAA, 8);
    
    /* 发送消息 */
    can_send_message(msg.id, msg.data, msg.length);
}

/**
  * @brief   CAN接收回调函数
  * @param   hcan: CAN句柄
  * @retval  none
*/
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[8];
    
    /* 获取接收到的消息 */
    if(HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rx_header, rx_data) == HAL_OK) {
        SEGGER_RTT_printf(0, "666\r\n");
        can_process_rx_message(&rx_header, rx_data);
    }
}

/**
  * @brief   CAN过滤器初始化
  * @retval  none
*/
static void can_filter_init()
{
    CAN_FilterTypeDef filter;
    
    /* 配置过滤器 */
    filter.FilterActivation = ENABLE;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh = 0x0000;
    filter.FilterIdLow = 0x0000;
    filter.FilterMaskIdHigh = 0x0000;
    filter.FilterMaskIdLow = 0x0000;
    filter.FilterBank = 0;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    
    /* 应用过滤器配置 */
    if(HAL_CAN_ConfigFilter(&hcan, &filter) != HAL_OK) {
        can_error_handler(CAN_ERROR_BUS_OFF);
        return;
    }
    
    /* 启动CAN */
    if(HAL_CAN_Start(&hcan) != HAL_OK) {
        can_error_handler(CAN_ERROR_BUS_OFF);
        return;
    }
    
    /* 使能接收中断 */
    if(HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
        can_error_handler(CAN_ERROR_BUS_OFF);
        return;
    }
}

/**
  * @brief   错误处理函数
  * @param   error: 错误类型
  * @retval  none
*/
static void can_error_handler(Can_Error_t error)
{
    can_handle.error = error;
    
    switch(error) {
        case CAN_ERROR_BUS_OFF:
            /* 总线关闭，尝试恢复 */
            HAL_CAN_ResetError(&hcan);
            HAL_CAN_Start(&hcan);
            break;
            
        case CAN_ERROR_TX_FAILED:
            /* 发送失败，增加重试计数 */
            can_handle.retryCount++;
            if(can_handle.retryCount > 3) {
                can_handle.error = CAN_ERROR_BUS_OFF;
            }
            break;
            
        default:
            break;
    }
}

/**
  * @brief   发送CAN消息
  * @param   id: 消息ID
  * @param   data: 数据指针
  * @param   len: 数据长度
  * @retval  HAL_StatusTypeDef
*/
static HAL_StatusTypeDef can_send_message(uint32_t id, uint8_t *data, uint8_t len)
{
    if (data == NULL || len > 8) {
        return HAL_ERROR;
    }

    /* 检查CAN状态 */
    if(can_handle.isBusy) {
        return HAL_BUSY;
    }
    
    /* 设置发送参数 */
    tx_header.StdId = id;
    tx_header.IDE = CAN_ID_STD;
    tx_header.RTR = CAN_RTR_DATA;
    tx_header.DLC = len;
    
    /* 复制数据 */
    memcpy(tx_data, data, len);
    
    /* 发送消息 */
    can_handle.isBusy = 1;
    HAL_StatusTypeDef status = HAL_CAN_AddTxMessage(&hcan, &tx_header, tx_data, &tx_mailbox);
    can_handle.isBusy = 0;
    
    /* 检查发送状态 */
    if(status != HAL_OK) {
        can_error_handler(CAN_ERROR_TX_FAILED);
    } else {
        can_handle.lastTxTime = HAL_GetTick();
        can_handle.retryCount = 0;
    }
    
    return status;
}



/**
  * @brief   处理接收到的消息
  * @param   rx_header: 接收消息头
  * @param   rx_data: 接收数据
  * @retval  none
*/
static void can_process_rx_message(CAN_RxHeaderTypeDef *rx_header, uint8_t *rx_data)
{
    if (rx_header == NULL || rx_data == NULL) {
        return;
    }

    switch(rx_header->StdId) {
        case CAN_MSG_ID_BROADCAST:
        case CAN_MSG_ID_FINGER1:
        case CAN_MSG_ID_FINGER2:
        case CAN_MSG_ID_FINGER3:
        case CAN_MSG_ID_FINGER4:
        case CAN_MSG_ID_FINGER5:
        {
            static uint8_t i = 0;
            //获取电机ID
            i = rx_header->StdId - CAN_MSG_ID_FINGER1;
            //解析电机数据
            get_motor_measure(&can_rec[i],rx_data);
            break;
        }
        default:
            break;
    }
}
