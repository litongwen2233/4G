#ifndef __WINDOW_SERIAL_H__
#define __WINDOW_SERIAL_H__

#include<windows.h>
#include <stdint.h>
#include <stdbool.h>

void Sertial_open(char *serial_name, DWORD baud_rate,char *Write_buff,char *Read_buff);

void Serial_write(uint16_t len);

void Serial_read(void);

#endif