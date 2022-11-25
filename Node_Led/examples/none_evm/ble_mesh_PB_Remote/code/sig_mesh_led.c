/**
 * Copyright (c) 2019, Freqchip
 *
 * All rights reserved.
 *
 *
 */

/*
 * INCLUDES 
 */
#include <stdint.h>
#include <stdbool.h>

#include "sha256.h"
#include "co_printf.h"
#include "co_list.h"
#include "os_timer.h"
#include "os_mem.h"
#include "sys_utils.h"
#include "gap_api.h"

#include "mesh_api.h"
#include "mesh_model_msg.h"
#include "sig_mesh_info.h"

#include "driver_plf.h"
#include "driver_system.h"
#include "driver_pmu.h"
#include "driver_gpio.h"
#include "driver_flash.h"
#include "button.h"

#include "flash_usage_config.h"
//#include "demo_clock.h"
//#include "vendor_timer_ctrl.h"
/*
 * MACROS 
 */
//#define SIG_MESH_TIMER

/*
 * CONSTANTS 
 */
#define ALI_SIG_MESH_VERSION                0

#define SIG_MESH_50HZ_CHECK_IO          GPIO_PD6

uint8_t mesh_key_bdaddr[] = {0xda,0xbb,0x6b,0x07,0xda,0x71};
uint8_t mesh_key_pid[] = {0x0e, 0x01, 0x00, 0x00};
uint8_t mesh_key_secret[] = {0x92,0x37,0x41,0x48,0xb5,0x24,0xd9,0xcf,0x7c,0x24,0x04,0x36,0x0b,0xa8,0x91,0xd0};

/*
 * TYPEDEFS 
 */
typedef struct led_hsl_s
{
    uint16_t led_l;
    uint16_t led_h;
    uint16_t led_s;
}led_hsl_t;

enum app_mesh_user_adv_state_t {
    APP_MESH_USER_ADV_IDLE,
    APP_MESH_USER_ADV_SENDING,
};

struct app_mesh_user_adv_item_t {
    struct co_list_hdr hdr;
    uint8_t duration;
    uint8_t adv_len;
    uint8_t adv_data[];
};

/*
 * GLOBAL VARIABLES 
 */

/*
 * LOCAL VARIABLES 
 */
static os_timer_t sig_mesh_prov_timer;
static enum app_mesh_user_adv_state_t app_mesh_user_adv_state = APP_MESH_USER_ADV_IDLE;
static struct co_list app_mesh_user_adv_list;

//static os_timer_t sig_mesh_send_health_message;
/*
 * LOCAL FUNCTIONS 
 */
static void app_mesh_recv_gen_onoff_led_msg(mesh_model_msg_ind_t const *ind);
static void app_mesh_recv_lightness_msg(mesh_model_msg_ind_t const *ind);
static void app_mesh_recv_hsl_msg(mesh_model_msg_ind_t const *ind);
static void app_mesh_recv_CTL_msg(mesh_model_msg_ind_t const *ind);
static void app_mesh_recv_CTL_setup_msg(mesh_model_msg_ind_t const *ind);
static void app_mesh_recv_CTL_temperature_msg(mesh_model_msg_ind_t const *ind);
void app_mesh_recv_vendor_msg(mesh_model_msg_ind_t const *ind);
static void app_mesh_stop_publish_msg_resend(void);

void app_mesh_start_publish_msg_resend(uint8_t * p_msg,uint8_t p_len);
void app_mesh_50Hz_check_timer_handler(void * arg);
void app_mesh_50Hz_check_enable(void);


