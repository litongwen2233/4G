add_library(window_serial STATIC
    Window_serial.c
)
target_include_directories(window_serial PUBLIC ${CMAKE_CURRENT_BINARY_DIR})