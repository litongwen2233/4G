#include"MQTT_client.h"

volatile uint8_t mqtt_read_flag = 0;
volatile uint16_t mqtt_read_rxLen = 0;
//指向当前模块状态
static mqtt_status_t mqtt_Status = mqtt_QMTCFG_pdp;
uint8_t       client_idx;     /* MQTT 客户端标识，范围 0~5 */
uint8_t       pdp_cid;        /* 使用的 PDP 上下文，默认 1（与 fourg_net PDP 对应） */
uint8_t recv_urc = 0;  // 默认不接收 URC 消息
uint8_t len_enable = 0;  // 默认不启用长度字段
uint8_t keepalive = 60;  // 默认心跳间隔为 60 秒
uint8_t clean_session = 1;  // 默认清除会话
uint8_t pkt_timeout;//数据包超时时间（秒）
uint8_t retry_times;//重试次数
uint8_t host ;//主机地址
uint8_t port ;//端口
uint8_t user; //用户名
uint8_t pwd; //密码
uint8_t clientid;// 客户端ID
uint16_t msgid;
char *topic;
uint8_t qos;
uint8_t retain;
uint8_t mqtt_send_data[256] = {0};
uint8_t mqtt_send_len = 0;
static uint8_t _4G_init_flag = 1;
static void mqtt_read_handle()
{
    
    static uint8_t mqtt_init_flag = 1;
    if(mqtt_read_flag == 0)
        return ;
    mqtt_read_flag = 0;
    if(_4G_init_flag == 1)
    {
        if(EG800SK_Data_Handle(mqtt_read_rxLen)) 
            _4G_init_flag = 0;
    }
    else if(mqtt_init_flag)
    {
        
    }
    else
    {

    }
}
static void mqtt_write_handle()
{
    char cmd[30] = {0};
    switch (mqtt_Status)
    {
        case mqtt_QMTCFG_pdp:
            snprintf(cmd, sizeof(cmd), "AT+QMTCFG=\"pdpcid\",%d,%d",
                (int)client_idx, (int)pdp_cid);
            break;
        case mqtt_QMTCFG_rec:
            snprintf(cmd, sizeof(cmd), "AT+QMTCFG=\"recv/mode\",%d,%d,%d",
                (int)client_idx, recv_urc, len_enable);
            break;
        case mqtt_QMTCFFG_keeep:
            snprintf(cmd, sizeof(cmd), "AT+QMTCFG=\"keepalive\",%d,%d",
                (int)client_idx, (int)keepalive);
            break;
        case mqtt_QMTCFFG_ses:
            snprintf(cmd, sizeof(cmd), "AT+QMTCFG=\"session\",%d,%d",
                (int)client_idx, clean_session ? 1 : 0);
            break;
        case mqtt_QMTCFFG_timeout:
            snprintf(cmd, sizeof(cmd), "AT+QMTCFG=\"timeout\",%d,%d,%d,0",
                (int)client_idx, (int)pkt_timeout, (int)retry_times);
            break;
        case mqtt_QMTOPEN:
            snprintf(cmd, sizeof(cmd), "AT+QMTOPEN=%d,\"%s\",%d",
                (int)client_idx, host, (int)port);
            break;
        case mqtt_QMTCONN:
        {
            if (user && pwd)
                snprintf(cmd, sizeof(cmd), "AT+QMTCONN=%d,\"%s\",\"%s\",\"%s\"",
                        (int)client_idx, clientid, user, pwd);
            else
                snprintf(cmd, sizeof(cmd), "AT+QMTCONN=%d,\"%s\"",
                        (int)client_idx, clientid);
            break;
        }
        case mqtt_QMTSUB:
            snprintf(cmd, sizeof(cmd), "AT+QMTSUB=%d,%d,\"%s\",%d",
                (int)client_idx, (int)msgid, topic, (int)qos);
            break;
        case mqtt_QMTPUB:
            snprintf(cmd, sizeof(cmd), "AT+QMTUNS=%d,%d,\"%s\"",
                (int)client_idx, (int)msgid, topic);
            break;
        case mqtt_QMTDISC:
            snprintf(cmd, sizeof(cmd), "AT+QMTPUBEX=%d,%d,%d,%d,\"%s\",%d",
                (int)client_idx, (int)msgid, (int)qos, (int)retain, topic, (int)mqtt_send_len);
            break;
        default:
            break;
    }
    EG800SK_Send(cmd);
}

void mqtt_register(mqtt_client_t *c)
{
    EG800SK_Init(&c->_4G_date);
}

void mqtt_Handle()
{
    mqtt_read_handle();
    if(_4G_init_flag == 0)
        mqtt_write_handle();
    else
        EG800SK_Config_Status();
} 