// Mesh model define
static mesh_model_t light_models[] = 
{
    [0] = {
        .model_id = MESH_MODEL_ID_ONOFF,
        .model_vendor = false,
        .element_idx = 0,
        .msg_handler = app_mesh_recv_gen_onoff_led_msg,
    },
    [1] = {
        .model_id = MESH_MODEL_ID_LIGHTNESS,
        .model_vendor = false,
        .element_idx = 0,
        .msg_handler = app_mesh_recv_lightness_msg,
    },
    [2] = {
        .model_id = MESH_MODEL_ID_LIGHTNESS_SETUP,
        .model_vendor = false,
        .element_idx = 0,
        .msg_handler = app_mesh_recv_lightness_msg,
    },
    [3] = {
        .model_id = MESH_MODEL_ID_LIGHTCRL,
        .model_vendor = false,
        .element_idx = 0,
        .msg_handler = app_mesh_recv_CTL_msg,
    },
    [4] = {
        .model_id = MESH_MODEL_ID_LIGHTCRL_SETUP,
        .model_vendor = false,
        .element_idx = 0,
        .msg_handler = app_mesh_recv_CTL_setup_msg,
    },
    [5] = {
        .model_id = MESH_MODEL_ID_CTL_TEMP,
        .model_vendor = false,
        .element_idx = 0,
        .msg_handler = app_mesh_recv_CTL_temperature_msg,
    },
    [6] = {
        .model_id = MESH_MODEL_ID_HSL,
        .model_vendor = false,
        .element_idx = 0,
        .msg_handler = app_mesh_recv_hsl_msg,
    },

    #if 0
    [7] = {
        .model_id = MESH_MODEL_ID_VENDOR_ALI,
        .model_vendor = true,
        .element_idx = 0,
        .msg_handler = app_mesh_recv_vendor_msg,
    },
    #endif
};

static os_timer_t app_mesh_50Hz_check_timer;
static os_timer_t publish_msg_resend_t;
static uint8_t publish_msg_buff[32] = {0},publish_msg_len = 0,resend_count = 0;

#if ALI_SIG_MESH_VERSION == 1
/* binding index, from 0 to total_model_num-1 */
static uint8_t app_key_binding_count = 0;
/* the index of app key to be binded */
static uint16_t app_key_binding_id = 0;
/* subscription index, from 0 to total_model_num-1 */
static uint8_t subscription_count = 0;
//static uint8_t subscription_element = 0;
static uint8_t publish_count = 0;

#endif  // ALI_MESH_VERSION == 1

static void sig_mesh_prov_timer_handler(void *arg)
{
    mesh_info_clear();
#ifdef MESH_USER_DATA_STORE
    app_mesh_user_data_clear();
#endif
    platform_reset_patch(0);
}

/*********************************************************************
 * @fn      app_mesh_send_dev_rst_ind
 *
 * @brief   The response opration when user delete the network.
 *
 * @param   None
 *
 * @return  None.
 */
void app_mesh_send_dev_rst_ind(void)
{
    uint16_t option = 0;

    mesh_publish_msg_t *msg = (mesh_publish_msg_t *)os_malloc(sizeof(mesh_publish_msg_t) + 4);
    msg->element_idx = 0;
    msg->model_id = MESH_MODEL_ID_VENDOR_ALI;
    msg->opcode = MESH_VENDOR_INDICATION;
    
    msg->msg_len = 4;
    msg->msg[0] = 0x01;
    option = MESH_EVENT_UPDATA_ID;
    memcpy(&(msg->msg[1]), (uint8_t *)&option, 2);
    msg->msg[3] = MESH_EVENT_DEV_RST;

    mesh_publish_msg(msg);
    app_mesh_start_publish_msg_resend(msg->msg,msg->msg_len);
    os_free(msg);
}

/*********************************************************************
 * @fn      app_auto_update_led_state
 *
 * @brief   Actively report the state of the led when user change the state.
 *
 * @param   state     - on/off state
 *
 * @return  None.
 */
