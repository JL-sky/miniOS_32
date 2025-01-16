#ifndef __FS_SUPER_BLOCK_H
#define __FS_SUPER_BLOCK_H

#include "stdint.h"

/*超级块*/
struct super_block
{
    // 标识文件系统
    uint32_t magic;
    // 本分区的起始lba地址
    uint32_t part_lba_base;
    // 本分区的总共扇区数
    uint32_t sec_cnt;

    // 分区中inode数量
    uint32_t inode_cnt;

    /*块位图，用于管理数据块*/
    // 块位图的lba地址
    uint32_t block_bitmap_lba;
    // 块位图占据的扇区数
    uint32_t block_bitmap_sects;

    /*inode位图，用于管理inode，一个inode对应一个文件*/
    // inode位图的lba地址
    uint32_t inode_bitmap_lba;
    // inode位图占据的扇区数
    uint32_t inode_bitmap_sects;

    // inode数组的lba地址
    uint32_t inode_table_lba;
    // inode数组占据的扇区数
    uint32_t inode_table_sects;

    // 数据区的第一个扇区号
    uint32_t data_start_lba;
    // 根目录的inode号
    uint32_t root_inode_no;
    // 目录项的大小
    uint32_t dir_entry_size;

    // 填充字段，让超级块占据完整的一个块（扇区）的大小
    uint8_t pad[460];
} __attribute__((packed));
#endif