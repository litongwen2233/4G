#ifndef FOURG_NET_H
#define FOURG_NET_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 4G 通信建立错误码（基于 Quectel LTE Standard(A) AT 命令手册 V1.3）。 */
typedef enum {
    FOURG_OK = 0,
    FOURG_ERR_AT,          /* 底层 AT 通信失败 */
    FOURG_ERR_SIM,         /* SIM 卡未就绪 / 需要 PIN */
    FOURG_ERR_REG,         /* 网络注册失败（CS/PS 域） */
    FOURG_ERR_ATTACH,      /* PS 域附着失败 */
    FOURG_ERR_PDP,         /* PDP 上下文定义/激活失败 */
    FOURG_ERR_TIMEOUT,     /* 等待超时 */
    FOURG_ERR_PARAM        /* 参数错误 */
} fourg_err_t;

/* PDP 上下文配置（对应 AT+CGDCONT / AT+CGACT）。 */
typedef struct {
    uint8_t  cid;          /* 上下文标识，范围 1~15，MQTT 默认使用 1 */
    const char *apn;       /* 接入点名称，如 "CMNET"/"UNINET"/"3gnet" */
    const char *username;  /* 认证用户名，无则填空 */
    const char *password;  /* 认证密码，无则填空 */
    const char *pdp_type;  /* "IP"(IPv4) / "IPV6" / "IPV4V6" */
} fourg_pdp_t;

/* 模块是否已完成上电握手（AT 回 OK）。 */
fourg_err_t fourg_handshake(uint32_t timeout_ms);

/* 关闭回显，避免命令回显污染解析（ATE0）。 */
fourg_err_t fourg_echo_off(void);

/* 查询 SIM 卡状态，需返回 READY（对应 AT+CPIN?）。 */
fourg_err_t fourg_check_sim(uint32_t timeout_ms);

/* 等待 CS 域（AT+CREG）与 PS 域（AT+CGREG）注册成功，
 * stat 为 1（本地网）或 5（漫游网）视为已注册。 */
fourg_err_t fourg_wait_network(uint32_t timeout_ms);

/* PS 域附着（AT+CGATT=1），并等待状态为 1。 */
fourg_err_t fourg_attach(uint32_t timeout_ms);

/* 定义 PDP 上下文（AT+CGDCONT=<cid>,<type>,<apn>）。 */
fourg_err_t fourg_define_pdp(const fourg_pdp_t *pdp);

/* 激活 PDP 上下文（AT+CGACT=1,<cid>）并确认状态为 1。 */
fourg_err_t fourg_activate_pdp(const fourg_pdp_t *pdp);

/* 查询信号强度（AT+CSQ），rssi/ber 经出参返回；rssi 0~31，99 表示未知。 */
fourg_err_t fourg_get_signal(int *rssi, int *ber);

/* 完整的 4G 数据通道建立流程：握手→关回显→SIM→注册→附着→定义并激活 PDP。
 * 成功后即可在此 PDP(cid) 之上建立 MQTT（QMTOPEN 默认复用 cid 1）。 */
fourg_err_t fourg_init(const fourg_pdp_t *pdp, uint32_t timeout_ms);

/* 去激活 PDP 上下文（AT+CGACT=0,<cid>）。 */
fourg_err_t fourg_deactivate_pdp(const fourg_pdp_t *pdp);

#ifdef __cplusplus
}
#endif

#endif /* FOURG_NET_H */
