#ifndef __FS_FS_H
#define __FS_FS_H
#include "stdint.h"

// 每个分区所支持最大创建的文件数
#define MAX_FILES_PER_PART 4096
// 每个扇区的位数（512*8）
#define BITS_PER_SECTOR 4096
// 扇区的字节大小
#define SECTOR_SIZE 512
// 块字节的大小，为方便我们将一块直接定义为一个扇区，当然也可以定义两个或者四个扇区等
#define BLOCK_SIZE SECTOR_SIZE

// 路径最大长度
#define MAX_PATH_LEN 512

/* 文件类型 */
enum file_types
{
    // 不支持的文件类型
    FT_UNKNOWN,
    // 普通文件
    FT_REGULAR,
    // 目录
    FT_DIRECTORY
};

/* 打开文件的选项 */
enum oflags
{
    O_RDONLY,   // 只读
    O_WRONLY,   // 只写
    O_RDWR,     // 读写
    O_CREAT = 4 // 创建
};

/* 用来记录查找文件过程中已找到的上级路径,也就是查找文件过程中"走过的地方" */
struct path_search_record
{
    // 查找过程中的父路径
    char searched_path[MAX_PATH_LEN];
    // 文件或目录所在的直接父目录
    struct dir *parent_dir;
    // 找到的是普通文件还是目录,找不到将为未知类型(FT_UNKNOWN)
    enum file_types file_type;
};

/* 文件读写位置偏移量 */
enum whence
{
    SEEK_SET = 1,
    SEEK_CUR,
    SEEK_END
};

void filests_init(void);
extern struct partition *cur_part;
int32_t path_depth_cnt(char *pathname);
int32_t sys_open(const char *pathname, uint8_t flags);
int32_t sys_close(int32_t fd);
int32_t sys_write(int32_t fd, const void *buf, uint32_t count);
int32_t sys_read(int32_t fd, void *buf, uint32_t count);
int32_t sys_lseek(int32_t fd, int32_t offset, uint8_t whence);
int32_t sys_unlink(const char *pathname);
#endif