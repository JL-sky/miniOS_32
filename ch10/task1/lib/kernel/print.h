#ifndef __LIB_KERNEL_PRINT_H
#define __LIB_KERNEL_PRINT_H
#include "stdint.h"               //stdint.h中定义了数据类型
void put_char(uint8_t char_asci); // 在stdint.h中uint8_t得到了定义，就是unsigned char
void put_str(char *message);
void put_int(uint32_t num); // 以16进制打印
void set_cursor(uint32_t cursor_pos);
#endif