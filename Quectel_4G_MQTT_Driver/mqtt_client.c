#include "mqtt_client.h"
#include "at_core.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MQTT_CMD_BUF 512

static mqtt_client_t *g_clients[6] = {NULL};

void mqtt_register(mqtt_client_t *c)
{
    if (c && c->client_idx < 6) 
        g_clients[c->client_idx] = c;
}

static mqtt_client_t *find_client(uint8_t idx)
{
    return (idx < 6) ? g_clients[idx] : NULL;
}

/* 取冒号后第 n 个（0-based）以逗号分隔的整数字段。 */
static int parse_nth_int(const char *line, int n)
{
    const char *p = strchr(line, ':');
    if (!p) return -1;
    p++;
    for (int i = 0; i <= n; i++) {
        while (*p == ' ') p++;
        char *end;
        long v = strtol(p, &end, 10);
        if (end == p) return -1;
        if (i == n) return (int)v;
        p = end;
        while (*p && *p != ',') p++;
        if (*p == ',') p++;
    }
    return -1;
}

/* 解析 +QMTRECV 数据行，提取 idx/msgid/topic/payload（字符串模式，topic 与 payload 带引号）。 */
static void parse_recv(const char *line)
{
    const char *p = line + strlen("+QMTRECV:");
    long idx = -1, msgid = -1;
    char *end;

    idx = strtol(p, &end, 10); if (end == p) return; p = end;
    while (*p == ',' || *p == ' ') p++;
    msgid = strtol(p, &end, 10); if (end == p) return; p = end;
    while (*p == ',' || *p == ' ') p++;
    if (*p != '"') return;                       /* 无 topic：存储状态 URC，忽略 */

    p++;
    const char *q = strchr(p, '"');
    if (!q) return;
    static char topic[300];
    size_t tl = (size_t)(q - p);
    if (tl >= sizeof(topic)) tl = sizeof(topic) - 1;
    memcpy(topic, p, tl); topic[tl] = '\0';
    p = q + 1;
    while (*p == ',' || *p == ' ') p++;
    strtol(p, &end, 10); if (end != p) p = end;  /* 跳过可选长度字段 */
    while (*p == ',' || *p == ' ') p++;
    if (*p != '"') return;
    p++;
    const char *q2 = strchr(p, '"');
    if (!q2) return;
    static char payload[AT_LINE_MAX];
    size_t pl = (size_t)(q2 - p);
    if (pl >= sizeof(payload)) pl = sizeof(payload) - 1;
    memcpy(payload, p, pl); payload[pl] = '\0';

    mqtt_client_t *c = find_client((uint8_t)idx);
    if (c && c->on_message)
        c->on_message((uint8_t)idx, (uint16_t)msgid, topic, payload);
}

/**
 * @brief
 * 
 * @param c MQTT客户端实例指针，包含模块的句柄信息
 * @param recv_urc 是否启用URC（Unsolicited Result Code）接收模式
 * @param len_enable 是否在接收数据时附带长度信息
 * @param keepalive MQTT心跳保活时间（秒）
 * @param clean_session 是否启用清洁会话（0=持久会话，1=清洁会话）
 * @param pkt_timeout 数据包超时时间（秒）
 * @param retry_times 重试次数
 * @return mqtt_err_t 函数返回类型，MQTT操作的结果状态码（如 MQTT_OK、MQTT_ERR_PARAM、MQTT_ERR_AT）
 */
mqtt_err_t mqtt_configure(mqtt_client_t *c,
                          bool recv_urc, bool len_enable,
                          uint16_t keepalive, uint8_t clean_session,
                          uint8_t pkt_timeout, uint8_t retry_times)
{
    if (!c) return MQTT_ERR_PARAM;
    char cmd[MQTT_CMD_BUF];
    //设置PDP上下文ID
    snprintf(cmd, sizeof(cmd), "AT+QMTCFG=\"pdpcid\",%d,%d",
             (int)c->client_idx, (int)c->pdp_cid);
    if (at_send_cmd(cmd, NULL, 0, 1000) != AT_OK) return MQTT_ERR_AT;
    //设置接收模式
    snprintf(cmd, sizeof(cmd), "AT+QMTCFG=\"recv/mode\",%d,%d,%d",
             (int)c->client_idx, recv_urc ? 1 : 0, len_enable ? 1 : 0);
    if (at_send_cmd(cmd, NULL, 0, 1000) != AT_OK) return MQTT_ERR_AT;
    //设置心跳保活时间
    snprintf(cmd, sizeof(cmd), "AT+QMTCFG=\"keepalive\",%d,%d",
             (int)c->client_idx, (int)keepalive);
    if (at_send_cmd(cmd, NULL, 0, 1000) != AT_OK) return MQTT_ERR_AT;
    //设置会话类型
    snprintf(cmd, sizeof(cmd), "AT+QMTCFG=\"session\",%d,%d",
             (int)c->client_idx, clean_session ? 1 : 0);
    if (at_send_cmd(cmd, NULL, 0, 1000) != AT_OK) return MQTT_ERR_AT;
    //设置超时和重试参数
    snprintf(cmd, sizeof(cmd), "AT+QMTCFG=\"timeout\",%d,%d,%d,0",
             (int)c->client_idx, (int)pkt_timeout, (int)retry_times);
    if (at_send_cmd(cmd, NULL, 0, 1000) != AT_OK) return MQTT_ERR_AT;

    return MQTT_OK;
}
/**
 * @brief 
 * 
 * @param c 
 * @param host 主机地址
 * @param port 端口
 * @return mqtt_err_t 
 */