void app_auto_update_led_state(uint8_t state)
{
    uint8_t on_off_state[] = {0x00, 0x00, 0x01, 0x00};
    static uint8_t on_off_tid = 0;
    mesh_publish_msg_t *msg = (mesh_publish_msg_t *)os_malloc(sizeof(mesh_publish_msg_t) + sizeof(on_off_state));
    msg->element_idx = 0;
    msg->model_id = MESH_MODEL_ID_VENDOR_ALI;
    msg->opcode = MESH_VENDOR_INDICATION;

    on_off_state[0] = on_off_tid++;
    on_off_state[3] = state;
    
    memcpy(msg->msg, on_off_state, sizeof(on_off_state));
    msg->msg_len = sizeof(on_off_state);
    
    mesh_publish_msg(msg);
    app_mesh_start_publish_msg_resend(msg->msg,msg->msg_len);
    os_free((void *)msg);
}

/*********************************************************************
 * @fn      app_mesh_publish_msg_resend
 *
 * @brief   Resend publish msg if TiMao not reply.
 *
 * @param   arg     - param of timer callback.
 *
 * @return  None.
 */
static void app_mesh_publish_msg_resend(void * arg)
{
    if(!publish_msg_len)
        return;
        
    mesh_publish_msg_t *msg = (mesh_publish_msg_t *)os_malloc(sizeof(mesh_publish_msg_t) + publish_msg_len);
    msg->element_idx = 0;
    msg->model_id = MESH_MODEL_ID_VENDOR_ALI;
    msg->opcode = MESH_VENDOR_INDICATION;

    memcpy(msg->msg, publish_msg_buff, publish_msg_len);
    msg->msg_len = publish_msg_len;
    
    mesh_publish_msg(msg);
    os_free((void *)msg);

    //co_printf("=publish_msg_resend=\r\n");
    resend_count++;
    if(resend_count > 3)
        app_mesh_stop_publish_msg_resend();
}

/*********************************************************************
 * @fn      app_mesh_start_publish_msg_resend
 *
 * @brief   Start loop timer to resend publish msg.
 *
 * @param   p_msg     - publish msg buff.
 *
 * @param   p_len     - publish msg length.
 *
 * @return  None.
 */
void app_mesh_start_publish_msg_resend(uint8_t * p_msg,uint8_t p_len)
{
    if(p_len < 32)
    {
        resend_count = 0;
        memcpy(publish_msg_buff,p_msg,p_len);
        publish_msg_len = p_len;
    
        os_timer_start(&publish_msg_resend_t,2000,true);
    }
}

/*********************************************************************
 * @fn      app_mesh_stop_publish_msg_resend
 *
 * @brief   Stop the timer that resend publish msg.
 *
 * @param   None
 *
 * @return  None.
 */
static void app_mesh_stop_publish_msg_resend(void)
{
    //co_printf("=stop_publish_msg_resend=\r\n");
    os_timer_stop(&publish_msg_resend_t);
    memset(publish_msg_buff,0,32);
    publish_msg_len = 0;
    resend_count = 0;
}

/*********************************************************************
 * @fn      app_heartbeat_send
 *
 * @brief   A example for sending heartbeat packets.
 *
 * @param   None
 *
 * @return  None.
 */
void app_heartbeat_send(void)
{
    #if 0
	uint8_t heartbeat_data[] = {0x00,0x00,0x01,0x00,0x0E,0x01,0x10,0x27,0x0F,0x01,0x35,0x00,0x64,0x01,0x03,0x00,0x14,   \
                                    0x04,0x00,0x06,0x05,0x00,0x0D,0x05,0x01,0x04,0xF0,0x27,0x00};
    #endif
	//status source Destination PeriodLog CountLog MinHops MaxHops
	uint8_t heartbeat_data[] = {0x00,0x00,0x05,0x00,0x00,0x79,0x02,0x05,0x02};
	
	static uint8_t heartbeat_id = 0;
    mesh_publish_msg_t *msg = (mesh_publish_msg_t *)os_malloc(sizeof(mesh_publish_msg_t) + sizeof(heartbeat_data));
    msg->element_idx = 0;
									
    #if 0										
    msg->model_id = MESH_MODEL_ID_VENDOR_ALI;	
    msg->opcode = MESH_VENDOR_STATUS;
	#else
	msg->model_id = 0x0001;		//Configuration Client Model Id
	msg->opcode = 0x00003C80;		//Heartbeat Subscription Status 
	#endif
									
    heartbeat_id++;
    heartbeat_data[0] = heartbeat_id;
    memcpy(msg->msg, heartbeat_data, sizeof(heartbeat_data));
    msg->msg_len = sizeof(heartbeat_data);
    
    mesh_publish_msg(msg);
    //app_mesh_start_publish_msg_resend(msg->msg,msg->msg_len);
    os_free((void *)msg);
}

