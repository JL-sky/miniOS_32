#include "dir.h"
#include "inode.h"
#include "super_block.h"
#include "stdio-kernel.h"
#include "string.h"
#include "debug.h"
#include "file.h"

// 根目录
struct dir root_dir;

/*打开根目录*/
void open_root_dir(struct partition *part)
{
    root_dir.inode = inode_open(part, part->sb->root_inode_no);
    root_dir.dir_pos = 0;
}

/*在分区part上打开inode号为inode_no的目录并返回*/
struct dir *dir_open(struct partition *part, uint32_t inode_no)
{
    struct dir *pdir = (struct dir *)sys_malloc(sizeof(struct dir));
    pdir->inode = inode_open(part, inode_no);
    pdir->dir_pos = 0;
    return pdir;
}

/* 在part分区内的pdir目录内寻找名为name的文件或目录,
 * 找到后返回true并将其目录项存入dir_e,否则返回false */
bool search_dir_entry(struct partition *part, struct dir *pdir, const char *name, struct dir_entry *dir_e)
{
    /*
    all_blocks用于存储所有块（直接块+间接块）的地址
    12个直接块大小+128个间接块,共560字节
    */
    uint32_t *all_blocks = (uint32_t *)sys_malloc(48 + 512);
    if (all_blocks == NULL)
    {
        printk("search_dir_entry: sys_malloc for all_blocks failed");
        return false;
    }

    /*读取直接块的地址*/
    uint32_t block_idx = 0;
    while (block_idx < 12)
    {
        all_blocks[block_idx] = pdir->inode->i_sectors[block_idx];
        ++block_idx;
    }

    /*读取间接索引块的地址*/
    if (pdir->inode->i_sectors[12] != 0)
    {
        ide_read(part->my_disk, pdir->inode->i_sectors[12], all_blocks + 12, 1);
    }
    /* 至此,all_blocks存储的是该文件或目录的所有扇区地址 */

    /* 写目录项的时候已保证目录项不跨扇区,
     * 这样读目录项时容易处理, 只申请容纳1个扇区的内存 */
    // buf为读取块数据的暂存区
    uint8_t *buf = (uint8_t *)sys_malloc(SECTOR_SIZE);
    // p_de为指向目录项的指针,值为buf起始地址
    struct dir_entry *p_de = (struct dir_entry *)buf;
    uint32_t dir_entry_size = part->sb->dir_entry_size;
    // 一个块中所包含的目录项的个数
    uint32_t dir_entry_cnt = SECTOR_SIZE / dir_entry_size;

    /* 开始在所有块中查找目录项 */
    // 12个直接块+128个一级间接块=140块
    uint32_t block_cnt = 140;
    block_idx = 0;
    while (block_idx < block_cnt)
    {
        /* 块地址为0时表示该块中无数据,继续在其它块中找 */
        if (all_blocks[block_idx] == 0)
        {
            block_idx++;
            continue;
        }
        ide_read(part->my_disk, all_blocks[block_idx], buf, 1);

        uint32_t dir_entry_idx = 0;
        /* 遍历扇区中所有目录项 */
        while (dir_entry_idx < dir_entry_cnt)
        {
            /* 若找到了,就直接复制整个目录项 */
            if (!strcmp(p_de->filename, name))
            {
                memcpy(dir_e, p_de, dir_entry_size);
                sys_free(buf);
                sys_free(all_blocks);
                return true;
            }
            dir_entry_idx++;
            p_de++;
        }
        block_idx++;
        /*
        此时p_de已经指向扇区内最后一个完整目录项了,需要恢复p_de指向为buf
        然后寻找下一块中的目录项
        */
        p_de = (struct dir_entry *)buf;
        memset(buf, 0, SECTOR_SIZE);
    }
    sys_free(buf);
    sys_free(all_blocks);
    return false;
}

/* 关闭目录 */
void dir_close(struct dir *dir)
{
    /*************      根目录不能关闭     ***************
     *1 根目录自打开后就不应该关闭,否则还需要再次open_root_dir();
     *2 root_dir所在的内存是低端1M之内,并非在堆中,free会出问题 */
    if (dir == &root_dir)
    {
        /* 不做任何处理直接返回*/
        return;
    }
    inode_close(dir->inode);
    sys_free(dir);
}

/* 在内存中初始化目录项p_de */
void create_dir_entry(char *filename, uint32_t inode_no, uint8_t file_type, struct dir_entry *p_de)
{
    ASSERT(strlen(filename) <= MAX_FILE_NAME_LEN);
    /* 初始化目录项 */
    memcpy(p_de->filename, filename, strlen(filename));
    p_de->i_no = inode_no;
    p_de->f_type = file_type;
}

