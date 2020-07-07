/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
   * @file      main.c
   * @brief     Source file for BLE central project, mainly used for initialize modules
   * @author    jane
   * @date      2017-06-12
   * @version   v1.0
   **************************************************************************************
   * @attention
   * <h2><center>&copy; COPYRIGHT 2017 Realtek Semiconductor Corporation</center></h2>
   **************************************************************************************
  */

/*============================================================================*
 *                              Header Files
 *============================================================================*/
#include <stdlib.h>
#include <os_sched.h>
#include <string.h>
#include <app_task.h>
#include <trace.h>
#include <gap.h>
#include <gap_bond_le.h>
#include <gap_scan.h>
#include <gap_config.h>
#include <profile_client.h>
#include <gap_msg.h>
#include <central_app.h>
#include <voice_client.h>
#include <gaps_client.h>
#include <bas_client.h>
#include <link_mgr.h>
#include <otp_config.h>
#include "board.h"
#if FEATURE_SUPPORT_AUDIO_DOWN_STREAMING
#include <audio_handle.h>
#endif

#include "audio_hd_detection.h"

#if KEYSCAN_EN
#include "keyscan_driver.h"
#include "rtl876x_keyscan.h"
#endif

#if RCU_VOICE_EN
#include "voice_driver.h"
#endif

#if LED_EN
#include "led_driver.h"
#endif

//#if VS1053B_EN
//#include "vs1053b.h"
//#include "data_uart_test.h"
//#endif

/** @defgroup  CENTRAL_DEMO_MAIN Central Main
    * @brief Main file to initialize hardware and BT stack and start task scheduling
    * @{
    */

/*============================================================================*
 *                              Constants
 *============================================================================*/
/** @brief Default scan interval (units of 0.625ms, 0x10=2.5ms) */
#define DEFAULT_SCAN_INTERVAL     0x10
/** @brief Default scan window (units of 0.625ms, 0x10=2.5ms) */
#define DEFAULT_SCAN_WINDOW       0x10


/*============================================================================*
 *                              Functions
 *============================================================================*/
/**
 * @brief  Config bt stack related feature
 * @return void
 */
#ifdef BT_STACK_CONFIG_ENABLE
#include "app_section.h"

APP_FLASH_TEXT_SECTION void bt_stack_config_init(void)
{
    gap_config_max_le_paired_device(APP_MAX_LINKS);
}
#endif

/**
  * @brief  Initialize central and gap bond manager related parameters
  * @return void
  */
void app_le_gap_init(void)
{
    /* Device name and device appearance */
    uint8_t  device_name[GAP_DEVICE_NAME_LEN] = "BLE_CENTRAL";
    uint16_t appearance = GAP_GATT_APPEARANCE_UNKNOWN;

    /* Scan parameters */
    uint8_t  scan_mode = GAP_SCAN_MODE_ACTIVE;
    uint16_t scan_interval = DEFAULT_SCAN_INTERVAL;
    uint16_t scan_window = DEFAULT_SCAN_WINDOW;
    uint8_t  scan_filter_policy = GAP_SCAN_FILTER_ANY;
    uint8_t  scan_filter_duplicate = GAP_SCAN_FILTER_DUPLICATE_ENABLE;

    /* GAP Bond Manager parameters */
    uint8_t  auth_pair_mode = GAP_PAIRING_MODE_PAIRABLE;
    uint16_t auth_flags = GAP_AUTHEN_BIT_BONDING_FLAG;
    uint8_t  auth_io_cap = GAP_IO_CAP_NO_INPUT_NO_OUTPUT;
    uint8_t  auth_oob = false;
    uint8_t  auth_use_fix_passkey = false;
    uint32_t auth_fix_passkey = 0;
    uint8_t  auth_sec_req_enable = false;
    uint16_t auth_sec_req_flags = GAP_AUTHEN_BIT_BONDING_FLAG;

    /* Set device name and device appearance */
    le_set_gap_param(GAP_PARAM_DEVICE_NAME, GAP_DEVICE_NAME_LEN, device_name);
    le_set_gap_param(GAP_PARAM_APPEARANCE, sizeof(appearance), &appearance);

    /* Set scan parameters */
    le_scan_set_param(GAP_PARAM_SCAN_MODE, sizeof(scan_mode), &scan_mode);
    le_scan_set_param(GAP_PARAM_SCAN_INTERVAL, sizeof(scan_interval), &scan_interval);
    le_scan_set_param(GAP_PARAM_SCAN_WINDOW, sizeof(scan_window), &scan_window);
    le_scan_set_param(GAP_PARAM_SCAN_FILTER_POLICY, sizeof(scan_filter_policy),
                      &scan_filter_policy);
    le_scan_set_param(GAP_PARAM_SCAN_FILTER_DUPLICATES, sizeof(scan_filter_duplicate),
                      &scan_filter_duplicate);

    /* Setup the GAP Bond Manager */
    gap_set_param(GAP_PARAM_BOND_PAIRING_MODE, sizeof(auth_pair_mode), &auth_pair_mode);
    gap_set_param(GAP_PARAM_BOND_AUTHEN_REQUIREMENTS_FLAGS, sizeof(auth_flags), &auth_flags);
    gap_set_param(GAP_PARAM_BOND_IO_CAPABILITIES, sizeof(auth_io_cap), &auth_io_cap);
    gap_set_param(GAP_PARAM_BOND_OOB_ENABLED, sizeof(auth_oob), &auth_oob);
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY, sizeof(auth_fix_passkey), &auth_fix_passkey);
    le_bond_set_param(GAP_PARAM_BOND_FIXED_PASSKEY_ENABLE, sizeof(auth_use_fix_passkey),
                      &auth_use_fix_passkey);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_ENABLE, sizeof(auth_sec_req_enable), &auth_sec_req_enable);
    le_bond_set_param(GAP_PARAM_BOND_SEC_REQ_REQUIREMENT, sizeof(auth_sec_req_flags),
                      &auth_sec_req_flags);

    /* register gap message callback */
    le_register_app_cb(app_gap_callback);
}