/*********************************************************************
 * @fn      app_mesh_recv_gen_onoff_led_msg
 *
 * @brief   used to check new received message whether belongs to generic on-off
 *          model or not.
 *
 * @param   ind     - message received from remote node
 *
 * @return  None.
 */
static void app_mesh_recv_gen_onoff_led_msg(mesh_model_msg_ind_t const *ind)
{
    mesh_led_on_off_msg_handler(ind);
}

/*********************************************************************
 * @fn      app_mesh_recv_gen_onoff_led_msg
 *
 * @brief   used to check new received message whether belongs to lightness
 *          model or not.
 *
 * @param   ind     - message received from remote node
 *
 * @return  None.
 */
static void app_mesh_recv_lightness_msg(mesh_model_msg_ind_t const *ind)
{
    mesh_lighness_msg_handler(ind);
}

/*********************************************************************
 * @fn      app_mesh_recv_gen_onoff_led_msg
 *
 * @brief   used to check new received message whether belongs to hsl
 *          model or not.
 *
 * @param   ind     - message received from remote node
 *
 * @return  None.
 */
static void app_mesh_recv_hsl_msg(mesh_model_msg_ind_t const *ind)
{
    mesh_RGB_msg_handler(ind);   
}

/*********************************************************************
 * @fn      app_mesh_recv_gen_onoff_led_msg
 *
 * @brief   used to check new received message whether belongs to CTL
 *          model or not.
 *
 * @param   ind     - message received from remote node
 *
 * @return  None.
 */
static void app_mesh_recv_CTL_msg(mesh_model_msg_ind_t const *ind)
{
    //mesh_CTL_msg_handler(ind);  
}

static void app_mesh_recv_CTL_setup_msg(mesh_model_msg_ind_t const *ind)
{
    //mesh_CTL_msg_handler(ind);  
}

static void app_mesh_recv_CTL_temperature_msg(mesh_model_msg_ind_t const *ind)
{
    mesh_CTL_msg_handler(ind);  
}

/*********************************************************************
 * @fn      app_mesh_recv_gen_onoff_led_msg
 *
 * @brief   used to check new received message whether belongs to vendor
 *          defined model or not.
 *
 * @param   ind     - message received from remote node
 *
 * @return  None.
 */
void app_mesh_recv_vendor_msg(mesh_model_msg_ind_t const *ind)
{
    co_printf("Received vendor meg\n");
}

/*********************************************************************
 * @fn      mesh_callback_func
 *
 * @brief   mesh message deal callback function.
 *
 * @param   event   - event to be processed
 *
 * @return  None.
 */
