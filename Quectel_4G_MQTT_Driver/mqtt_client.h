#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

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
    mqtt_recv_cb  on_message;
    mqtt_state_cb on_state;
    mqtt_ping_cb  on_ping;
} mqtt_client_t;

/* 注册客户端（用于 URC 分发，支持最多 6 个 client_idx）。 */
void mqtt_register(mqtt_client_t *c);

/* 基础配置：PDP 绑定、接收模式、保活、会话、传输超时。
 * recv_urc=1 以 URC 上报；len_enable=1 在 URC 中携带负载长度；
 * keepalive 秒（0~3600，默认 120）；clean_session=1 清理会话；
 * pkt_timeout 秒（1~60）；retry_times（0~10）。 */
mqtt_err_t mqtt_configure(mqtt_client_t *c,
                          bool recv_urc, bool len_enable,
                          uint16_t keepalive, uint8_t clean_session,
                          uint8_t pkt_timeout, uint8_t retry_times);

/* 打开 MQTT 网络（AT+QMTOPEN）。host 可为域名或 IP，port 如 1883/8883。 */
mqtt_err_t mqtt_open(mqtt_client_t *c, const char *host, uint16_t port);

/* 连接 MQTT 服务器（AT+QMTCONN）。user/pwd 为 NULL 时省略。 */
mqtt_err_t mqtt_connect(mqtt_client_t *c, const char *clientid,
                        const char *user, const char *pwd);

/* 订阅主题（AT+QMTSUB）。 */
mqtt_err_t mqtt_subscribe(mqtt_client_t *c, uint16_t msgid,
                          const char *topic, uint8_t qos);

/* 退订主题（AT+QMTUNS）。 */
mqtt_err_t mqtt_unsubscribe(mqtt_client_t *c, uint16_t msgid, const char *topic);

/* 发布字符串消息（AT+QMTPUBEX）。payload 按字符串长度发送。 */
mqtt_err_t mqtt_publish(mqtt_client_t *c, uint16_t msgid, uint8_t qos,
                        uint8_t retain, const char *topic, const char *payload);

/* 发布二进制/定长消息（AT+QMTPUBEX）。发送 data 的前 len 字节。 */
mqtt_err_t mqtt_publish_ex(mqtt_client_t *c, uint16_t msgid, uint8_t qos,
                           uint8_t retain, const char *topic,
                           const uint8_t *data, uint16_t len);

/* 断开与服务器的连接（AT+QMTDISC）。 */
mqtt_err_t mqtt_disconnect(mqtt_client_t *c);

/* 关闭 MQTT 网络（AT+QMTCLOSE）。 */
mqtt_err_t mqtt_close(mqtt_client_t *c);

/* 轮询并处理异步 URC（+QMTRECV / +QMTSTAT / +QMTPING）。
 * 建议在应用主循环或定时任务中周期性调用。timeout_ms 为单次轮询空闲等待。 */
void mqtt_poll(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_CLIENT_H */
