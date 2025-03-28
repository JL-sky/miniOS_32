#ifndef __FS_FILE_H
#define __FS_FILE_H
#include "stdint.h"
#include "inode.h"
#include "dir.h"

// 系统可打开的最大文件数
#define MAX_FILE_OPEN 32

/* 文件结构 */
struct file
{
    uint32_t fd_pos; // 记录当前文件操作的偏移地址,以0为起始,最大为文件大小-1
    uint32_t fd_flag;
    struct inode *fd_inode;
};

/* 位图类型 */
enum bitmap_type
{
    INODE_BITMAP, // inode位图
    BLOCK_BITMAP  // 块位图
};

int32_t get_free_slot_in_global(void);
int32_t pcb_fd_install(int32_t globa_fd_idx);
int32_t inode_bitmap_alloc(struct partition *part);
int32_t block_bitmap_alloc(struct partition *part);
void bitmap_sync(struct partition *part, uint32_t bit_idx, uint8_t btmp_type);
int32_t file_create(struct dir *parent_dir, char *filename, uint8_t flag);
extern struct file file_table[MAX_FILE_OPEN];
#endif
