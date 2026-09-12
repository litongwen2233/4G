#ifndef __MQTT_CLIENT_H__
#define __MQTT_CLIENT_H__

#include"EG800AK.h"

/* MQTT 客户端错误码（基于 Quectel LTE Standard(A) MQTT 应用指导 V1.6）。 */
typedef enum {
    MQTT_OK = 0,
    MQTT_ERR_AT,        /* 底层 AT 通信失败 */
    MQTT_ERR_OPEN,      /* QMTOPEN 结果非 0 */
    MQTT_ERR_CONN,      /* QMTCONN 结果非 0 */
    MQTT_ERR_SUB,       /* QMTSUB 结果非 0 */
    MQTT_ERR_PUB,       /* QMTPUBEX 结果非 0 */
    MQTT_ERR_DISC,      /* QMTDISC 结果非 0 */
    MQTT_ERR_CLOSE,     /* QMTCLOSE 结果非 0 */
    MQTT_ERR_PARAM,
    MQTT_ERR_TIMEOUT
} mqtt_err_t;

typedef enum 
{
    mqtt_QMTCFG_pdp = 0,//设置PDP上下文ID
    mqtt_QMTCFG_rec,//设置接收模式
    mqtt_QMTCFFG_keeep,//心跳保活实践
    mqtt_QMTCFFG_ses,//设置会话类型
    mqtt_QMTCFFG_timeout,//设置超时时间
    mqtt_QMTOPEN,//打开MQTT客户端
    mqtt_QMTCONN,//连接MQTT服务器
    mqtt_QMTSUB,//订阅主题
    mqtt_QMTPUB,//发布消息
    mqtt_QMTDISC,//断开MQTT连接
}mqtt_status_t;

/* 接收订阅消息回调。topic/payload 指向驱动内部缓冲，仅在回调内有效。 */
typedef void (*mqtt_recv_cb)(uint8_t client_idx, uint16_t msgid,
                             const char *topic, const char *payload);
/* 链路状态变化回调（+QMTSTAT），err_code 见 MQTT 应用指导表 4。 */
typedef void (*mqtt_state_cb)(uint8_t client_idx, int err_code);
/* 保活 Ping 结果回调（+QMTPING）。 */
typedef void (*mqtt_ping_cb)(uint8_t client_idx, int result);

typedef struct {
    uint8_t       client_idx;     /* MQTT 客户端标识，范围 0~5 */
    uint8_t       pdp_cid;        /* 使用的 PDP 上下文，默认 1（与 fourg_net PDP 对应） */
    mqtt_recv_cb  on_message;     /* 订阅消息回调 */
    mqtt_state_cb on_state;       /* 链路状态变化回调 */
    mqtt_ping_cb  on_ping;        /* 保活 Ping 结果回调 */
    _4G_EG800AK_Handle _4G_date; /* 4G 模块句柄 */
} mqtt_client_t;

static inline mqtt_Read_Flag(uint16_t Rx_len)
{ 
    extern volatile uint8_t  mqtt_read_flag;
    extern volatile uint16_t mqtt_read_rxLen;
    mqtt_read_flag = 1; 
    mqtt_read_rxLen = Rx_len;
}

void mqtt_register(mqtt_client_t *c);
void mqtt_Handle(void);

#endif