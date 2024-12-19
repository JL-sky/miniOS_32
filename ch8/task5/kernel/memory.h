#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H

#include "stdint.h"
#include "bitmap.h"

/*管理虚拟内存池的数据结果*/
struct virtual_addr
{
    struct bitmap vaddr_bitmap; // 管理虚拟内存池的位图
    uint32_t vaddr_start;       // 虚拟内存池的起始地址
};

/*管理物理内存池的数据结构*/
struct pool
{
    struct bitmap pool_bitmap; // 管理物理内存池的位图
    uint32_t phy_addr_start;   // 物理内存池的起始地址
    uint32_t pool_size;        // 物理内存池的大小
};
void mem_init(void);

#define PG_P_1 1  // 页表项或页目录项存在属性位
#define PG_P_0 0  // 页表项或页目录项存在属性位
#define PG_RW_R 0 // R/W 属性位值, 读/执行
#define PG_RW_W 2 // R/W 属性位值, 读/写/执行
#define PG_US_S 0 // U/S 属性位值, 系统级
#define PG_US_U 4 // U/S 属性位值, 用户级

enum pool_flags
{
    PF_KERNEL = 1,
    PF_USER = 2
};
uint32_t *pde_ptr(uint32_t vaddr);
uint32_t *pte_ptr(uint32_t vaddr);
void *malloc_page(enum pool_flags pf, uint32_t pg_cnt);
void *get_kernel_pages(uint32_t pg_cnt);
void malloc_init(void);

#endif