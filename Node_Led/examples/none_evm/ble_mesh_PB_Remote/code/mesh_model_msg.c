#include <stdint.h>

#include "co_printf.h"
#include "driver_pmu.h"
#include "mesh_api.h"
#include "os_mem.h"
#include "sys_utils.h"

#include "mesh_model_msg.h"

#include "led_fun.h"

#define MESH_MODEL_MSG_LOG  1
#if MESH_MODEL_MSG_LOG
#define MESH_MODEL_LOG      co_printf
#else
#define MESH_MODEL_LOG
#endif

struct mesh_led_state_t
{
    uint8_t on_off_state;
    uint16_t lightness;
    uint16_t hsl_lightness;
    uint16_t hsl_hue;
    uint16_t hsl_saturation;
    uint16_t ctl_lightness;
    uint16_t ctl_temperature;
    uint16_t delta_UV;
};
struct mesh_led_state_t mesh_led_state;

/*********************************************************************
 * @fn      app_mesh_status_send_rsp
 *
 * @brief   used to send response to remote after receiving acknowledged-message.
 *
 * @param   ind     - message received from remote node
 *          opcode  - opcode field should be set in response message
 *          msg     - response message pointer
 *          msg_len - response message length
 *
 * @return  None.
 */
static void app_mesh_status_send_rsp(mesh_model_msg_ind_t const *ind, uint32_t opcode, uint8_t *msg, uint16_t msg_len)
{
    mesh_rsp_msg_t * p_rsp_msg = (mesh_rsp_msg_t *)os_malloc((sizeof(mesh_rsp_msg_t)+msg_len));
    p_rsp_msg->element_idx = ind->element;
    p_rsp_msg->app_key_lid = ind->app_key_lid;
    p_rsp_msg->model_id = ind->model_id;
    p_rsp_msg->opcode = opcode;
    p_rsp_msg->dst_addr = ind->src;
    p_rsp_msg->msg_len = msg_len;
    memcpy(p_rsp_msg->msg, msg, msg_len);
    
    mesh_send_rsp(p_rsp_msg);
    os_free(p_rsp_msg);
}
#if 0
static void mesh_lighness_status_send_rsp(mesh_model_msg_ind_t const *ind, uint32_t opcode, uint8_t *msg, uint16_t msg_len)
{
    mesh_rsp_msg_t * p_rsp_msg = (mesh_rsp_msg_t *)os_malloc((sizeof(mesh_rsp_msg_t)+msg_len));
    p_rsp_msg->element_idx = ind->element;
    p_rsp_msg->app_key_lid = ind->app_key_lid;
    p_rsp_msg->model_id = ind->model_id;
    p_rsp_msg->opcode = opcode;
    p_rsp_msg->dst_addr = ind->src;
    p_rsp_msg->msg_len = msg_len;
    memcpy(p_rsp_msg->msg, msg, msg_len);

    mesh_send_rsp(p_rsp_msg);
    os_free(p_rsp_msg);
}
#endif

