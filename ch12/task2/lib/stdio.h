#ifndef __LIB_STDIO_H
#define __LIB_STDIO_H

#include "stdint.h"
typedef char *va_list;
// 按照format格式解析字符串，并传出str
uint32_t vsprintf(char *str, const char *format, va_list ap);
// 将解析后的字符串通过系统调用打印到屏幕上
uint32_t printf(const char *str, ...);

#endif