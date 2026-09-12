#include "fourg_net.h"
#include "at_core.h"
#include <string.h>
#include <stdio.h>

/* 解析 "+CREG: <n>,<stat>" / "+CGREG: ..." 中的 stat 字段（逗号后第 2 个整数）。 */
static int parse_reg_stat(const char *resp, const char *prefix)
{
    const char *p = strstr(resp, prefix);
    if (!p) return -1;
    p += strlen(prefix);
    int n, stat;
    if (sscanf(p, ":%d,%d", &n, &stat) == 2) 
        return stat;
    if (sscanf(p, " %d,%d", &n, &stat) == 2) 
        return stat;
    return -1;
}

fourg_err_t fourg_handshake(uint32_t timeout_ms)
{
    at_status_t s = at_send_cmd("AT", NULL, 0, timeout_ms);
    return (s == AT_OK) ? FOURG_OK : FOURG_ERR_AT;
}

fourg_err_t fourg_echo_off(void)
{
    at_status_t s = at_send_cmd("ATE0", NULL, 0, 1000);
    return (s == AT_OK) ? FOURG_OK : FOURG_ERR_AT;
}

fourg_err_t fourg_check_sim(uint32_t timeout_ms)
{
    static char resp[128];
    at_status_t s = at_send_cmd("AT+CPIN?", resp, sizeof(resp), timeout_ms);
    if (s != AT_OK) return FOURG_ERR_AT;
    if (strstr(resp, "READY")) return FOURG_OK;
    return FOURG_ERR_SIM;
}

fourg_err_t fourg_wait_network(uint32_t timeout_ms)
{
    uint32_t start = at_tick_ms();
    for (;;) {
        static char resp[128];
        int cs = -1, ps = -1;
        //CS域网络注册
        if (at_send_cmd("AT+CREG?", resp, sizeof(resp), 1000) == AT_OK)
            cs = parse_reg_stat(resp, "+CREG");
        //PS域网络注册
        if (at_send_cmd("AT+CGREG?", resp, sizeof(resp), 1000) == AT_OK)
            ps = parse_reg_stat(resp, "+CGREG");

        if ((cs == 1 || cs == 5) && (ps == 1 || ps == 5))
            return FOURG_OK;

        if (at_tick_ms() - start > timeout_ms) return FOURG_ERR_TIMEOUT;
        at_delay_ms(1000);
    }
}

fourg_err_t fourg_attach(uint32_t timeout_ms)
{
    if (at_send_cmd("AT+CGATT=1", NULL, 0, 10000) != AT_OK)
        return FOURG_ERR_AT;

    uint32_t start = at_tick_ms();
    for (;;) {
        static char resp[64];
        if (at_send_cmd("AT+CGATT?", resp, sizeof(resp), 2000) == AT_OK) {
            int state = -1;
            const char *p = strstr(resp, "+CGATT:");
            if (p && sscanf(p, "+CGATT: %d", &state) == 1 && state == 1)
                return FOURG_OK;
        }
        if (at_tick_ms() - start > timeout_ms) return FOURG_ERR_ATTACH;
        at_delay_ms(1000);
    }
}

fourg_err_t fourg_define_pdp(const fourg_pdp_t *pdp)
{
    if (!pdp || !pdp->apn) return FOURG_ERR_PARAM;
    char cmd[160];
    const char *type = pdp->pdp_type ? pdp->pdp_type : "IP";
    int n = snprintf(cmd, sizeof(cmd), "AT+CGDCONT=%u,\"%s\",\"%s\"",
                     (unsigned)pdp->cid, type, pdp->apn);
    if (n < 0 || n >= (int)sizeof(cmd)) return FOURG_ERR_PARAM;
    return (at_send_cmd(cmd, NULL, 0, 2000) == AT_OK) ? FOURG_OK : FOURG_ERR_PDP;
}

fourg_err_t fourg_activate_pdp(const fourg_pdp_t *pdp)
{
    if (!pdp) return FOURG_ERR_PARAM;
    char cmd[32];

    snprintf(cmd, sizeof(cmd), "AT+CGACT=1,%u", (unsigned)pdp->cid);
    if (at_send_cmd(cmd, NULL, 0, 15000) != AT_OK) return FOURG_ERR_PDP;

    uint32_t start = at_tick_ms();
    for (;;) {
        static char resp[128];
        if (at_send_cmd("AT+CGACT?", resp, sizeof(resp), 2000) == AT_OK) 
        {
            /* +CGACT: <cid>,<state> 可能有多行，扫描目标 cid。 */
            const char *p = resp;
            for (;;) {
                const char *line = strstr(p, "+CGACT:");
                if (!line) break;
                int cid = -1, state = -1;
                if (sscanf(line, "+CGACT: %d,%d", &cid, &state) == 2 &&
                    cid == (int)pdp->cid && state == 1)
                    return FOURG_OK;
                p = line + 1;
            }
        }
        if (at_tick_ms() - start > 15000) return FOURG_ERR_PDP;
        at_delay_ms(1000);
    }
}

fourg_err_t fourg_get_signal(int *rssi, int *ber)
{
    static char resp[64];
    if (at_send_cmd("AT+CSQ", resp, sizeof(resp), 2000) != AT_OK)
        return FOURG_ERR_AT;
    int r = -1, b = -1;
    const char *p = strstr(resp, "+CSQ:");
    if (p && sscanf(p, "+CSQ: %d,%d", &r, &b) == 2) {
        if (rssi) *rssi = r;
        if (ber) *ber = b;
        return FOURG_OK;
    }
    return FOURG_ERR_AT;
}

fourg_err_t fourg_init(const fourg_pdp_t *pdp, uint32_t timeout_ms)
{
    if (!pdp) return FOURG_ERR_PARAM;

    if (fourg_handshake(2000) != FOURG_OK) return FOURG_ERR_AT;
    fourg_echo_off();
    if (fourg_check_sim(10000) != FOURG_OK) return FOURG_ERR_SIM;
    if (fourg_wait_network(timeout_ms) != FOURG_OK) return FOURG_ERR_REG;
    if (fourg_attach(timeout_ms) != FOURG_OK) return FOURG_ERR_ATTACH;
    if (fourg_define_pdp(pdp) != FOURG_OK) return FOURG_ERR_PDP;
    if (fourg_activate_pdp(pdp) != FOURG_OK) return FOURG_ERR_PDP;

    return FOURG_OK;
}

fourg_err_t fourg_deactivate_pdp(const fourg_pdp_t *pdp)
{
    if (!pdp) return FOURG_ERR_PARAM;
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CGACT=0,%u", (unsigned)pdp->cid);
    return (at_send_cmd(cmd, NULL, 0, 15000) == AT_OK) ? FOURG_OK : FOURG_ERR_PDP;
}
