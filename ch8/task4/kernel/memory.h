#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H

#include "stdint.h"
#include "bitmap.h"

/*管理虚拟内存池的数据结果*/
struct virtual_addr
{
    struct bitmap vaddr_map; // 管理虚拟内存池的位图
    uint32_t vaddr_start;    // 虚拟内存池的起始地址
};

/*管理物理内存池的数据结构*/
struct pool
{
    struct bitmap pool_bitmap; // 管理物理内存池的位图
    uint32_t phy_addr_start;   // 物理内存池的起始地址
    uint32_t pool_size;        // 物理内存池的大小
};
void mem_init();

#endif