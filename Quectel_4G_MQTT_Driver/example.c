#include "fourg_net.h"
#include "mqtt_client.h"
#include "pc_sim.h"
#include <stdio.h>

void at_transport_install(void);   /* transport_template.c 提供 */

/* ---------- MQTT 回调：与 AT 收发区分，使用 [MSG] 前缀 ---------- */
static void on_message(uint8_t idx, uint16_t msgid,
                      const char *topic, const char *payload)
{
    printf("[MSG] client=%d msgid=%u topic=%s payload=%s\r\n", idx, msgid, topic, payload);
}
static void on_state(uint8_t idx, int err_code)
{
    printf("[MSG] client=%d state changed, err=%d\r\n", idx, err_code);
}
static void on_ping(uint8_t idx, int result)
{
    printf("[MSG] client=%d ping result=%d\r\n", idx, result);
}

int main(void)
{
    at_transport_install();

    /* ===== 1) 4G 数据通道建立：CPIN -> CREG/CGREG -> CGATT -> CGDCONT -> CGACT ===== */
    printf("================== 4G 通信建立流程 ==================\r\n");
    fourg_pdp_t pdp = {
        .cid = 1,
        .apn = "CMNET",          /* 按运营商修改：移动 CMNET / 联通 UNINET / 电信 CTNET */
        .username = "",
        .password = "",
        .pdp_type = "IP"
    };
    if (fourg_init(&pdp, 180000) != FOURG_OK) {
        printf("[ERR] 4G 网络建立失败\r\n");
        return -1;
    }
    printf("[OK] 4G 已附着并激活 PDP(cid=%d)\r\n", pdp.cid);

    /* ===== 2) 在 4G 通道之上建立 MQTT 连接 ===== */
    printf("================== MQTT 通信建立流程 ==================\r\n");
    mqtt_client_t cli = {
        .client_idx = 0,
        .pdp_cid    = pdp.cid,
        .on_message = on_message,
        .on_state   = on_state,
        .on_ping    = on_ping
    };
    mqtt_register(&cli);

    mqtt_configure(&cli, true, true, 60, 1, 5, 3);

    if (mqtt_open(&cli, "mqtt.example.com", 1883) != MQTT_OK) {
        printf("[ERR] MQTT 打开网络失败\r\n");
        return -1;
    }
    if (mqtt_connect(&cli, "clientExample", NULL, NULL) != MQTT_OK) {
        printf("[ERR] MQTT 连接服务器失败\r\n");
        return -1;
    }
    mqtt_subscribe(&cli, 1, "topic/example", 0);
    mqtt_publish(&cli, 0, 0, 0, "topic/pub", "hello MQTT");
    printf("[OK] MQTT 已连接，完成订阅与发布\r\n");

    /* ===== 3) 模拟服务器下行一条消息，观察 URC 解析与回调（仅 PC 演示） ===== */
    printf("================== 模拟服务器下行消息 ==================\r\n");
    pc_sim_inject_recv("+QMTRECV: 0,0,\"topic/example\",36,\"This is the payload related to topic\"");

    /* ===== 4) 主循环：周期性轮询处理下行 MQTT 消息与链路 URC ===== */
    printf("================== 进入主循环 (mqtt_poll) ==================\r\n");
    for (int i = 0; i < 3; i++) {
        mqtt_poll(100);          /* 实际工程中放在定时器或主循环 */
    }
    printf("[DONE] 流程演示结束\r\n");
    return 0;
}
