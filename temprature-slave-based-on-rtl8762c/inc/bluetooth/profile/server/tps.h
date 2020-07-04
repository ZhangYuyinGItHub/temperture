/**
*****************************************************************************************
*     Copyright(c) 2017, Realtek Semiconductor Corporation. All rights reserved.
*****************************************************************************************
  * @file     tps.h
  * @brief    Head file for using TX power service.
  * @details  TPS data structs and external functions declaration.
  * @author
  * @date
  * @version  v1.0
  * *************************************************************************************
  */

#ifndef _TPS_H_
#define _TPS_H_

#ifdef  __cplusplus
extern "C" {
#endif      /* __cplusplus */

#include <stdint.h>
#include "profile_server.h"


/** @defgroup TPS Tx Power Service
  * @brief Tx power service
  * @details

    The Tx Power service uses the Tx Power Level characteristic to expose the current transmit power level
    of a device when in a connection.The Tx Power service contains only a Tx Power Level characteristic.
    The Tx Power Service generally makes up a profile with some other services, such as Proximity, and its
    role is to indicate a device's transmit power level when in a connection.

    Application shall register Tx Power service when initialization through @ref tps_add_service function.

    Application can set the TX power value through @ref tps_set_parameter function.

  * @{
  */
/*============================================================================*
 *                              Macros
 *============================================================================*/
/** @defgroup TPS_Exported_Macros TPS Exported Macros
  * @brief
  * @{
  */
/** @defgroup TPS_Read_Info TPS Read Info
  * @brief  Read characteristic value.
  * @{
  */
#define TPS_READ_TX_POWER_VALUE         1
/** @} */
/** @} End of TPS_Exported_Macros */
/*============================================================================*
 *                              Types
 *============================================================================*/
/** @defgroup TPS_Exported_Types TPS Exported Types
  * @brief
  * @{
  */
/* Add all public types here */
/** @defgroup TPS_Application_Parameters TPS Application Parameters
  * @brief  Type of parameters set/got from application.
  * @{
  */
typedef enum
{
    TPS_PARAM_TX_POWER
} T_TPS_PARAM_TYPE;
/** @} */

/** @defgroup TPS_Upstream_Message TPS Upstream Message
  * @brief TPS data struct for notification data to application.
  * @{
  */
/** Message content: @ref TPS_Upstream_Message */
typedef union
{
    uint8_t read_value_index;
} T_TPS_UPSTREAM_MSG_DATA;

/** TPS service data to inform application. */
typedef struct
{
    uint8_t                 conn_id;
    T_SERVICE_CALLBACK_TYPE msg_type;
    T_TPS_UPSTREAM_MSG_DATA msg_data;
} T_TPS_CALLBACK_DATA;
/** @} */

/** @} End of TPS_Exported_Types */
/*============================================================================*
 *                              Functions
 *============================================================================*/
/** @defgroup TPS_Exported_Functions TPS Exported Functions
  * @{
  */

/**
 * @brief       Set a Tx power service parameter.
 *
 *              NOTE: You can call this function with a tx power service parameter type and it will set the
 *                      tx power service parameter.  Tx power service parameters are defined in @ref T_TPS_PARAM_TYPE.
 *                      If the "len" field sets to the size of a "uint16_t" ,the
 *                      "p_value" field must point to a data with type of "uint16_t".
 *
 * @param[in]   param_type   Tx power service parameter type: @ref T_TPS_PARAM_TYPE
 * @param[in]   len       Length of data to write
 * @param[in]   p_value Pointer to data to write.  This is dependent on
 *                      the parameter type and WILL be cast to the appropriate
 *                      data type (example: data type of uint16 will be cast to
 *                      uint16 pointer).
 *
 * @return Operation result.
 * @retval true Operation success.
 * @retval false Operation failure.
 *
 * <b>Example usage</b>
 * \code{.c}
    void test(void)
    {
        uint8_t tx_power = 0;
        tps_set_parameter(TPS_PARAM_TX_POWER, 1, &tx_power);
    }
 * \endcode
 */
bool tps_set_parameter(T_TPS_PARAM_TYPE param_type, uint8_t len, void *p_value);


/**
  * @brief Add tx power service to the BLE stack database.
  *
  * @param[in]   p_func  Callback when service attribute was read, write or cccd update.
  * @return Service id generated by the BLE stack: @ref T_SERVER_ID.
  * @retval 0xFF Operation failure.
  * @retval others Service id assigned by stack.
  *
  * <b>Example usage</b>
  * \code{.c}
     void profile_init()
     {
         server_init(1);
         tps_id = tps_add_service(app_handle_profile_message);
     }
  * \endcode
  */
T_SERVER_ID tps_add_service(void *p_func);

/** @} End of TPS_Exported_Functions */

/** @} End of TPS */


#ifdef  __cplusplus
}
#endif      /*  __cplusplus */

#endif /* _TPS_H_ */
