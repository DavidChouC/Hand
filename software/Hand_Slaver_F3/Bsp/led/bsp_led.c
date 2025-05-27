#include "bsp_led.h"

/**
  * @brief   点亮LED
  * @param   none
  * @retval  none
*/
void led_on(){
    HAL_GPIO_WritePin(LED_GPIO_Port,LED_Pin,GPIO_PIN_SET);
}

/**
  * @brief   熄灭LED
  * @param   none
  * @retval  none
*/
void led_off(){
    HAL_GPIO_WritePin(LED_GPIO_Port,LED_Pin,GPIO_PIN_RESET);
}

/**
  * @brief   LED切换状态
  * @param   none
  * @retval  none
*/
void led_toggle(){
    HAL_GPIO_TogglePin(LED_GPIO_Port,LED_Pin);
}