mqtt_err_t mqtt_open(mqtt_client_t *c, const char *host, uint16_t port)
{
    if (!c || !host) return MQTT_ERR_PARAM;
    char cmd[MQTT_CMD_BUF];
    static char line[AT_LINE_MAX];

    snprintf(cmd, sizeof(cmd), "AT+QMTOPEN=%d,\"%s\",%d",
             (int)c->client_idx, host, (int)port);
    if (at_send_cmd(cmd, NULL, 0, 1000) != AT_OK) return MQTT_ERR_AT;

    if (at_wait_line(line, sizeof(line), "+QMTOPEN:", 120000) != AT_OK)
        return MQTT_ERR_TIMEOUT;
    return (parse_nth_int(line, 1) == 0) ? MQTT_OK : MQTT_ERR_OPEN;
}

/**
 * @brief 连接MQTT服务器
 * 
 * @param c 句柄
 * @param clientid 客户端ID
 * @param user 用户名
 * @param pwd 密码
 * @return mqtt_err_t 
 */
mqtt_err_t mqtt_connect(mqtt_client_t *c, const char *clientid,
                        const char *user, const char *pwd)
{
    if (!c || !clientid) return MQTT_ERR_PARAM;
    char cmd[MQTT_CMD_BUF];
    static char line[AT_LINE_MAX];

    if (user && pwd)
        snprintf(cmd, sizeof(cmd), "AT+QMTCONN=%d,\"%s\",\"%s\",\"%s\"",
                 (int)c->client_idx, clientid, user, pwd);
    else
        snprintf(cmd, sizeof(cmd), "AT+QMTCONN=%d,\"%s\"",
                 (int)c->client_idx, clientid);

    if (at_send_cmd(cmd, NULL, 0, 1000) != AT_OK) return MQTT_ERR_AT;

    if (at_wait_line(line, sizeof(line), "+QMTCONN:", 30000) != AT_OK)
        return MQTT_ERR_TIMEOUT;
    return (parse_nth_int(line, 1) == 0) ? MQTT_OK : MQTT_ERR_CONN;
}

mqtt_err_t mqtt_subscribe(mqtt_client_t *c, uint16_t msgid,
                          const char *topic, uint8_t qos)
{
    if (!c || !topic) return MQTT_ERR_PARAM;
    char cmd[MQTT_CMD_BUF];
    static char line[AT_LINE_MAX];

    snprintf(cmd, sizeof(cmd), "AT+QMTSUB=%d,%d,\"%s\",%d",
             (int)c->client_idx, (int)msgid, topic, (int)qos);
    if (at_send_cmd(cmd, NULL, 0, 1000) != AT_OK) return MQTT_ERR_AT;

    if (at_wait_line(line, sizeof(line), "+QMTSUB:", 30000) != AT_OK)
        return MQTT_ERR_TIMEOUT;
    return (parse_nth_int(line, 2) == 0) ? MQTT_OK : MQTT_ERR_SUB;
}

mqtt_err_t mqtt_unsubscribe(mqtt_client_t *c, uint16_t msgid, const char *topic)
{
    if (!c || !topic) return MQTT_ERR_PARAM;
    char cmd[MQTT_CMD_BUF];
    static char line[AT_LINE_MAX];

    snprintf(cmd, sizeof(cmd), "AT+QMTUNS=%d,%d,\"%s\"",
             (int)c->client_idx, (int)msgid, topic);
    if (at_send_cmd(cmd, NULL, 0, 1000) != AT_OK) return MQTT_ERR_AT;

    if (at_wait_line(line, sizeof(line), "+QMTUNS:", 30000) != AT_OK)
        return MQTT_ERR_TIMEOUT;
    return (parse_nth_int(line, 2) == 0) ? MQTT_OK : MQTT_ERR_SUB;
}

