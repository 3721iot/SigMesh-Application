/**
 * Copyright (c) 2019, Freqchip
 *
 * All rights reserved.
 *
 *
 */
#ifndef _SIG_MESH_INFO_H_
#define _SIG_MESH_INFO_H_

/*
 * INCLUDES 
 */
#include <stdbool.h>

/*
 * MACROS 
 */

/*
 * CONSTANTS 
 */
//#define MESH_QUICK_SWITCH_CTRL 
//#define MESH_USER_DATA_STORE
/* flash space used to store mesh network information, such as app key, network key, etc. */
#define MESH_INFO_STORE_ADDR                0x62000
/* flash space used to store user data information, such as on-off state, lightness, etc. */
#ifdef MESH_USER_DATA_STORE
#define APP_MESH_USER_DATA_STORED_OFFSET    0x63000
#endif
#ifdef MESH_QUICK_SWITCH_CTRL
/* flash space used to store quick switch time,to exit the network */
#define MESH_STORE_SWITCH_TIME              0x64000
#endif


/*********************************************************************
 * @fn      app_mesh_store_info_timer_start
 *
 * @brief   once MESH_EVT_UPDATE_IND message is receivec, this function
 *          should be called to start delay storage timer.
 *
 * @param   duration    - delay storage timer duration
 *
 * @return  None.
 */
void app_mesh_store_info_timer_start(uint16_t duration);

/*********************************************************************
 * @fn      app_mesh_store_info_timer_init
 *
 * @brief   initialize delay storage timer.
 *
 * @param   None
 *
 * @return  None.
 */
void app_mesh_store_info_timer_init(void);

/*********************************************************************
 * @fn      app_mesh_user_data_load
 *
 * @brief   get user data from flash, such as on_off state, lightness, etc.
 *
 * @param   msg     - user data pointer
 *          msg_len - user data length
 *
 * @return  None.
 */
bool app_mesh_user_data_load(uint8_t *msg, uint8_t msg_len);

/*********************************************************************
 * @fn      app_mesh_user_data_update
 *
 * @brief   update user data to flash, such as on_off state, lightness, etc.
 *
 * @param   msg     - user data pointer
 *          msg_len - user data length
 *          delay   - use delay to avoid execute flash operation too frequently.
 *
 * @return  None.
 */
void app_mesh_user_data_update(uint8_t *msg, uint8_t msg_len, uint32_t delay);

/*********************************************************************
 * @fn      app_mesh_user_data_clear
 *
 * @brief   clear user data storage space in flash.
 *
 * @param   None.
 *
 * @return  None.
 */
void app_mesh_user_data_clear(void);

/*********************************************************************
 * @fn      app_mesh_store_user_data_timer_init
 *
 * @brief   initialize delay storage timer.
 *
 * @param   None
 *
 * @return  None.
 */
void app_mesh_store_user_data_timer_init(void);

#ifdef MESH_QUICK_SWITCH_CTRL
/*********************************************************************
 * @fn      app_mesh_store_switch_time
 *
 * @brief   store quick switch time,to exit the network.
 *
 * @param   None
 *
 * @return  None.
 */
void app_mesh_store_switch_time(void);

/*********************************************************************
 * @fn      app_mesh_clear_switch_time
 *
 * @brief   clear quick switch time.
 *
 * @param   None
 *
 * @return  None.
 */
void app_mesh_clear_switch_time(void);
#endif

#endif  // _SIG_MESH_INFO_H_