/***********************mesh ON/OFF ctrl***********************/
uint16_t gatway_addr; 
uint16_t node_addr;
void mesh_led_on_off_msg_handler(mesh_model_msg_ind_t const *ind)
{
    struct mesh_gen_onoff_model_status_t status;
    struct mesh_gen_onoff_model_set_t *onoff_set = (struct mesh_gen_onoff_model_set_t *)ind->msg;;
        
    switch(ind->opcode)
    {
        case MESH_GEN_ONOFF_SET:
        case MESH_GEN_ONOFF_SET_UNACK:
        case MESH_GEN_ONOFF_GET:
        {
            if((ind->opcode == MESH_GEN_ONOFF_SET) || (ind->opcode == MESH_GEN_ONOFF_SET_UNACK))
            {
                pmu_set_led2_value(onoff_set->onoff);
                mesh_led_state.on_off_state = onoff_set->onoff;
                MESH_MODEL_LOG("gen on off=%d\r\n",onoff_set->onoff);
								MESH_MODEL_LOG("tid[%d],ttl:[%d],delay:[%d]\n", onoff_set->tid, onoff_set->ttl, onoff_set->delay);
            
				gatway_addr = ind->src;
				node_addr = ind->dst;
				if(onoff_set->onoff) {		//open
					update_led_pwm(5, 5, 5);
				} else {					//close
					update_led_pwm(0, 0, 0);
				}
				
				//MESH_MODEL_LOG("@---->src_addr[%d] dest_addr:[%d]\n", ind->src, ind->dst);
			}
            if(ind->opcode != MESH_GEN_ONOFF_SET_UNACK)
            {
                MESH_MODEL_LOG("@-->have ack");
				status.present_onoff = mesh_led_state.on_off_state;
                status.target_onoff = mesh_led_state.on_off_state;
                status.remain = 0;
                app_mesh_status_send_rsp(ind, MESH_GEN_ONOFF_STATUS, (uint8_t *)&status, sizeof(status.present_onoff));
            }
        }
        break;
        default:
        break;
    }
}
/***********************mesh ON/OFF ctrl***********************/


/***********************mesh lightness ctrl***********************/
static void mesh_lightness_get_handler(mesh_model_msg_ind_t const *ind)
{
    struct mesh_lightness_model_status_t status;
    
    status.current_level = mesh_led_state.lightness;
    status.target_level = mesh_led_state.lightness;
    status.remain = 0;
    app_mesh_status_send_rsp(ind, MESH_LIGHTNESS_STATUS, (uint8_t *)&status,sizeof(status));
}

static void mesh_lightness_set_handler(mesh_model_msg_ind_t const *ind)
{
    struct mesh_lightness_model_status_t status;
    struct mesh_lightness_model_set_t *lightness_set = (struct mesh_lightness_model_set_t *)ind->msg;

    MESH_MODEL_LOG("@-->Lightness Set=%x\r\n", lightness_set->level);
    mesh_led_state.lightness = lightness_set->level;
	
	color_temperature_set(mesh_led_state.ctl_temperature, mesh_led_state.lightness);
    if(ind->opcode == MESH_LIGHTNESS_SET)
    {
        status.current_level = mesh_led_state.lightness;
        status.target_level = lightness_set->level;
        status.remain = 0;
        app_mesh_status_send_rsp(ind, MESH_LIGHTNESS_STATUS, (uint8_t *)&status,sizeof(status));
    }
}

static void mesh_lightness_linear_handler(mesh_model_msg_ind_t const *ind)
{
    
}

static void mesh_lightness_range_handler(mesh_model_msg_ind_t const *ind)
{
    static uint16_t lightness_range_max = 0xffff;
    static uint16_t lightness_range_min = 1;
//    struct mesh_lightness_model_range_t *range_linear = (struct mesh_lightness_model_range_t *)ind->msg;

    if(ind->opcode == MESH_LIGHTNESS_RANGE_GET) {
        struct mesh_lightness_model_range_status_t status;
        status.status = 0;
        status.range_max = lightness_range_max;
        status.range_min = lightness_range_min;
        app_mesh_status_send_rsp(ind, MESH_LIGHTNESS_RANGE_STATUS, (uint8_t *)&status,sizeof(status));
    }
}

void mesh_lighness_msg_handler(mesh_model_msg_ind_t const *ind)
{
    switch(ind->opcode)
    {
        case MESH_LIGHTNESS_GET:
            mesh_lightness_get_handler(ind);
        break;
        case MESH_LIGHTNESS_SET:
        case MESH_LIGHTNESS_SET_UNACK:
            mesh_lightness_set_handler(ind);
        break;
        case MESH_LIGHTNESS_LINEAR_GET:
        case MESH_LIGHTNESS_LINEAR_SET:
        case MESH_LIGHTNESS_LINEAR_SET_UNACK:
            mesh_lightness_linear_handler(ind);
        break;
        case MESH_LIGHTNESS_LAST_GET:
        break;
        case MESH_LIGHTNESS_DEFAULT_GET:
        break;
        case MESH_LIGHTNESS_RANGE_GET:
            mesh_lightness_range_handler(ind);
        break;
        default:
        break;
    }
}
/***********************mesh lightness ctrl***********************/