/* 将目录项p_de写入父目录parent_dir中,io_buf由主调函数提供 */
bool sync_dir_entry(struct dir *parent_dir, struct dir_entry *p_de, void *io_buf)
{
    struct inode *dir_inode = parent_dir->inode;
    uint32_t dir_size = dir_inode->i_size;
    uint32_t dir_entry_size = cur_part->sb->dir_entry_size;
    // dir_size应该是dir_entry_size的整数倍
    ASSERT(dir_size % dir_entry_size == 0);

    // 每扇区最大的目录项数目
    uint32_t dir_entrys_per_sec = (512 / dir_entry_size);
    int32_t block_lba = -1;

    // all_blocks保存目录所有的块
    uint32_t all_blocks[140] = {0};

    /* 将该目录的所有扇区地址(12个直接块+ 128个间接块)存入all_blocks */
    uint8_t block_idx = 0;
    /* 将12个直接块存入all_blocks */
    while (block_idx < 12)
    {
        all_blocks[block_idx] = dir_inode->i_sectors[block_idx];
        block_idx++;
    }
    block_idx = 0;
    if (dir_inode->i_sectors[12] != 0)
    { // 若含有一级间接块表
        ide_read(cur_part->my_disk, dir_inode->i_sectors[12], all_blocks + 12, 1);
    }
    // dir_e用来在io_buf中遍历目录项
    struct dir_entry *dir_e = (struct dir_entry *)io_buf;
    int32_t block_bitmap_idx = -1;

    /* 开始遍历所有块以寻找目录项空位,若已有扇区中没有空闲位,
     * 在不超过文件大小的情况下申请新扇区来存储新目录项 */

    block_idx = 0;
    while (block_idx < 140)
    {
        // 文件(包括目录)最大支持12个直接块+128个间接块＝140个块
        block_bitmap_idx = -1;
        // 如果块不存在，需求判定是什么块不存在：直接块，间接索引块，间接块
        if (all_blocks[block_idx] == 0)
        {
            // 块不存在需要申请块
            block_lba = block_bitmap_alloc(cur_part);
            if (block_lba == -1)
            {
                printk("alloc block bitmap for sync_dir_entry failed\n");
                return false;
            }

            /* 每分配一个块就同步一次block_bitmap */
            block_bitmap_idx = block_lba - cur_part->sb->data_start_lba;
            ASSERT(block_bitmap_idx != -1);
            bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP);

            /*第一种情况，直接块不存在*/
            block_bitmap_idx = -1;
            if (block_idx < 12)
            {
                dir_inode->i_sectors[block_idx] = all_blocks[block_idx] = block_lba;
            }
            /*第二种情况，间接索引块不存在*/
            else if (block_idx == 12)
            {
                dir_inode->i_sectors[12] = block_lba; // 将上面分配的块做为一级间接块表地址
                block_lba = -1;
                block_lba = block_bitmap_alloc(cur_part); // 再分配一个块做为第0个间接块
                if (block_lba == -1)
                {
                    block_bitmap_idx = dir_inode->i_sectors[12] - cur_part->sb->data_start_lba;
                    bitmap_set(&cur_part->block_bitmap, block_bitmap_idx, 0);
                    dir_inode->i_sectors[12] = 0;
                    printk("alloc block bitmap for sync_dir_entry failed\n");
                    return false;
                }

                /* 每分配一个块就同步一次block_bitmap */
                block_bitmap_idx = block_lba - cur_part->sb->data_start_lba;
                ASSERT(block_bitmap_idx != -1);
                bitmap_sync(cur_part, block_bitmap_idx, BLOCK_BITMAP);

                all_blocks[12] = block_lba;
                /* 把新分配的第0个间接块地址写入一级间接块表 */
                ide_write(cur_part->my_disk, dir_inode->i_sectors[12], all_blocks + 12, 1);
            }
            /*第三种情况，间接块未分配*/
            else
            {
                all_blocks[block_idx] = block_lba;
                /* 把新分配的第(block_idx-12)个间接块地址写入一级间接块表 */
                ide_write(cur_part->my_disk, dir_inode->i_sectors[12], all_blocks + 12, 1);
            }

            /* 再将新目录项p_de写入新分配的间接块 */
            memset(io_buf, 0, 512);
            memcpy(io_buf, p_de, dir_entry_size);
            ide_write(cur_part->my_disk, all_blocks[block_idx], io_buf, 1);
            dir_inode->i_size += dir_entry_size;
            return true;
        }

        /* 若第block_idx块已存在,将其读进内存,然后在该块中查找空目录项 */
        ide_read(cur_part->my_disk, all_blocks[block_idx], io_buf, 1);
        /* 在扇区内查找空目录项 */
        uint8_t dir_entry_idx = 0;
        while (dir_entry_idx < dir_entrys_per_sec)
        {
            if ((dir_e + dir_entry_idx)->f_type == FT_UNKNOWN)
            { // FT_UNKNOWN为0,无论是初始化或是删除文件后,都会将f_type置为FT_UNKNOWN.
                memcpy(dir_e + dir_entry_idx, p_de, dir_entry_size);
                ide_write(cur_part->my_disk, all_blocks[block_idx], io_buf, 1);

                dir_inode->i_size += dir_entry_size;
                return true;
            }
            dir_entry_idx++;
        }
        block_idx++;
    }
    printk("directory is full!\n");
    return false;
}