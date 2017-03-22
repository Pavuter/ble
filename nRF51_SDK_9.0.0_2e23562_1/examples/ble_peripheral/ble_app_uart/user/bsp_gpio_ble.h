#ifndef __BSP_GPIO_BLE_H
#define __BSP_GPIO_BLE_H


extern struct ble_switch_t  m_ble_switch_status;

void bsp_button_led_init(void);
void switch_control_task( void );

#endif



