#include "string.h"
#include "global.h"
#include "debug.h"
/*内存初始化函数，将dst_起始的size个字节的初始化为value*/
void memset(void *dst_, uint8_t value, uint32_t size)
{
    ASSERT(dst_ != NULL);
    uint8_t *dst = (uint8_t *)dst_;
    /*
    先执行*dst=value
    然后执行dst++
    */
    while (size--)
        *dst++ = value;
}

/*
将src_起始处的size个字节的内容拷贝到dst_
*/
void memcpy(void *dst_, const void *src_, uint32_t size)
{
    ASSERT(dst_ != NULL && src_ != NULL);
    uint8_t *src = (uint8_t *)src_;
    uint8_t *dst = (uint8_t *)dst_;
    while (size--)
    {
        *dst++ = *src++;
    }
}

/*
比较两个地址的起始size字节的数据是否相等
相等则返回0
如果不相等，则比较第一个不相等的字节数据
*/
int memcmp(const void *a_, const void *b_, uint32_t size)
{
    ASSERT(a_ != NULL && b_ != NULL);
    const char *a = a_;
    const char *b = b_;
    while (size--)
    {
        if (*a != *b)
        {
            return *a > *b ? 1 : -1;
        }
        a++;
        b++;
    }
    return 0;
}

/*
字符串拷贝
*/
char *strcpy(char *dst_, const char *src_)
{
    ASSERT(dst_ != NULL && src_ != NULL);
    char *res = dst_;
    while ((*dst_++ = *src_++))
        ;
    return res;
}

/*
返回字符串的长度
*/
uint32_t strlen(const char *str)
{
    ASSERT(str != NULL);
    const char *p = str;
    while (*p++)
        ;
    return (p - str - 1);
}

/*
比较两个字符串
    若a_中的字符与b_中的字符全部相同，则返回0
    如果不同，则比较第一个不同的字符
        如果a_>b_返回1，反之返回-1
*/
int8_t strcmp(const char *str1, const char *str2)
{
    ASSERT(str1 != NULL && str2 != NULL);
    // 比较两个字符串
    while (*str1 != '\0' && *str2 != '\0')
    {
        if (*str1 != *str2)
        {
            return (*str1 < *str2) ? -1 : 1;
        }
        ++str1;
        ++str2;
    }

    // 如果两个字符串走到末尾还是没有不同，比较它们的结束符
    return (*str1 == *str2) ? 0 : (*str1 < *str2 ? -1 : 1);
}

/*
从左到右查找字符串str中首次出现字符ch的地址
const char *str，表示str指向的字符串内容不可改变
但是str指针值是可以改变的
*/
char *strchr(const char *str, const uint8_t ch)
{
    ASSERT(str != NULL);
    while (*str != 0)
    {
        if (*str == ch)
            return (char *)str;
        ++str;
    }
    return NULL;
}

/*
从后往前查找字符串str中首次出现字符ch的地址(不是下标,是地址)
*/

char *strrchr(const char *str, const uint8_t ch)
{
    ASSERT(str != NULL);
    const char *last_char = NULL;
    /* 从头到尾遍历一次,若存在ch字符,last_char总是该字符最后一次出现在串中的地址(不是下标,是地址)*/
    while (*str != 0)
    {
        if (*str == ch)
        {
            last_char = str;
        }
        str++;
    }
    return (char *)last_char;
}

/* 将字符串src_拼接到dst_后,将回拼接的串地址 */
char *strcat(char *dst_, const char *src_)
{
    ASSERT(dst_ != NULL && src_ != NULL);
    char *p = dst_;
    while (*p++)
        ;
    --p;
    while ((*p++ = *src_++))
        ;
    return dst_;
}
/* 在字符串str中查找指定字符ch出现的次数 */
uint32_t strchrs(const char *str, uint8_t ch)
{
    ASSERT(str != NULL);
    uint32_t cnt = 0;
    const char *p = str;
    while (*p != 0)
    {
        if (*p == ch)
            cnt++;
        ++p;
    }
    return cnt;
}
