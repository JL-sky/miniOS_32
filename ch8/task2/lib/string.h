#ifndef __LIB_STRING_H
#define __LIB_STRING_H
#include "stdint.h"
/*内存初始化函数，将dst_起始的size个字节的初始化为value*/
void memset(void *dst_, uint8_t value, uint32_t size);
/*内存拷贝函数，将src_起始处的size个字节的内容拷贝到dst_*/
void memcpy(void *dst_, const void *src_, uint32_t size);
/*
比较两个地址的起始size字节的数据是否相等
相等则返回0
如果不相等，则比较第一个不相等的字节数据
*/
int memcmp(const void *a_, const void *b_, uint32_t size);
/*字符串拷贝*/
char *strcpy(char *dst_, const char *src_);
/*获取字符串的长度*/
uint32_t strlen(const char *str);
/*
比较两个字符串
    若a_中的字符与b_中的字符全部相同，则返回0
    如果不同，则比较第一个不同的字符
        如果a_>b_返回1，反之返回-1
*/
int8_t strcmp(const char *str1, const char *str2);
/*从左到右查找字符串str中首次出现字符ch的地址*/
char *strchr(const char *string, const uint8_t ch);
/*从后往前查找字符串str中首次出现字符ch的地址(不是下标,是地址)*/
char *strrchr(const char *string, const uint8_t ch);
/* 将字符串src_拼接到dst_后,将回拼接的串地址 */
char *strcat(char *dst_, const char *src_);
/* 在字符串str中查找指定字符ch出现的次数 */
uint32_t strchrs(const char *filename, uint8_t ch);
#endif