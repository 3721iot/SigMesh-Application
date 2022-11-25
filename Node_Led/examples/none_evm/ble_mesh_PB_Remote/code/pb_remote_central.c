/**
 * Copyright (c) 2019, Freqchip
 * 
 * All rights reserved.
 * 
 * 
 */
 
 /*
 * INCLUDES (包含头文件)
 */
#include <stdio.h>
#include <string.h>

#include "os_timer.h"
#include "driver_pmu.h"

#include "ble_hl_error.h"
#include "gap_api.h"
#include "gatt_api.h"
#include "gatt_sig_uuid.h"
#include "sys_utils.h"

/*
 * MACROS (宏定义)
 */
#define PB_REMOTE_CONN_TRY_MAX_COUNT            3

/*
 * CONSTANTS (常量定义)
 */

/*
 * TYPEDEFS (???????)
 */
struct m_pbrs_prov_bearer_gatt_cb_t {
    /// used create gatt bearer callde by PB-Remote server
    void (*gatt_bearer_create)(uint8_t addr_type, mac_addr_t *addr);

    /// used close gatt bearer callde by PB-Remote server
    void (*gatt_bearer_close)(void);

    /// used by PB-Remote server to send data to provisionee
    void (*gatt_bearer_send)(uint8_t *data, uint16_t length, uint8_t out_seq);
};

/*
 * GLOBAL VARIABLES (??????)
 */

/*
 * LOCAL VARIABLES (本地变量)
 */
static const gatt_uuid_t client_att_tb[] =
{
    [0]  =
    { UUID_SIZE_2, UUID16_ARR(0x2adb)},
    [1]  =
    { UUID_SIZE_2, UUID16_ARR(0x2adc)},
};

static uint8_t client_id;
static uint8_t client_idx = 0xff;

static os_timer_t pb_remote_conn_timer;
static uint8_t pb_remote_conn_try_times = 0;

static uint8_t pb_remote_last_addr_type;
static mac_addr_t *pb_remote_last_addr;

static uint8_t pb_remote_last_out_seq = 0;

static void pb_remote_gatt_create(uint8_t addr_type, mac_addr_t *addr);
static void pb_remote_gatt_close(void);
static void pb_remote_gatt_send(uint8_t *data, uint16_t length, uint8_t out_seq);

static const struct m_pbrs_prov_bearer_gatt_cb_t cb = {
    .gatt_bearer_create = pb_remote_gatt_create,
    .gatt_bearer_close = pb_remote_gatt_close,
    .gatt_bearer_send = pb_remote_gatt_send,
};

void m_pbrs_prov_bearer_gatt_cb_regist(const struct m_pbrs_prov_bearer_gatt_cb_t *cb);
void m_pbrs_prov_bearer_gatt_opened(bool result);
void m_pbrs_prov_bearer_gatt_closed(uint8_t reason);
void m_pbrs_prov_bearer_gatt_sent(uint8_t out_seq);
void m_pbrs_prov_bearer_gatt_rx(uint16_t length, uint8_t *data);

/*
 * LOCAL FUNCTIONS (本地函数)
 */

/*********************************************************************
 * @fn      simple_central_msg_handler
 *
 * @brief   Simple Central GATT message handler, handles messages from GATT layer.
 *          Messages like read/write response, notification/indication values, etc.
 *
 * @param   p_msg       - GATT message structure.
 *
 * @return  uint16_t    - Data length of the GATT message handled.
 */
