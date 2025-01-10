#ifndef __DEVICE_IDE_H
#define __DEVICE_IDE_H
#include "stdint.h"
#include "sync.h"
#include "bitmap.h"
/*分区结构*/
struct partition
{
    // 起始扇区
    uint32_t start_lba;
    // 扇区数
    uint32_t sec_cnt;
    // 分区所属的磁盘
    struct disk *my_disk;
    // 队列节点标记，用于将分区形成链表进行管理
    struct list_elem part_tag;
    // 分区名称
    char name[8];
    // 本分区的超级块
    struct super_block *sb;
    // 块位图
    struct bitmap block_bitmap;
    // inode节点位图
    struct bitmap inode_bitmap;
    // 本分区打开的inode节点队列
    struct list open_inodes;
}

/*硬盘结构*/
struct disk
{
    // 磁盘名称
    char name[8];
    // 表示主盘还是从盘
    uint8_t dev_no;
    // 本磁盘所在的通道
    struct ide_channel *my_channel;
    // 磁盘的主分区，最多四个
    struct partition prim_parts[4];
    // 磁盘的逻辑分区，理论上数量无限，我们这里设定8个
    struct partition logic_parts[8];
};
/* ata通道结构 */
struct ide_channel
{
    // ata通道名称
    char name[8];
    // 本通道的起始端口号（书p126）
    uint16_t port_base;
    // 本通道所用的中断号
    uint8_t irq_no;
    // ata通道有两个，这两个通道共用一个接口，故而每次访问都需要互斥进行
    struct lock lock;
    // 等待硬盘的中断
    bool expecting_intr;
    // 用于阻塞、唤醒驱动程序
    struct semaphore disk_done;
    // 一个通道上连接两个硬盘，一主一从
    struct disk devices[2];
};
void ide_init(void);
extern uint8_t channel_cnt;
extern struct ide_channel channels[];
#endif