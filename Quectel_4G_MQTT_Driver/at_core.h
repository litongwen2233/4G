#ifndef AT_CORE_H
#define AT_CORE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 单条 AT 回复行的最大长度。MQTT 接收缓存 +QMTRECV 行可长达 1500 字节负载，
 * 因此本驱动统一使用 1600 字节行缓冲。 */
#define AT_LINE_MAX 1600

/* 底层串口收发抽象。用户需根据所用 MCU（如 STM32 HAL/UART、ESP-IDF 等）实现。 */
typedef struct {
    /* 发送 len 字节，返回实际发送字节数（等于 len 表示成功）。 */
    int (*send)(const uint8_t *data, uint16_t len);
    /* 在 timeout_ms 内接收最多 len 字节，返回实际接收字节数（0 表示超时）。 */
    int (*recv)(uint8_t *data, uint16_t len, uint32_t timeout_ms);
    /* 返回单调毫秒时钟。用于命令整体超时控制；若为空，则退化为单字节 200ms 超时。 */
    uint32_t (*tick_ms)(void);
    /* 毫秒延时（用于轮询间隔），可为空。 */
    void (*delay_ms)(uint32_t ms);
} at_transport_t;

typedef enum {
    AT_OK = 0,        /* 收到 OK */
    AT_ERROR,         /* 收到 ERROR / +CME ERROR / +CMS ERROR */
    AT_TIMEOUT,       /* 在限定时间内未收到期望结果 */
    AT_BUSY,          /* 未设置传输层 */
    AT_OVERFLOW       /* 行/响应缓冲溢出 */
} at_status_t;

/* 安装传输层。 */
void at_set_transport(const at_transport_t *tp);

/* 返回当前毫秒时钟（未提供 tick_ms 时返回 0）。 */
uint32_t at_tick_ms(void);

/* 毫秒延时（未提供 delay_ms 时为空操作）。 */
void at_delay_ms(uint32_t ms);

/* 直接发送原始字符串（不含结束符）。 */
at_status_t at_transport_send(const char *str);

/* 发送原始字节（用于 QMTPUBEX 的 ">" 提示后负载数据）。 */
at_status_t at_send_bytes(const uint8_t *data, uint16_t len);

/* 发送一条 AT 命令（自动补 \r），读取回复直至 OK/ERROR 或超时。
 * resp 用于收集非终结行（可传 NULL）。timeout_ms 为命令整体超时。 */
at_status_t at_send_cmd(const char *cmd, char *resp, uint16_t resp_max, uint32_t timeout_ms);

/* 判断 resp 中是否包含子串 token。 */
bool at_resp_contains(const char *resp, const char *token);

/* 阻塞等待以 prefix 开头的行（prefix 为 NULL 表示任意行），
 * 返回该行到 line。timeout_ms 为整体超时。用于捕获 QMTOPEN/QMTCONN 等异步结果 URC。 */
at_status_t at_wait_line(char *line, uint16_t max, const char *prefix, uint32_t timeout_ms);

/* 尝试从当前串口读取一行（非阻塞，短超时），用于 MQTT URC 轮询。 */
at_status_t at_poll_line(char *line, uint16_t max, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* AT_CORE_H */