/***********************mesh RGB ctrl***********************/
/****************HSL TO RGB***************/
#if 0
double Hue2RGB(double v1, double v2, double vH)
{   
	if (vH < 0)
	{
		vH += 10;
	}
	if (vH > 10)
		vH -= 10;   
	if (6 * vH < 10) 
		return v1 + (v2 - v1) * 6 * vH/100;   
	if (2 * vH < 10) 
		return v2;  
	if (3 * vH < 20) 
		return v1 + (v2 - v1) * ((20 / 3) - vH) * 6/100;   
	return (v1);
}

void HSL2RGB(uint16_t H,uint16_t S,uint16_t L)
{    
	uint16_t R,G,B;   
	uint16_t var_1, var_2; 
	uint16_t h = 10*H/65535;
	uint8_t s = 10*S/65535;
	uint8_t l = 10*L/65535;
    //co_printf("%x %x %x",H,S,L);
	// L:[0-1] S:[0-1] H:[0-1]   * 10
	// L:[0-10] S:[0-10] H:[0-10]
	if (s == 0)                           
	{        
		R = l * 0xffff/10;                    
		G = l * 0xffff/10;     
		B = l * 0xffff/10;  
	}   
	else   
	{        
		if (l < 5) 
		{
			var_2 = l * (10 + s)/10;  
		}
		else        
			var_2 = (l + s) - (s * l/10);   
		var_1 = 2 * l - var_2;    
		R = 0xffff * Hue2RGB(var_1, var_2, h + (10 / 3))/10;    
		G = 0xffff * Hue2RGB(var_1, var_2, h)/10;     
		B = 0xffff * Hue2RGB(var_1, var_2, (char)(h - (10 / 3)))/10;   
	}    

    MESH_MODEL_LOG("R=%x,G=%x,B=%x\r\n",(uint16_t)R,(uint16_t)G,(uint16_t)B);
}
#else
double HSL2RGBvalue(double n1,double n2, double hue)
{
    if(hue > 360.0)
        hue -= 360.0;
    else if(hue < 0)
        hue += 360.0;
    if(hue < 60.0)
        return n1+(n2-n1)*hue/60.0;
    else if(hue < 180.0)
        return n2;
    else if(hue < 240.0)
        return n1+(n2-n1)*(240.0-hue)/60.0;
    else
        return n1;
}

void HSL2RGB(uint16_t h, uint16_t s, uint16_t l, uint8_t *r, uint8_t *g, uint8_t *b)
{
    co_printf("H: %x S: %x L: %x\r\n", h, s, l);
    double cmax, cmin, H, S, L;
    H = h * 360.0 / 65535.0;
    L = l / 65535.0;
    S = s  / 65535.0;

    if(L <= 0.5)
        cmax = L * (1.0 + S);
    else
        cmax = L * (1.0 - S) + S;
    cmin = 2.0 * L - cmax;
    if(S == 0)
    {
        *r = *g = *b = L * 255.0;
    }
    else
    {
        *r = HSL2RGBvalue(cmin, cmax, H + 120.0) * 255;
        *g = HSL2RGBvalue(cmin, cmax, H) * 255.0;
        *b = HSL2RGBvalue(cmin, cmax, H - 120.0) * 255;
    }
    MESH_MODEL_LOG("R = %x, G = %x, B = %x\r\n", *r, *g, *b);
}
#endif
/****************HSL TO RGB***************/

