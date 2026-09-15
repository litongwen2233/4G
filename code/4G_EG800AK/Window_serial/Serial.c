#include"Serial.h"
#include <stdio.h>

/* 全局变量定义 */
HANDLE g_hSerial = INVALID_HANDLE_VALUE;
char *g_Write_buff = NULL;
char *g_Read_buff = NULL;
volatile uint16_t g_Read_len = 0;

/* =========================================================
 * 函数：Sertial_open
 * 功能：打开串口并配置参数（115200,8,N,1）
 * ========================================================= */
bool Sertial_open(char *serial_name, DWORD baud_rate, char *Write_buff, char *Read_buff)
{
    DCB dcbParams = {0};
    COMMTIMEOUTS timeouts = {0};

    /* 保存外部缓冲区指针 */
    g_Write_buff = Write_buff;
    g_Read_buff  = Read_buff;
    g_Read_len   = 0;

    /* 1. 打开串口（COM10+ 需要 \\.\ 前缀） */
    if (serial_name[0] != '\\') /* 简单判断，统一转换为 \\.\COMx 格式 */
    {
        char full_name[32] = "\\\\.\\";
        strncat(full_name, serial_name, sizeof(full_name) - 6);
        serial_name = full_name;
    }

    g_hSerial = CreateFileA(
        serial_name,
        GENERIC_READ | GENERIC_WRITE,
        0, NULL,
        OPEN_EXISTING,
        0, NULL
    );

    if (g_hSerial == INVALID_HANDLE_VALUE) {
        printf("[SERIAL] 打开失败，错误码: %lu\n", GetLastError());
        return false;
    }

    /* 2. 配置串口参数 */
    dcbParams.DCBlength = sizeof(dcbParams);
    if (!GetCommState(g_hSerial, &dcbParams)) {
        printf("[SERIAL] GetCommState 失败\n");
        CloseHandle(g_hSerial);
        g_hSerial = INVALID_HANDLE_VALUE;
        return false;
    }

    dcbParams.BaudRate = baud_rate;
    dcbParams.ByteSize = 8;
    dcbParams.Parity   = NOPARITY;
    dcbParams.StopBits = ONESTOPBIT;
    dcbParams.fOutxCtsFlow = FALSE;   /* 禁用硬件流控 */
    dcbParams.fRtsControl = RTS_CONTROL_DISABLE;

    if (!SetCommState(g_hSerial, &dcbParams)) {
        printf("[SERIAL] SetCommState 失败\n");
        CloseHandle(g_hSerial);
        g_hSerial = INVALID_HANDLE_VALUE;
        return false;
    }

    /* 3. 设置超时（关键：防止 ReadFile 阻塞） */
    timeouts.ReadIntervalTimeout         = 10;    /* 10ms 无数据则超时 */
    timeouts.ReadTotalTimeoutConstant    = 50;    /* 固定 50ms 超时 */
    timeouts.ReadTotalTimeoutMultiplier  = 5;     /* 每字节乘数 */
    timeouts.WriteTotalTimeoutConstant   = 100;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    SetCommTimeouts(g_hSerial, &timeouts);

    /* 4. 清空缓冲区 */
    PurgeComm(g_hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);

    printf("[SERIAL] %s 已打开，波特率 %lu\n", serial_name, baud_rate);
    return true;
}

/* =========================================================
 * 函数：Serial_write
 * 功能：将 g_Write_buff 中的数据写入串口设备
 * 输入：len —— 要从 g_Write_buff 中发送的字节数
 * ========================================================= */
int Serial_write(uint16_t len)
{
    DWORD bytes_written = 0;

    if (g_hSerial == INVALID_HANDLE_VALUE || g_Write_buff == NULL) {
        return -1;
    }

    if (!WriteFile(g_hSerial, g_Write_buff, len, &bytes_written, NULL)) {
        return -1;
    }
    printf("[SERIAL] 当前写入内容: %s,写入长度%d\n", g_Write_buff, (int)bytes_written);
    return (int)bytes_written;
}

/* =========================================================
 * 函数：Serial_read
 * 功能：从串口读取数据存入 g_Read_buff，并更新 g_Read_len
 * 建议：可在定时中断或主循环中循环调用
 * ========================================================= */
int Serial_read(void)
{
    DWORD bytes_read = 0;

    if (g_hSerial == INVALID_HANDLE_VALUE || g_Read_buff == NULL) {
        return 0;
    }

    /* 从串口读取数据，最多 255 字节（防止溢出） */
    BOOL success = ReadFile(g_hSerial, g_Read_buff, 255, &bytes_read, NULL);

    if (success && bytes_read > 0) {
        g_Read_len = (uint16_t)bytes_read;   /* 更新全局长度 */
        printf("[SERIAL] 当前读取内容: %s,读取长度%d\n", g_Read_buff, g_Read_len);
        return (int)bytes_read;
    }
    
    g_Read_len = 0;
    return 0;
}