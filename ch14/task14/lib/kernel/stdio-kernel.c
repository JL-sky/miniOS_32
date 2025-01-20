#include "stdio-kernel.h"
#include "stdio.h"
#include "console.h"
#include "global.h"

#define va_start(args, first_fix) args = (va_list) & first_fix
#define va_end(args) args = NULL

/* 供内核使用的格式化输出函数 */
void printk(const char *format, ...)
{
    va_list args;
    // 定义固定参数
    va_start(args, format);
    // 定义变换后的字符串接受缓冲
    char buf[1024] = {0};
    // 按照format格式将固定参数args填充进去，然后返回给buf
    vsprintf(buf, format, args);
    va_end(args);
    console_put_str(buf);
}