mqtt_err_t mqtt_publish_ex(mqtt_client_t *c, uint16_t msgid, uint8_t qos,
                           uint8_t retain, const char *topic,
                           const uint8_t *data, uint16_t len)
{
    if (!c || !topic || (!data && len)) return MQTT_ERR_PARAM;
    char cmd[MQTT_CMD_BUF];
    static char line[AT_LINE_MAX];

    if (qos == 0) msgid = 0;   /* QoS=0 时 msgid 必须为 0 */

    snprintf(cmd, sizeof(cmd), "AT+QMTPUBEX=%d,%d,%d,%d,\"%s\",%d",
             (int)c->client_idx, (int)msgid, (int)qos, (int)retain, topic, (int)len);
    if (at_send_cmd(cmd, NULL, 0, 1000) != AT_OK) return MQTT_ERR_AT;

    /* 等待 ">" 提示后发送负载。 */
    if (at_wait_line(line, sizeof(line), ">", 5000) != AT_OK)
        return MQTT_ERR_TIMEOUT;
    if (len && at_send_bytes(data, len) != AT_OK) return MQTT_ERR_AT;

    if (at_wait_line(line, sizeof(line), "+QMTPUBEX:", 30000) != AT_OK)
        return MQTT_ERR_TIMEOUT;
    return (parse_nth_int(line, 2) == 0) ? MQTT_OK : MQTT_ERR_PUB;
}

mqtt_err_t mqtt_publish(mqtt_client_t *c, uint16_t msgid, uint8_t qos,
                        uint8_t retain, const char *topic, const char *payload)
{
    uint16_t len = payload ? (uint16_t)strlen(payload) : 0;
    return mqtt_publish_ex(c, msgid, qos, retain, topic,
                           (const uint8_t *)payload, len);
}

mqtt_err_t mqtt_disconnect(mqtt_client_t *c)
{
    if (!c) return MQTT_ERR_PARAM;
    char cmd[MQTT_CMD_BUF];
    static char line[AT_LINE_MAX];

    snprintf(cmd, sizeof(cmd), "AT+QMTDISC=%d", (int)c->client_idx);
    if (at_send_cmd(cmd, NULL, 0, 1000) != AT_OK) return MQTT_ERR_AT;

    if (at_wait_line(line, sizeof(line), "+QMTDISC:", 30000) != AT_OK)
        return MQTT_ERR_TIMEOUT;
    return (parse_nth_int(line, 1) == 0) ? MQTT_OK : MQTT_ERR_DISC;
}

mqtt_err_t mqtt_close(mqtt_client_t *c)
{
    if (!c) return MQTT_ERR_PARAM;
    char cmd[MQTT_CMD_BUF];
    static char line[AT_LINE_MAX];

    snprintf(cmd, sizeof(cmd), "AT+QMTCLOSE=%d", (int)c->client_idx);
    if (at_send_cmd(cmd, NULL, 0, 1000) != AT_OK) return MQTT_ERR_AT;

    if (at_wait_line(line, sizeof(line), "+QMTCLOSE:", 30000) != AT_OK)
        return MQTT_ERR_TIMEOUT;
    return (parse_nth_int(line, 1) == 0) ? MQTT_OK : MQTT_ERR_CLOSE;
}

void mqtt_poll(uint32_t timeout_ms)
{
    static char line[AT_LINE_MAX];
    uint32_t start = at_tick_ms();

    for (;;) {
        at_status_t s = at_poll_line(line, sizeof(line), 50);
        if (s == AT_TIMEOUT) break;          /* 空闲 */
        if (s != AT_OK) continue;

        if (strncmp(line, "+QMTRECV:", 9) == 0) {
            parse_recv(line);
        } else if (strncmp(line, "+QMTSTAT:", 9) == 0) {
            int idx = parse_nth_int(line, 0);
            int err = parse_nth_int(line, 1);
            mqtt_client_t *c = find_client((uint8_t)idx);
            if (c && c->on_state) c->on_state((uint8_t)idx, err);
        } else if (strncmp(line, "+QMTPING:", 9) == 0) {
            int idx = parse_nth_int(line, 0);
            int r = parse_nth_int(line, 1);
            mqtt_client_t *c = find_client((uint8_t)idx);
            if (c && c->on_ping) c->on_ping((uint8_t)idx, r);
        }

        if (at_tick_ms() - start > timeout_ms) break;
    }
}