static void mesh_callback_func(mesh_event_t * event)
{
    uint8_t tmp_data[16];
    static uint8_t mesh_in_network = 0;

    if(event->type != MESH_EVT_ADV_REPORT) {
        //co_printf("mesh_callback_func: %d.\r\n", event->type);
    }
    
    switch(event->type) {
        case MESH_EVT_READY:
            mesh_start();
            break;
        case MESH_EVT_STARTED:
            //app_led_init();
            //app_mesh_50Hz_check_enable();
            if(mesh_in_network) {
                mesh_proxy_ctrl(1);
            }
            break;
        case MESH_EVT_STOPPED:
            //system_sleep_enable();
            break;
        case MESH_EVT_PROXY_CTRL_STATUS:
            co_printf("MESH_EVT_PROXY_CTRL_STATUS: status is 0x%04x.\r\n", event->param.proxy_adv_status);
            break;
        case MESH_EVT_IN_NETWORK:
            co_printf("device already in network\r\n");
            mesh_in_network = 1;
            break;    
        case MESH_EVT_RESET:
            mesh_in_network = 0;
            co_printf("removed from network by provisoner.\r\n");
            mesh_info_clear();
#ifdef MESH_USER_DATA_STORE
            app_mesh_user_data_clear();
#endif
            platform_reset_patch(0);
            break;
        case MESH_EVT_PROV_PARAM_REQ:
            //co_printf("=mesh param req=\r\n");
            //tmp_data[0] = 0xa8;
            //tmp_data[1] = 0x01;
			tmp_data[0] = 0xdd;
            tmp_data[1] = 0xdd;
            tmp_data[2] = 0x71;
            memcpy(&tmp_data[3], mesh_key_pid, 4);
            memcpy(&tmp_data[7], mesh_key_bdaddr, 6);
#if ALI_SIG_MESH_VERSION == 1
            tmp_data[13] = 0x02;
#else   // SIG_MESH_VERSION == 1
            tmp_data[13] = 0x00;
#endif  // SIG_MESH_VERSION == 1
            tmp_data[14] = 0x00;
            tmp_data[15] = 0x00;
            mesh_send_prov_param_rsp((uint8_t *)tmp_data, 0xd97478b3, 0, 0, 0, 0, 0, 0, 0, 1, 0);
            break;
        case MESH_EVT_PROV_AUTH_DATA_REQ:
            sha256_gen_auth_value((BYTE *)mesh_key_pid, (BYTE *)mesh_key_bdaddr, (BYTE *)mesh_key_secret, tmp_data);
            mesh_send_prov_auth_data_rsp(true, 16, (uint8_t *)tmp_data);
            break;
        case MESH_EVT_PROV_RESULT:
            co_printf("result=%d\r\n",event->param.prov_result.state);
            if(event->param.prov_result.state == 1) {
                os_timer_start(&sig_mesh_prov_timer, 30000, false);
            }
            break;
        case MESH_EVT_UPDATE_IND:
            //co_printf("=upd_type=%d\r\n",event->param.update_ind->upd_type);
            if(event->param.update_ind->upd_type == MESH_UPD_TYPE_APP_KEY_UPDATED)
            {
                uint16_t length = event->param.update_ind->length;
                uint8_t *data = event->param.update_ind->data;
                co_printf("APP_KEY: ");
                for(uint16_t i=2; i<(2+16); i++) {
                    co_printf("%02x", data[length-i-1]);
                }
                co_printf("\r\n");
            }
            else if(event->param.update_ind->upd_type == MESH_UPD_TYPE_NET_KEY_UPDATED)
            {
                uint16_t length = event->param.update_ind->length;
                uint8_t *data = event->param.update_ind->data;
                co_printf("NETWORK_KEY: ");
                for(uint16_t i=0; i<(0+16); i++) {
                    co_printf("%02x", data[length-i-1]);
                }
                co_printf("\r\n");
            }
            else if(event->param.update_ind->upd_type == MESH_UPD_TYPE_STATE)
            {
                uint16_t length = event->param.update_ind->length;
                uint8_t *data = event->param.update_ind->data;
                if(data[1] == 7/*M_TB_STORE_TYPE_DEV_KEY*/) {
                    co_printf("DEV_KEY: ");
                    for(uint16_t i=2; i<(2+16); i++) {
                        co_printf("%02x", data[length-i-1]);
                    }
                    co_printf("\r\n");
                }
            }
            else if(event->param.update_ind->upd_type == MESH_UPD_TYPE_PUBLI_PARAM)
            {
                uint16_t length = event->param.update_ind->length;
                uint8_t *data = event->param.update_ind->data;
                co_printf("PUBLISH PARAM: ");
                for(uint16_t i=0; i<length; i++) {
                    co_printf("%02x", data[length-i-1]);
                }
                co_printf("\r\n");
            }
            else if(event->param.update_ind->upd_type == MESH_UPD_TYPE_SUBS_LIST)
            {
                os_timer_stop(&sig_mesh_prov_timer);
                uint16_t length = event->param.update_ind->length;
                uint8_t *data = event->param.update_ind->data;
                co_printf("SUBSCRIPT LIST: ");
                for(uint16_t i=0; i<length; i++) {
                    co_printf("%02x", data[length-i-1]);
                }
                co_printf("\r\n");
            }
            else if(event->param.update_ind->upd_type == MESH_UPD_TYPE_BINDING)
            {
                uint16_t length = event->param.update_ind->length;
                uint8_t *data = event->param.update_ind->data;
                os_timer_stop(&sig_mesh_prov_timer);
                co_printf("BINDING: ");
                for(uint16_t i=0; i<length; i++) {
                    co_printf("%02x", data[length-i-1]);
                }
                co_printf("\r\n");
            }
#if ALI_SIG_MESH_VERSION == 1
            if(event->param.update_ind->upd_type == MESH_UPD_TYPE_APP_KEY_UPDATED) // message type is app key update
            {
                mesh_appkey_t *appkey = (mesh_appkey_t *)event->param.update_ind->data;
                app_key_binding_id = appkey->appkey_id;
                mesh_model_bind_appkey(light_models[app_key_binding_count].model_id,
                                      light_models[app_key_binding_count].element_idx,
                                      app_key_binding_id);
                app_key_binding_count++;
            }
#endif  // SIG_MESH_VERSION == 1
            app_mesh_store_info_timer_start(2000);
            break;
        case MESH_EVT_RECV_MSG:
            {
                const mesh_model_msg_ind_t *ind = &(event->param.model_msg);
                co_printf("opcode: 0x%06x\r\n", ind->opcode);
                co_printf("msg: ");
                for(uint8_t i=0; i<ind->msg_len; i++)
                {
                    co_printf("%02x ", ind->msg[i]);
                }
                co_printf("\r\n");
				co_printf("model id:[%d]\n", ind->model_id);
                for(uint8_t i=0; i < sizeof(light_models)/sizeof(light_models[0]); i++)
                {
                    /* 
                     * lower layer of mesh will loop all models for group address (AKA message destination
                     * address) match checking, and only the primary model inside one element subscript
                     * these group address. The result is that the model_id field inside this message
                     * will only match the primary model, even the opcode is used by the other models.
                     * So only element field is checked here, and the code inside msg_handler takes
                     * responsibility to check which model has to deal with this mesasge.
                     */
					if((ind->element == light_models[i].element_idx) && (ind->model_id == light_models[i].model_id))
                    {
						light_models[i].msg_handler(ind);
                    }
                }
            }
            break;
        case MESH_EVT_ADV_REPORT:
            {
                #if 0
                gap_evt_adv_report_t *report = &(event->param.adv_report);
                co_printf("recv adv from: %02x-%02x-%02x-%02x-%02x-%02x\r\n", report->src_addr.addr.addr[5],
                                                                                report->src_addr.addr.addr[4],
                                                                                report->src_addr.addr.addr[3],
                                                                                report->src_addr.addr.addr[2],
                                                                                report->src_addr.addr.addr[1],
                                                                                report->src_addr.addr.addr[0]);
                for(uint16_t i=0; i<report->length; i++) {
                    co_printf("%02x ", report->data[i]);
                }
                co_printf("\r\n");
                #endif
            }
            break;
        case MESH_EVT_PROXY_END_IND:
            co_printf("MESH_EVT_PROXY_END_IND\r\n");
//            show_ke_malloc();
//            show_mem_list();
            mesh_proxy_ctrl(1);
            break;
        case MESH_EVT_FATAL_ERROR:
            co_printf("MESH_EVT_FATAL_ERROR\r\n");
            platform_reset_patch(0);
            break;
        default:
            break;
    }
}

