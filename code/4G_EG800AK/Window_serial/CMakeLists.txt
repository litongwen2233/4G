add_library(window_serial STATIC
    Serial.c
)
target_include_directories(window_serial PUBLIC ${CMAKE_CURRENT_BINARY_DIR})