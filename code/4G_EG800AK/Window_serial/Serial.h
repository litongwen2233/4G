#ifndef __WINDOW_SERIAL_H__
#define __WINDOW_SERIAL_H__

#include<windows.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

bool Sertial_open(char *serial_name, DWORD baud_rate,char *Write_buff,char *Read_buff);

int Serial_write(uint16_t len);

int Serial_read(void);

#endif