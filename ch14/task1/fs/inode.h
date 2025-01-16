#ifndef __FS_INODE_H
#define __FS_INODE_H

#include "stdint.h"
#include "list.h"

/*inode结构*/
struct inode
{
    // inode编号
    uint32_t i_no;

    /* 当此inode是文件时,i_size是指文件大小,
    若此inode是目录,i_size是指该目录下所有目录项大小之和*/
    uint32_t i_size;
    // 记录此文件被打开的次数
    uint32_t i_open_cnts;
    // 写文件不能并行,进程写文件前检查此标识，类似于锁机制
    bool write_deny;

    /*
    i_sectors[0-11]是直接块,
    i_sectors[12]用来存储一级间接块指针
    */
    uint32_t i_sectors[13];
    // 将文件内容读取进内存之后使用双链表进行组织
    struct list_elem inode_tag;
};
#endif