/****************RGB to HSL***************/    
void RGBtoHSL(uint16_t rColor,uint16_t gColor,uint16_t bColor) 
{      
    float h=0, s=0, l=0;    
    // normalizes red-green-blue values    
    float r = rColor/255.f;    
    float g = gColor/255.f;   
    float b = bColor/255.f;   
    float maxVal = (r>g)?((r>b)?r:b):((g>b)?g:b);    
    float minVal = (r>g)?((g>b)?b:g):((r>b)?b:r);
//    uint16_t m_Hue,m_Saturation,m_Light;
   
    ///////////////////////////// hue//////////////// 
    if(maxVal == minVal)      
    {      
        h = 0; // undefined      
    } 
    else if(maxVal==r && g>=b)      
    {      
        h = 60.0f*(g-b)/(maxVal-minVal);      
    } 
    else if(maxVal==r && g<b)      
    {      
        h = 60.0f*(g-b)/(maxVal-minVal) + 360.0f;    
    } 
    else if(maxVal==g)      
    {      
        h = 60.0f*(b-r)/(maxVal-minVal) + 120.0f;      
    }    
    else if(maxVal==b)      
    {      
        h = 60.0f*(r-g)/(maxVal-minVal) + 240.0f;      
    }    
   
    ////////////////////// luminance ////////////////// 
    l = (maxVal+minVal)/2.0f;  
 
    ///////////////////////// saturation //////////////// 
    if(l == 0 || maxVal == minVal)    
    {      
        s = 0;      
    }  
    else if(0<l && l<=0.5f)    
    {      
        s = (maxVal-minVal)/(maxVal+minVal);      
    }    
    else if(l>0.5f)      
    {      
        s = (maxVal-minVal)/(2 - (maxVal+minVal)); //(maxVal-minVal > 0)?      
    }    
    float hue = (h>360)? 360 : ((h<0)?0:h);       
    float saturation = ((s>1)? 1 : ((s<0)?0:s))*100;      
    float luminance = ((l>1)? 1 : ((l<0)?0:l))*100;  
 
//    m_Hue = 240 * hue / 360.f;      
//    m_Saturation = 240 * saturation / 100.f;      
//    m_Light = 240 * luminance / 100.f; 
}  
/****************RGB to HSL***************/

static void mesh_hsl_get(mesh_model_msg_ind_t const *ind)
{
    struct mesh_hsl_model_status_t status;
    
    status.hsl_lightness = mesh_led_state.hsl_lightness;
    status.hsl_hue = mesh_led_state.hsl_hue;
    status.hsl_saturation = mesh_led_state.hsl_saturation;
    status.remain = 0;
    app_mesh_status_send_rsp(ind, MESH_HSL_STATUS, (uint8_t *)&status, sizeof(status));
}

static void mesh_hsl_range_get(mesh_model_msg_ind_t const *ind)
{
    struct mesh_hsl_model_range_t range;
    
    range.status_code = 0x00; // sucess;
    range.hsl_range_min = 0x0000;
    range.hsl_range_max = 0xffff;
    range.hsl_saturation_range_min = 0x0000;
    range.hsl_saturation_range_max = 0xffff;

    app_mesh_status_send_rsp(ind, MESH_HSL_RANGE_STATUS, (uint8_t *)&range, sizeof(range)); // range status
}

static void mesh_hsl_set(mesh_model_msg_ind_t const *ind)
{
    struct mesh_hsl_model_set_t *hsl_set = (struct mesh_hsl_model_set_t *)ind->msg;
    struct mesh_hsl_model_status_t status;
    uint8_t R,G,B;

    hsl_set = (struct mesh_hsl_model_set_t *)ind->msg;
    mesh_led_state.hsl_lightness = hsl_set->lightness;
    mesh_led_state.hsl_hue = hsl_set->hue;
    mesh_led_state.hsl_saturation = hsl_set->hsl_saturation;
	MESH_MODEL_LOG("mesh RGB set L[%d] H[%d] S[%d]\r\n", mesh_led_state.hsl_lightness, mesh_led_state.hsl_hue, mesh_led_state.hsl_saturation);
    HSL2RGB(mesh_led_state.hsl_hue,mesh_led_state.hsl_saturation,mesh_led_state.hsl_lightness,&R,&G,&B);

    update_led_pwm(R, G, B);		//
	
    if(ind->opcode == MESH_HSL_SET)
    {
        status.hsl_lightness = mesh_led_state.hsl_lightness;
        status.hsl_hue = mesh_led_state.hsl_hue;
        status.hsl_saturation = mesh_led_state.hsl_saturation;
        status.remain = 0;
        app_mesh_status_send_rsp(ind, MESH_HSL_STATUS, (uint8_t *)&status, sizeof(status));
    }
}

