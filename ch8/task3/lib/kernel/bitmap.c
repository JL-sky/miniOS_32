#include "bitmap.h"
#include "debug.h"
#include "interrupt.h"
#include "print.h"
#include "stdint.h"
#include "string.h"

/*位图初始化*/
void bitmap_init(struct bitmap *btmp)
{
    memset(btmp->bits, 0, btmp->btmp_bytes_len);
}
/*确定位图的bit_idx位是否为1*/
bool bitmap_scan_test(struct bitmap *btmp, uint32_t bit_idx)
{
    uint32_t byte_idx = bit_idx / 8;
    uint32_t bit_odd = bit_idx % 8;
    // 小端字节序，故低位在右侧
    return (btmp->bits[byte_idx] & (BITMAP_MASK << bit_odd));
}
/*
在位图中连续的cnt个0，以此来分配一块连续未被占用的内存
*/
int bitmap_scan(struct bitmap *btmp, uint32_t cnt)
{
    uint32_t idx_type = 0;
    /*先逐字节比较，找到第一个含有比特0位的字节，0xff=1111 1111*/
    while (btmp->bits[idx_type] == 0xff && idx_type < btmp->btmp_bytes_len)
        ++idx_type;
    ASSERT(idx_type < btmp->btmp_bytes_len);
    if (idx_type == btmp->btmp_bytes_len)
        return -1;

    /*在该字节内找到第一个为0的比特位*/
    int idx_bit = 0;
    while ((uint8_t)(BITMAP_MASK << idx_bit) & btmp->bits[idx_type])
        ++idx_bit;

    /*空闲位在位图内的下标*/
    int bit_idx_start = idx_type * 8 + idx_bit;
    if (cnt == 1)
        return bit_idx_start;

    // 记录还剩余多少位没有判断
    uint32_t bit_left = btmp->btmp_bytes_len * 8 - bit_idx_start;
    uint32_t next_bit = bit_idx_start + 1;
    uint32_t count = 0;

    /*寻找剩余连续的0*/
    bit_idx_start = -1;
    while (bit_left--)
    {
        if (!bitmap_scan_test(btmp, next_bit))
            ++count;
        else
            count = 0;
        if (count == cnt)
        {
            bit_idx_start = next_bit - cnt + 1;
            break;
        }
        ++next_bit;
    }
    return bit_idx_start;
}
/*
将位图的某一位设置为1或0
*/
void bitmap_set(struct bitmap *btmp, uint32_t bit_idx, int8_t value)
{
    ASSERT((value == 0) || (value == 1));
    uint32_t idx_byte = bit_idx / 8;
    uint32_t idx_odd = bit_idx % 8;
    /* 一般都会用个0x1这样的数对字节中的位操作,
     * 将1任意移动后再取反,或者先取反再移位,可用来对位置0操作。*/
    if (value)
    { // 如果value为1
        btmp->bits[idx_byte] |= (BITMAP_MASK << idx_odd);
    }
    else
    { // 若为0
        btmp->bits[idx_byte] &= ~(BITMAP_MASK << idx_odd);
    }
}