static void sig_mesh_send_health_sta_timer_handler(void *arg)		//add by my 
{
	app_heartbeat_send();
	printf("@---->Sned Heartbeat\n");
}

/*********************************************************************
 * @fn      app_mesh_led_init
 *
 * @brief   init mesh model, set callback function, set feature supported by
 *          this application, add models, etc.
 *
 * @param   None.
 *
 * @return  None.
 */
void app_mesh_led_init(void)
{
    mesh_composition_cfg_t dev_cfg = {
        .features = MESH_FEATURE_RELAY|MESH_FEATURE_PROXY|MESH_FEATURE_PB_GATT,
        .cid = 0x08b4, // freqchip
        .vid = 0x000f,
        .pid = 0x0006,
    };
    /* used for debug, reset Node in unprovisioned state at first */
    flash_erase(MESH_INFO_STORE_ADDR, 0x1000);			//after poweron clean stored node infor 
    co_list_init(&app_mesh_user_adv_list);

#if 1
    mac_addr_t set_addr;
    gap_address_get(&set_addr);
    memcpy(mesh_key_bdaddr,set_addr.addr,6);
    mesh_key_bdaddr[0] += 1; // to distinguish the mesh address and general gatt address
#endif

    /* set mesh message callback function */
    mesh_set_cb_func(mesh_callback_func);
    /* set scan parameters */
    mesh_set_scan_parameter(32, 32);
    /* set scan response in proxy advertising */
    uint8_t rsp_data[] = {0x05, 0x09, 0x30, 0x30, 0x30, 0x30};
    mesh_set_scan_rsp_data(6, rsp_data);
    /* register several models */
    for(uint8_t i=0; i<sizeof(light_models)/sizeof(light_models[0]); i++) {
        mesh_add_model(&light_models[i]);
    }
    /* user to start mesh stack initialization */
    mesh_init(dev_cfg, (void *)mesh_key_bdaddr, MESH_INFO_STORE_ADDR);
    mesh_dev_led_ctrl_init();    
    app_mesh_store_info_timer_init();
    os_timer_init(&app_mesh_50Hz_check_timer, app_mesh_50Hz_check_timer_handler, NULL);
    os_timer_init(&publish_msg_resend_t,app_mesh_publish_msg_resend,NULL);
#ifdef SIG_MESH_TIMER
    sys_timer_init();
#endif

#if ALI_SIG_MESH_VERSION == 1
    app_key_binding_count = 0;
    app_key_binding_id = 0;
    subscription_count = 0;
    publish_count = 0;
#endif  // SIG_MESH_VERSION == 1

    os_timer_init(&sig_mesh_prov_timer, sig_mesh_prov_timer_handler, NULL);

	#if 0
	os_timer_init(&sig_mesh_send_health_message, sig_mesh_send_health_sta_timer_handler, NULL);
	os_timer_start(&sig_mesh_send_health_message, 5000, true);
	#endif
}

