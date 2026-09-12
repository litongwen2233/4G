cmake_minimum_required(VERSION 3.10)
project(quectel_4g_mqtt_driver C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(MSVC)
    # /utf-8: 源码含中文注释时避免 C4819；_CRT_SECURE_NO_WARNINGS: 屏蔽 strcat/sscanf 等“不安全”告警
    add_compile_options(/W4 /utf-8)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra)
endif()

# 驱动核心库：AT 内核 + 4G 建链 + MQTT 客户端 + 传输层模板
# 注意：pc_sim.c 仅用于 PC 演示（虚拟模组），接真机时从本列表删除并自行实现
#       uart_recv_byte / uart_send_byte / sys_tick_ms / sys_delay_ms 四个接口。
add_library(fourg_mqtt_driver STATIC
    at_core.c
    fourg_net.c
    mqtt_client.c
    transport_template.c
    pc_sim.c
)

target_include_directories(fourg_mqtt_driver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 示例可执行程序（提供 UART/tick 桩实现，演示完整调用流程）
add_executable(example example.c)
target_link_libraries(example PRIVATE fourg_mqtt_driver)
