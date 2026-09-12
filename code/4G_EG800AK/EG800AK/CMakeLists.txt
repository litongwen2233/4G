add_library(es800ka STATIC
    EG800AK.c
    MQTT_client.c
)
target_include_directories(es800ka PUBLIC ${CMAKE_CURRENT_BINARY_DIR})