/*********************************************************************
 * @fn      app_mesh_50Hz_check_enable
 *
 * @brief   enable 50Hz check, enable extern interrupt, start timer.
 *
 * @param   arg     - useless
 *
 * @return  None.
 */
void app_mesh_50Hz_check_enable(void)
{
    /* use PA0 for 50Hz detection */
    pmu_port_wakeup_func_set(SIG_MESH_50HZ_CHECK_IO);
    NVIC_EnableIRQ(PMU_IRQn);

    /* start timer */
    os_timer_start(&app_mesh_50Hz_check_timer, 100, false);
}

/*********************************************************************
 * @fn      ali_mesh_50Hz_check_timer_handler
 *
 * @brief   trigger of this timer means AC power is removed.
 *
 * @param   arg     - useless
 *
 * @return  None.
 */
void app_mesh_50Hz_check_timer_handler(void * arg)
{
    mesh_stop();
}

/*********************************************************************
 * @fn      pmu_gpio_isr_ram
 *
 * @brief   interrupt handler of 50Hz voltage level changing.
 *
 * @param   arg     - useless
 *
 * @return  None.
 */
__attribute__((section("ram_code"))) void pmu_gpio_isr_ram(void)
{
//    static uint32_t last_value = 0;
    uint32_t curr_value;
    
    /* get current gpio value and initial gpio last value */
    curr_value = ool_read32(PMU_REG_GPIOA_V);
    ool_write32(PMU_REG_PORTA_LAST, curr_value);

    #if 1
    button_toggle_detected(curr_value);
    #else
    if((curr_value ^ last_value) & SIG_MESH_50HZ_CHECK_IO) {
        /* restart timer */
        os_timer_start(&app_mesh_50Hz_check_timer, 100, false);
    }
    last_value = curr_value;
    #endif
}

