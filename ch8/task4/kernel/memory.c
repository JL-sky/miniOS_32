#include "memory.h"
#include "print.h"

#define PG_SIZE 4096

/*虚拟内存池的位图在虚拟内存中的起始地址*/
#define MEM_BITMAP_BASE 0xc009a000

/*内核堆区的起始地址，堆区其实就是虚拟内存池*/
#define K_HEAP_START 0xc0100000

/*定义内核的虚拟内存池*/
struct virtual_addr kernel_vaddr;
/*定义内核的物理内存池和用户的物理内存池*/
struct pool kernel_pool, user_pool;

/*
初始化内存池
    参数：物理内存的所有容量，为32MB
    该容量的数值存储在物理内存的0xb00处
*/
static void mem_pool_init(uint32_t all_mem)
{
    put_str("memory pool init start!\n");
    /*
    所有内核页表占据的物理内存
    一张页表占据4KB，共256张页表（一张页目录表+0号页表项和768号页表项共同指向的一张页表+769~1022号页表项指向的254张页表）
    */
    uint32_t page_table_size = PG_SIZE * 256;
    // 目前已经使用的物理内存：0~1MB的内核空间+页表占据的空间
    uint32_t used_mem = 0x100000 + page_table_size;
    // 当前所有可用的物理内存
    uint32_t free_mem = all_mem - used_mem;
    // 当前所有可用的物理页数
    uint16_t all_free_pages = free_mem / PG_SIZE;
    // 设置所有可用的内核物理页（物理内存池）
    uint16_t kernel_free_pages = all_free_pages / 2;
    // 设置所有可用的用户物理页（用户内存池）
    uint16_t user_free_pages = all_free_pages - kernel_free_pages;

    /*
    定义管理内核物理内存池的位图长度
    位图中一个比特位管理一页物理内存，故用字节表示位图长度除以8即可
    */
    uint32_t kbm_length = kernel_free_pages / 8;
    // 定义管理用户物理内存池的位图长度
    uint32_t ubm_length = user_free_pages / 8;

    // 内核物理内存池的起始地址
    uint32_t kp_start = used_mem;
    // 用户物理内存池的起始地址
    uint32_t up_start = kp_start + kernel_free_pages * PG_SIZE;

    /******************** 初始化物理内存池 **********************/
    /*以下初始化内核物理内存池*/
    kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;
    kernel_pool.pool_bitmap.bits = (void *)MEM_BITMAP_BASE;
    kernel_pool.phy_addr_start = kp_start;
    kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
    /*以下初始化用户物理内存池*/
    user_pool.pool_bitmap.btmp_bytes_len = ubm_length;
    user_pool.pool_bitmap.bits = (void *)(MEM_BITMAP_BASE + kbm_length);
    user_pool.phy_addr_start = up_start;
    user_pool.pool_size = user_free_pages * PG_SIZE;

    /******************** 输出物理内存池信息 **********************/
    put_str("      kernel_pool_bitmap_start:");
    put_int((int)kernel_pool.pool_bitmap.bits);
    put_str(" kernel_pool_phy_addr_start:");
    put_int(kernel_pool.phy_addr_start);
    put_str("\n");
    put_str("      user_pool_bitmap_start:");
    put_int((int)user_pool.pool_bitmap.bits);
    put_str(" user_pool_phy_addr_start:");
    put_int(user_pool.phy_addr_start);
    put_str("\n");
    /*将位图置为0，初始化位图*/
    bitmap_init(&kernel_pool.pool_bitmap);
    bitmap_init(&user_pool.pool_bitmap);

    /******************** 初始化虚拟内存池 **********************/
    /*初始化内核虚拟内存池*/
    kernel_vaddr.vaddr_map.btmp_bytes_len = kbm_length;
    kernel_vaddr.vaddr_map.bits = (void *)(MEM_BITMAP_BASE + kbm_length + ubm_length);
    kernel_vaddr.vaddr_start = K_HEAP_START;
    bitmap_init(&kernel_vaddr.vaddr_map);
    put_str("memory pool init done!\n");
}

void mem_init()
{
    put_str("mem_init start\n");
    // 此处存放着物理内存容量，具体可见/boot/loader.S中total_mem_bytes的定义和计算
    uint32_t mem_bytes_total = (*(uint32_t *)(0xb00));
    // 初始化内存池
    mem_pool_init(mem_bytes_total);
    put_str("mem_init done\n");
}
