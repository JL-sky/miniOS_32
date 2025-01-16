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

/* 文件类型 */
enum file_types
{
    FT_UNKNOWN,  // 不支持的文件类型
    FT_REGULAR,  // 普通文件
    FT_DIRECTORY // 目录
};

void filests_init(void);

#endif