static uint16_t pb_remote_central_msg_handler(gatt_msg_t *p_msg)		//GATT event callback function, p_msg is pointer to event content
{
    switch(p_msg->msg_evt)	//event type, GATT event type is defined in gatt_msg_evt_t in "gatt_api.h"
    {
        case GATTC_MSG_NTF_REQ:
            if(p_msg->att_idx == 1) {
                m_pbrs_prov_bearer_gatt_rx(p_msg->param.msg.msg_len, p_msg->param.msg.p_msg_data);
            }
            break;
        case GATTC_MSG_CMP_EVT:	//A GATTC operation is over
        {
            if(p_msg->param.op.operation == GATT_OP_PEER_SVC_DISC_END)
            {
                co_printf("peer svc discovery done\r\n");
            }
            else if(p_msg->param.op.operation == GATT_OP_PEER_SVC_REGISTERED)	//if operation is discovery serivce group on peer device
            {
                uint16_t att_handles[2];
                memcpy(att_handles,p_msg->param.op.arg,4);	//copy discoveried handles on peer device
                show_reg((uint8_t *)att_handles,4,1);	//printf handlers for UUID array idx. if 0 means handler nnumber is not not found

                if((att_handles[0] == 0)
                    || (att_handles[1] == 0)) {
                    gap_disconnect_req(client_idx);
                }
                else {
                    gatt_client_enable_ntf_t ntf_enable;
                    ntf_enable.conidx = client_idx;
                    ntf_enable.client_id = client_id;
                    ntf_enable.att_idx = 1;
                    gatt_client_enable_ntf(ntf_enable);

                    m_pbrs_prov_bearer_gatt_opened(true);
                }
            }
            else if(p_msg->param.op.operation == GATT_OP_WRITE_CMD) {
                m_pbrs_prov_bearer_gatt_sent(pb_remote_last_out_seq);
            }
        }
        break;
        
        default:
        break;
    }

    return 0;
}

static void pb_remote_connect_timeout(void *arg)
{
    gap_stop_conn();

    m_pbrs_prov_bearer_gatt_opened(false);
}

/*
 * EXTERN FUNCTIONS (外部函数)
 */

/*
 * PUBLIC FUNCTIONS (全局函数)
 */

/** @function group ble peripheral device APIs (ble外设相关的API)
 * @{
 */

/*********************************************************************
 * @fn      pb_remote_central_init
 *
 * @brief   Initialize simple central, BLE related parameters.
 *
 * @param   None. 
 *       
 *
 * @return  None.
 */
void pb_remote_central_init(void)
{
    // Initialize GATT 
    gatt_client_t client;
    
#if 1
    client.p_att_tb = client_att_tb;
    client.att_nb = 2;
#else
    client.p_att_tb = NULL;
    client.att_nb = 0;
#endif

    client.gatt_msg_handler = pb_remote_central_msg_handler;
    client_id = gatt_add_client(&client); 

    m_pbrs_prov_bearer_gatt_cb_regist(&cb);

    os_timer_init(&pb_remote_conn_timer, pb_remote_connect_timeout, NULL);
}

void pb_remote_connected(uint8_t conidx)
{
    client_idx = conidx;
    gatt_discovery_all_peer_svc(client_id, conidx);
    os_timer_stop(&pb_remote_conn_timer);
}

void pb_remote_disconnected(uint8_t conidx, uint8_t reason)
{
    if(conidx != client_idx) {
        return;
    }

    client_idx = 0xff;
    
    if(reason == 0x3e/*CO_ERROR_CONN_FAILED_TO_BE_EST*/) {
        if(pb_remote_conn_try_times <= PB_REMOTE_CONN_TRY_MAX_COUNT) {
            pb_remote_conn_try_times++;
            gap_start_conn((mac_addr_t *)pb_remote_last_addr, pb_remote_last_addr_type, 32, 32, 0, 500);
            os_timer_start(&pb_remote_conn_timer, 1000, false);
            return;
        }
    }

    m_pbrs_prov_bearer_gatt_closed(reason);
}

static void pb_remote_gatt_create(uint8_t addr_type, mac_addr_t *addr)
{
    pb_remote_last_addr_type = addr_type;
    pb_remote_last_addr = addr;
    pb_remote_conn_try_times = 1;
    
    gap_start_conn((mac_addr_t *)addr, addr_type, 32, 32, 0, 500);
    os_timer_start(&pb_remote_conn_timer, 1000, false);
}

static void pb_remote_gatt_close(void)
{
    gap_disconnect_req(client_idx);
}

static void pb_remote_gatt_send(uint8_t *data, uint16_t length, uint8_t out_seq)
{
    gatt_client_write_t client_write;

    pb_remote_last_out_seq = out_seq;

    client_write.conidx = client_idx;
    client_write.client_id = client_id;
    client_write.att_idx = 0;
    client_write.p_data = data;
    client_write.data_len = length;

    gatt_client_write_cmd(client_write);
}