/**
 * @brief  Add GATT clients and register callbacks
 * @return void
 */
void app_le_profile_init(void)
{
    client_init(3);
    gaps_client_id  = gaps_add_client(app_client_callback, APP_MAX_LINKS);
    voice_client_id = voice_add_client(app_client_callback, APP_MAX_LINKS);
    bas_client_id = bas_add_client(app_client_callback, APP_MAX_LINKS);
}

/**
 * @brief    Contains the initialization of pinmux settings and pad settings
 * @note     All the pinmux settings and pad settings shall be initiated in this function,
 *           but if legacy driver is used, the initialization of pinmux setting and pad setting
 *           should be peformed with the IO initializing.
 * @return   void
 */
void board_init(void)
{

}

/**
 * @brief    Contains the initialization of peripherals
 * @note     Both new architecture driver and legacy driver initialization method can be used
 * @return   void
 */
extern void data_uart_send(uint8_t *pbuf, uint16_t length);
void driver_init(void)
{
#if FEATURE_SUPPORT_AUDIO_DOWN_STREAMING
    audio_handle_disable_power();
#endif

#if FEATURE_SUPPORT_AUDIO_DOWN_STREAMING
    audio_handle_init_timer();
#endif

#if AUDIO_SUPPORT_HEADPHONE_DETECT
    audio_hd_driver_init();
#endif

    //audio_handle_enable_power();

#if KEYSCAN_EN
    keyscan_init_data();
    keyscan_init_pad_config();
    keyscan_pinmux_config();
    keyscan_init_driver(KeyScan_Debounce_Enable);
    keyscan_nvic_config();
    keyscan_init_timer();
#endif

    app_global_data.mtu_size = 23;

#if RCU_VOICE_EN
    //voice_driver_init_data();
#endif

#if LED_EN
    led_init_timer();
#endif

//#if VS1053B_EN
//    /* Initialize Data UART peripheral */
//    DataUARTInit(CHANGE_BAUDRATE_OPTION_2M);
//    vs1053b_gpio_init();
//    vs1053b_driver_spi_init();

//    VS_Sine_Test();
//    vs1053b_recoder_start(4);
//
//
//#endif
}

/**
 * @brief    Contains the power mode settings
 * @return   void
 */
void pwr_mgr_init(void)
{
}

/**
 * @brief    Contains the initialization of all tasks
 * @note     There is only one task in BLE Central APP, thus only one APP task is init here
 * @return   void
 */
void task_init(void)
{
    app_task_init();
}

/**
 * @brief    Entry of APP code
 * @return   int (To avoid compile warning)
 */
int main(void)
{
    extern uint32_t random_seed_value;
    srand(random_seed_value);
    board_init();
    le_gap_init(APP_MAX_LINKS);
    gap_lib_init();
    app_le_gap_init();
    app_le_profile_init();
    pwr_mgr_init();
    task_init();
    os_sched_start();

    return 0;
}
/** @} */ /* End of group CENTRAL_DEMO_MAIN */


