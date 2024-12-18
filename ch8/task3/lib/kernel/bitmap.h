#ifndef __LIB_KERNEL_BITMAP_H
#define __LIB_KERNEL_BITMAP_H
#include "global.h"
#define BITMAP_MASK 1
struct bitmap
{                            // 这个数据结构就是用来管理整个位图
    uint32_t btmp_bytes_len; // 记录整个位图的大小，单位为字节
    uint8_t *bits;           // 用来记录位图的起始地址，我们未来用这个地址遍历位图时，操作单位指定为最小的字节
};
/*位图初始化*/
void bitmap_init(struct bitmap *btmp);
/*确定位图的bit_idx位是否为1，即判断某块内存是否被分配*/
bool bitmap_scan_test(struct bitmap *btmp, uint32_t bit_idx);
/*在位图中寻找连续的cnt个0，以此来分配一块连续未被占用的内存*/
int bitmap_scan(struct bitmap *btmp, uint32_t cnt);
/*将位图的某一位设置为1或0*/
void bitmap_set(struct bitmap *btmp, uint32_t bit_idx, int8_t value);
#endif