void mesh_RGB_msg_handler(mesh_model_msg_ind_t const *ind)
{
    switch(ind->opcode)
    {
        case MESH_HSL_GET:
            mesh_hsl_get(ind);
            break;
        case MESH_HSL_SET:
        case MESH_HSL_SET_UNACK:
            mesh_hsl_set(ind);
            break;
        case MESH_HSL_RANGE_GET:
            mesh_hsl_range_get(ind);
            break;
        default:
            break;
    }  
}
/***********************mesh RGB ctrl***********************/


/***********************mesh CTL ctrl***********************/
static void mesh_temperature_range_get(mesh_model_msg_ind_t const *ind)
{
    struct mesh_CTL_model_range_t range;
    
    range.status_code = 0; // sucess
    range.temperature_range_min = 0x320;
    range.temperature_range_max = 0x4e20;
    app_mesh_status_send_rsp(ind, MESH_TEMPERATURE_RANGE_STATUS, (uint8_t *)&range, sizeof(range));
}

static void mesh_temperature_get(mesh_model_msg_ind_t const *ind)
{
    struct mesh_CTL_temp_status_t status;
    
    status.current_temperature = mesh_led_state.ctl_temperature;
    status.current_delta_UV = mesh_led_state.delta_UV;
    status.target_temperature = mesh_led_state.ctl_temperature;
    status.target_delta_UV = mesh_led_state.delta_UV;
    status.remain = 0;
    app_mesh_status_send_rsp(ind, MESH_TEMPERATURE_STATUS, (uint8_t *)&status, sizeof(status));
}

static void mesh_temperature_set(mesh_model_msg_ind_t const *ind)
{
//    struct mesh_CTL_model_status_t status;
    struct mesh_ctl_temperature_set_t *ctl_set;
    
    ctl_set = (struct mesh_ctl_temperature_set_t *)ind->msg;

    mesh_led_state.ctl_temperature = ctl_set->ctl_temperature;
    mesh_led_state.delta_UV = ctl_set->ctl_delta_UV;
    MESH_MODEL_LOG("mesh color temperature set temp=%x UV=%x\r\n",mesh_led_state.ctl_temperature,mesh_led_state.delta_UV); 
    
	color_temperature_set(mesh_led_state.ctl_temperature, mesh_led_state.lightness);			//set colar temprature
	if(ind->opcode == MESH_TEMPERATURE_SET)
    {
        mesh_temperature_get(ind);
    }
}

void mesh_CTL_msg_handler(mesh_model_msg_ind_t const *ind)
{
    switch(ind->opcode)
    {
        case MESH_TEMPERATURE_GET:
            mesh_temperature_get(ind);
        break;
        case MESH_TEMPERATURE_RANGE_GET:
            mesh_temperature_range_get(ind);
        break;
        case MESH_TEMPERATURE_SET:
        case MESH_TEMPERATURE_SET_UNACK:
            mesh_temperature_set(ind);
        break;
        default:
        break;
    }  
}

/***********************mesh CTL ctrl***********************/

/***********************mesh LED ctrl***********************/
void mesh_dev_led_ctrl_init(void)
{
    mesh_led_state.on_off_state = 0;
    mesh_led_state.lightness = 0xffff;
    mesh_led_state.hsl_lightness = 0xffff;
    mesh_led_state.hsl_hue = 0xffff;
    mesh_led_state.hsl_saturation = 0xffff;
    mesh_led_state.ctl_lightness = 0xffff;
    mesh_led_state.ctl_temperature = 0x320;
    mesh_led_state.delta_UV = 0x0000;
}
/***********************mesh LED ctrl***********************/



