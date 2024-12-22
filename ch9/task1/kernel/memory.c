#include "memory.h"
#include "print.h"
#include "debug.h"
#include "string.h"

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
    一张页表占据4KB，共256张页表（一张页目录表+0号页目录项和768号页目录项共同指向的一张页表+769~1022号页表项指向的254张页表）
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
    kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;
    kernel_vaddr.vaddr_bitmap.bits = (void *)(MEM_BITMAP_BASE + kbm_length + ubm_length);
    kernel_vaddr.vaddr_start = K_HEAP_START;
    bitmap_init(&kernel_vaddr.vaddr_bitmap);
    put_str("memory pool init done!\n");
}
/*
在pf所指向的虚拟内存池中寻找pg_cnt空闲虚拟页，并分配之
*/
static void *vaddr_get(enum pool_flags pf, uint32_t pg_cnt)
{
    int vaddr_start = 0, bit_idx_start = -1;
    if (pf == PF_KERNEL)
    {
        /*内核虚拟内存池*/
        // 寻找可用的连续页
        bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
        if (bit_idx_start == -1)
            return NULL;
        // 如果找到就将这些连续空间设置为已分配
        uint32_t cnt = 0;
        while (cnt < pg_cnt)
            bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1);
        vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
    }
    else
    {
        /*用户虚拟内存池*/
    }
    return (void *)vaddr_start;
}
/*
在m_pool(物理内存池数据结构)所指向的物理内存池中寻找一页空闲的物理页，并分配之
*/
static void *palloc(struct pool *m_pool)
{
    int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);
    if (bit_idx == -1)
        return NULL;
    bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
    uint32_t page_phyaddr = ((bit_idx * PG_SIZE) + m_pool->phy_addr_start);
    return (void *)page_phyaddr;
};

// 取出虚拟地址的高10位
#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)
// 取出虚拟地址的中间10位
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)
/*
返回虚拟地址vaddr所代表的pte（页表项）的虚拟地址
此举是为了取出虚拟地址vaddr所代表的页表项的内容
*/
uint32_t *pte_ptr(uint32_t vaddr)
{
    /*
    0xffc00000：表示页目录表的第1023个页目录项，该页目录项的存储的是页目录表的物理地址(见loader.S中页目录项的初始化部分内容)
    vaddr & 0xffc00000：取出vaddr的高10位，也就是vaddr所代表的页目录项的索引
     先访问到页目录表自己 + 再用页目录项pde(页目录内页表的索引)作为pte的索引访问到页表 + 再用pte的索引做为页内偏移
    */
    uint32_t *pte = (uint32_t *)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);
    return pte;
}
/*
返回虚拟地址vaddr所代表的pde（页目录项）的虚拟地址
此举是为了取出虚拟地址vaddr所代表的页目录项的内容
*/
uint32_t *pde_ptr(uint32_t vaddr)
{
    /* 0xfffff是用来访问到页表本身所在的地址 */
    uint32_t *pde = (uint32_t *)((0xfffff000) + PDE_IDX(vaddr) * 4);
    return pde;
}
/*
页表中添加虚拟地址_vaddr和物理地址_page_phyaddr的映射关系
*/
static void page_table_add(void *_vaddr, void *_page_phyaddr)
{
    uint32_t vaddr = (uint32_t)_vaddr, page_phyaddr = (uint32_t)_page_phyaddr;
    uint32_t *pde = pde_ptr(vaddr);
    uint32_t *pte = pte_ptr(vaddr);
    /* 判断页目录内目录项的P位，若为1,则表示其指向的页表已存在，直接装填对应的页表项*/
    if (*pde & 0x00000001)
    {
        // 如果页目录项P位为1，说明页目录项指向的页表存在，但是页表项不存在，则直接创建页表项
        if (!(*pte & 0x00000001))
        {
            // 填充页目录项
            *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        }
        else
        {
            PANIC("pte repeat");
            *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        }
    }
    else
    {
        /*
        页目录项指向的页表不存在，需要首先构建页表，然后再装填对应的页目录项和页表项
        1.在物理内存池中寻找一页物理页分配给页表
        2.将该页表初始化为0，防止这块内存的脏数据乱入
        3.将该页表写入页目录项
        4.构建页表项，填充进页表
        */
        // 在内核物理内存池找一页物理页分配给页表
        uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);
        // 初始化页表清零
        /* 访问到pde对应的物理地址,用pte取高20位便可.
         * 因为pte是基于该pde对应的物理地址内再寻址,
         * 把低12位置0便是该pde对应的物理页的起始*/
        memset((void *)((int)pte & 0xfffff000), 0, PG_SIZE);
        // 将分配的页表物理地址写入页目录项
        *pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
        ASSERT(!(*pte & 0x00000001));
        // 构建页表项，填充进页表
        *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
    }
}
/*
分配pg_cnt个页空间，成功也返回起始虚拟地址，失败则返回NULL
*/
void *malloc_page(enum pool_flags pf, uint32_t pg_cnt)
{
    /***********   malloc_page的原理是三个动作的合成:   ***********
      1通过vaddr_get在虚拟内存池中申请虚拟地址
      2通过palloc在物理内存池中申请物理页
      3通过page_table_add将以上得到的虚拟地址和物理地址在页表中完成映射
    ***************************************************************/
    ASSERT(pg_cnt > 0 && pg_cnt < 3840);
    // 获取连续的pg_cnt个虚拟页
    void *vaddr_start = vaddr_get(pf, pg_cnt);
    if (vaddr_start == NULL)
        return NULL;
    uint32_t vaddr = (uint32_t)vaddr_start, cnt = pg_cnt;
    struct pool *mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;
    /*
    逐一申请物理页，然后再挨个映射进虚拟页
    之所以物理页不能一次性申请，是因为物理页可能是离散的
    但是虚拟页是连续的
    */
    while (cnt--)
    {
        void *page_phyaddr = palloc(mem_pool);
        if (page_phyaddr == NULL)
            return NULL;
        page_table_add((void *)vaddr, page_phyaddr);
        vaddr += PG_SIZE;
    }
    return vaddr_start;
}
/*
从内核物理内存池中申请pg_cnt页内存,成功则返回其虚拟地址,失败则返回NULL
*/
void *get_kernel_pages(uint32_t pg_cnt)
{
    void *vaddr = malloc_page(PF_KERNEL, pg_cnt);
    if (vaddr)
        memset(vaddr, 0, pg_cnt * PG_SIZE);
    return vaddr;
}
/*
内存管理初始化入口
*/
void mem_init(void)
{
    put_str("mem_init start\n");
    // 此处存放着物理内存容量，具体可见/boot/loader.S中total_mem_bytes的定义和计算
    uint32_t mem_bytes_total = (*(uint32_t *)(0xb00));
    // 初始化内存池
    mem_pool_init(mem_bytes_total);
    put_str("mem_init done\n");
}
