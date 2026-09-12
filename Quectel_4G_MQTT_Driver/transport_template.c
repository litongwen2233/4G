#include "at_core.h"
#include <string.h>

/* ===========================================================================
 * 传输层模板：将 at_transport_t 对接到 MCU 的 UART。
 * 用户需根据所用平台（STM32 HAL / ESP-IDF / 裸机）实现下方 4 个底层接口：
 *   uart_recv_byte()  - 非阻塞读取 UART 一字节，无数据返回 -1
 *   uart_send_byte()  - 向 UART 发送一字节
 *   sys_tick_ms()     - 返回单调毫秒时钟（驱动超时控制依赖它）
 *   sys_delay_ms()    - 毫秒延时（轮询间隔使用）
 * 并在 UART 接收中断中调用 uart_ring_put() 把收到的字节存入环形缓冲。
 * ========================================================================= */

extern int  uart_recv_byte(void);
extern void uart_send_byte(uint8_t b);
extern uint32_t sys_tick_ms(void);
extern void sys_delay_ms(uint32_t ms);

#define RING_SZ 1024
static uint8_t  g_ring[RING_SZ];
static volatile uint16_t g_head = 0, g_tail = 0;

/* 在 UART 接收中断里调用，把一字节放入环形缓冲。 */
void uart_ring_put(uint8_t b)
{
    uint16_t next = (uint16_t)((g_head + 1) % RING_SZ);
    if (next != g_tail) {
        g_ring[g_head] = b;
        g_head = next;
    }
}

static int tp_send(const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) uart_send_byte(data[i]);
    return (int)len;
}

static int tp_recv(uint8_t *data, uint16_t len, uint32_t timeout_ms)
{
    uint32_t t0 = sys_tick_ms();
    uint16_t got = 0;
    while (got < len) {
        int b = uart_recv_byte();
        if (b >= 0) { data[got++] = (uint8_t)b; continue; }
        if (sys_tick_ms() - t0 > timeout_ms) break;
    }
    return (int)got;
}

static uint32_t tp_tick(void)  { return sys_tick_ms(); }
static void     tp_delay(uint32_t ms) { sys_delay_ms(ms); }

static const at_transport_t g_tp = { tp_send, tp_recv, tp_tick, tp_delay };

void at_transport_install(void)
{
    at_set_transport(&g_tp);
}