static void app_mesh_send_user_adv_packet_entity(uint8_t duration, uint8_t *adv_data, uint8_t adv_len)
{
    gap_adv_param_t adv_param;

    adv_param.adv_mode = GAP_ADV_MODE_NON_CONN_NON_SCAN;
    adv_param.adv_addr_type = GAP_ADDR_TYPE_PRIVATE;
    adv_param.adv_intv_min = 32;
    adv_param.adv_intv_max = 32;
    adv_param.adv_chnl_map = GAP_ADV_CHAN_ALL;
    adv_param.adv_filt_policy = GAP_ADV_ALLOW_SCAN_ANY_CON_ANY;
    gap_set_advertising_param1(&adv_param);
    gap_set_advertising_data1(adv_data, adv_len);
    gap_start_advertising1(duration);
}

void app_mesh_send_user_adv_end(void)
{
    if(co_list_is_empty(&app_mesh_user_adv_list)) {
        app_mesh_user_adv_state = APP_MESH_USER_ADV_IDLE;
    }
    else {
        struct app_mesh_user_adv_item_t *item = (struct app_mesh_user_adv_item_t *)co_list_pop_front(&app_mesh_user_adv_list);
        if(item) {
            app_mesh_user_adv_state = APP_MESH_USER_ADV_SENDING;
            app_mesh_send_user_adv_packet_entity(item->duration, &item->adv_data[0], item->adv_len);
            os_free((void *)item);
        }
    }
}

/*********************************************************************
 * @fn      ali_mesh_send_user_adv_packet
 *
 * @brief   this is an example to show how to send data defined by user, 
 *          this function should not be recall until event GAP_EVT_ADV_END 
 *          is received.
 *
 * @param   duration    - how many 10ms advertisng will last.
 *          adv_data    - advertising data pointer
 *          adv_len     - advertising data length
 *
 * @return  None.
 */
void app_mesh_send_user_adv_packet(uint8_t duration, uint8_t *adv_data, uint8_t adv_len)
{
    if(app_mesh_user_adv_state == APP_MESH_USER_ADV_IDLE) {
        app_mesh_user_adv_state = APP_MESH_USER_ADV_SENDING;
        app_mesh_send_user_adv_packet_entity(duration, adv_data, adv_len);
    }
    else {
        struct app_mesh_user_adv_item_t *item = (struct app_mesh_user_adv_item_t *)os_malloc(sizeof(struct app_mesh_user_adv_item_t) + adv_len);
        item->duration = duration;
        item->adv_len = adv_len;
        memcpy(&item->adv_data[0], adv_data, adv_len);
        co_list_push_back(&app_mesh_user_adv_list, &item->hdr);
    }
}

