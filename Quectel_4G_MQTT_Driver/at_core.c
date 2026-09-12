#include "at_core.h"
#include <string.h>

static const at_transport_t *g_tp = NULL;

void at_set_transport(const at_transport_t *tp)
{
    g_tp = tp;
}

static uint32_t now_ms(void)
{
    return (g_tp && g_tp->tick_ms) ? g_tp->tick_ms() : 0;
}

uint32_t at_tick_ms(void)
{
    return now_ms();
}

void at_delay_ms(uint32_t ms)
{
    if (g_tp && g_tp->delay_ms) g_tp->delay_ms(ms);
}

at_status_t at_transport_send(const char *str)
{
    if (!g_tp || !g_tp->send) 
        return AT_BUSY;
    uint16_t len = (uint16_t)strlen(str);
    int n = g_tp->send((const uint8_t *)str, len);
    return (n == (int)len) ? AT_OK : AT_ERROR;
}

at_status_t at_send_bytes(const uint8_t *data, uint16_t len)
{
    if (!g_tp || !g_tp->send) return AT_BUSY;
    int n = g_tp->send(data, len);
    return (n == (int)len) ? AT_OK : AT_ERROR;
}

/* 读取一行（以 \n 结束），去掉 CR/LF，null 结尾。
 * deadline 为整体截止时间（基于 tick_ms）；若无 tick 则使用单字节超时 timeout_per_byte。 */
static at_status_t read_line(char *line, uint16_t max, uint32_t deadline, uint32_t timeout_per_byte)
{
    uint16_t i = 0;
    bool have_tick = (g_tp && g_tp->tick_ms) != 0;

    while (i < max - 1) {
        if (have_tick && now_ms() > deadline) return AT_TIMEOUT;

        uint8_t b;
        uint32_t bt = timeout_per_byte;
        if (have_tick) {
            int64_t rem = (int64_t)deadline - (int64_t)now_ms();
            if (rem < (int64_t)bt) bt = (rem > 0) ? (uint32_t)rem : 1;
        }

        int r = g_tp->recv(&b, 1, bt);
        if (r <= 0) {
            if (have_tick) continue;          /* 无 tick：继续等待直到整体超时 */
            else if (i == 0) return AT_TIMEOUT;
            else break;                       /* 无 tick 且已有数据：行结束 */
        }

        if (b == '\n') { line[i] = '\0'; return AT_OK; }
        if (b == '\r') continue;
        line[i++] = (char)b;
    }
    line[i] = '\0';
    return AT_OK;
}

at_status_t at_send_cmd(const char *cmd, char *resp, uint16_t resp_max, uint32_t timeout_ms)

{
    if (!g_tp) return AT_BUSY;

    at_transport_send(cmd);
    at_transport_send("\r");

    uint32_t deadline = now_ms() + timeout_ms;
    if (resp && resp_max) resp[0] = '\0';

    static char line[AT_LINE_MAX];
    while (1) 
    {
        at_status_t s = read_line(line, sizeof(line), deadline, 200);
        if (s == AT_TIMEOUT) 
            return AT_TIMEOUT;
        if (line[0] == '\0') 
            continue;
        if (strcmp(line, "OK") == 0) 
            return AT_OK;
        if (strcmp(line, "ERROR") == 0) 
            return AT_ERROR;
        if (strncmp(line, "+CME ERROR", 10) == 0 ||
            strncmp(line, "+CMS ERROR", 10) == 0) 
            return AT_ERROR;
        if (resp && resp_max) 
        {
            size_t cur = strlen(resp);
            size_t add = strlen(line);
            if (cur + add + 2 < resp_max) 
            {
                if (cur) 
                    strcat(resp, "\n");
                strcat(resp, line);
            } 
            else 
            {
                return AT_OVERFLOW;
            }
        }
    }
}

bool at_resp_contains(const char *resp, const char *token)
{
    return resp && token && strstr(resp, token) != NULL;
}

at_status_t at_wait_line(char *line, uint16_t max, const char *prefix, uint32_t timeout_ms)
{
    if (!g_tp) return AT_BUSY;
    uint32_t deadline = now_ms() + timeout_ms;
    uint16_t plen = prefix ? (uint16_t)strlen(prefix) : 0;

    while (1) {
        at_status_t s = read_line(line, max, deadline, 200);
        if (s == AT_TIMEOUT) return AT_TIMEOUT;
        if (line[0] == '\0') continue;
        if (!prefix) return AT_OK;
        if (strncmp(line, prefix, plen) == 0) return AT_OK;
    }
}

at_status_t at_poll_line(char *line, uint16_t max, uint32_t timeout_ms)
{
    if (!g_tp) return AT_BUSY;
    uint32_t deadline = now_ms() + timeout_ms;
    return read_line(line, max, deadline, timeout_ms);
}
