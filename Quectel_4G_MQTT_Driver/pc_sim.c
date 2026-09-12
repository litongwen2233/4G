#include "pc_sim.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <windows.h>

/* ===========================================================================
 * 仅用于 PC 演示：实现 transport_template.c 所需的底层接口桩，并内置一个“虚拟
 * 模组”，根据 MCU 发出的 AT 命令回送对应的响应，使整套驱动能在无真实硬件时
 * 跑通 4G->MQTT 流程并打印收发过程。
 * 接真机时：删除 pc_sim.c / pc_sim.h，在真实工程中实现 uart_recv_byte /
 * uart_send_byte / sys_tick_ms / sys_delay_ms 四个接口即可。
 * ========================================================================= */

/* 发送/回复 打印封装（PC 演示日志）：做好区分
 *   app_send  —— [TX] 代表 MCU -> 模组 的发送侧
 *   app_reply —— [RX] 代表 模组 -> MCU 的回复侧 */
static void app_send(const char *msg)   { printf("[TX] %s\r\n", msg); }
static void app_reply(const char *line) { printf("[RX] %s\r\n", line); }

#define RX_FIFO_SZ 4096
static uint8_t  g_rxfifo[RX_FIFO_SZ];
static uint16_t g_rx_head = 0, g_rx_tail = 0;
static uint8_t  g_txbuf[256];
static uint16_t g_txlen = 0;
static int      g_payload_rem = 0;   /* QMTPUBEX '>' 之后待忽略的负载字节数 */

/* 把一行回复送入接收 FIFO（供驱动读取），并用 app_reply 打印。 */
static void rx_put(const char *s)
{
    app_reply(s);
    size_t n = strlen(s);
    for (size_t i = 0; i < n; i++) {
        uint16_t nx = (uint16_t)((g_rx_head + 1) % RX_FIFO_SZ);
        if (nx != g_rx_tail) { g_rxfifo[g_rx_head] = (uint8_t)s[i]; g_rx_head = nx; }
    }
    uint16_t nx = (uint16_t)((g_rx_head + 1) % RX_FIFO_SZ);
    if (nx != g_rx_tail) { g_rxfifo[g_rx_head] = '\n'; g_rx_head = nx; }
}

/* 供 example.c 调用：模拟服务器下行一条 URC。 */
void pc_sim_inject_recv(const char *urc)
{
    rx_put(urc);
}

/* 依据命令关键字返回模组应有的回复（仅供演示）。 */
static void module_respond(const char *cmd)
{
    if      (strcmp(cmd, "AT") == 0)                       { rx_put("OK"); return; }
    else if (strcmp(cmd, "ATE0") == 0)                     { rx_put("OK"); return; }
    else if (strncmp(cmd, "AT+CPIN?", 7) == 0)             { rx_put("+CPIN: READY"); rx_put("OK"); return; }
    else if (strncmp(cmd, "AT+CREG?", 7) == 0)             { rx_put("+CREG: 0,1");  rx_put("OK"); return; }
    else if (strncmp(cmd, "AT+CGREG?", 8) == 0)            { rx_put("+CGREG: 0,1"); rx_put("OK"); return; }
    else if (strncmp(cmd, "AT+CGATT=1", 10) == 0)          { rx_put("OK"); return; }
    else if (strncmp(cmd, "AT+CGATT?", 8) == 0)            { rx_put("+CGATT: 1"); rx_put("OK"); return; }
    else if (strncmp(cmd, "AT+CGDCONT", 10) == 0)          { rx_put("OK"); return; }
    else if (strncmp(cmd, "AT+CGACT=1", 10) == 0)          { rx_put("OK"); return; }
    else if (strncmp(cmd, "AT+CGACT?", 8) == 0)            { rx_put("+CGACT: 1,1"); rx_put("OK"); return; }
    else if (strncmp(cmd, "AT+QMTCFG", 8) == 0)            { rx_put("OK"); return; }
    else if (strncmp(cmd, "AT+QMTOPEN", 10) == 0)          { rx_put("OK"); rx_put("+QMTOPEN: 0,0"); return; }
    else if (strncmp(cmd, "AT+QMTCONN", 10) == 0)          { rx_put("OK"); rx_put("+QMTCONN: 0,0,0"); return; }
    else if (strncmp(cmd, "AT+QMTSUB", 9) == 0)            { rx_put("OK"); rx_put("+QMTSUB: 0,1,0,0"); return; }
    else if (strncmp(cmd, "AT+QMTUNS", 9) == 0)            { rx_put("OK"); rx_put("+QMTUNS: 0,1,0"); return; }
    else if (strncmp(cmd, "AT+QMTPUBEX", 11) == 0) {
        int len = 0;
        const char *p = strrchr(cmd, ',');
        if (p) len = atoi(p + 1);          /* 取最后一个字段：负载长度 */
        g_payload_rem = len;
        rx_put("OK"); rx_put(">"); rx_put("+QMTPUBEX: 0,0,0");
        return;
    }
    else if (strncmp(cmd, "AT+QMTDISC", 10) == 0)          { rx_put("OK"); rx_put("+QMTDISC: 0,0"); return; }
    else if (strncmp(cmd, "AT+QMTCLOSE", 11) == 0)         { rx_put("OK"); rx_put("+QMTCLOSE: 0,0"); return; }
    rx_put("OK");                                          /* 兜底 */
}

/* ---------- 底层接口桩（被 transport_template.c 引用） ---------- */
int uart_recv_byte(void)
{
    if (g_rx_head == g_rx_tail) return -1;
    uint8_t b = g_rxfifo[g_rx_tail];
    g_rx_tail = (uint16_t)((g_rx_tail + 1) % RX_FIFO_SZ);
    return b;
}

void uart_send_byte(uint8_t b)
{
    if (g_payload_rem > 0) { g_payload_rem--; return; }   /* 发布负载数据，不计为命令 */
    if (b == '\r' || b == '\n') {
        if (g_txlen > 0) {
            g_txbuf[g_txlen] = '\0';
            app_send((char *)g_txbuf);                    /* 发送侧：打印 MCU 发出的命令 */
            module_respond((char *)g_txbuf);              /* 回复侧：产生并打印模组回复 */
            g_txlen = 0;
        }
        return;
    }
    if (g_txlen < sizeof(g_txbuf) - 1) g_txbuf[g_txlen++] = b;
}

uint32_t sys_tick_ms(void) { return GetTickCount(); }   /* PC 演示用真实时钟；固件替换为 HAL 时基 */
void     sys_delay_ms(uint32_t ms) { Sleep(ms); }
