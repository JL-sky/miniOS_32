#ifndef __LIB_USER_SYSCALL_H
#define __LIB_USER_SYSCALL_H
#include "stdint.h"
/*定义系统调用号*/
enum SYSCALL_NR
{
    SYS_GETPID
};
uint32_t getpid(void);

#endif