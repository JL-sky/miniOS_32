#ifndef __LIB_KERNEL_LIST_H
#define __LIB_KERNEL_LIST_H

#include "global.h"
// 计算链表节点在PCB中的偏移量，其中链表节点是PCB的成员
#define offset(struct_type, member) (int)(&((struct_type *)0)->member)

// 通过结构体成员（链表节点）地址计算出整个结构体（PCB）的起始地址
#define elem2entry(struct_type, struct_member_name, elem_ptr) \
    (struct_type *)((int)elem_ptr - offset(struct_type, struct_member_name))

/*链表节点类型*/
struct list_elem
{
    struct list_elem *prev;
    struct list_elem *next;
};

/*双链表类型*/
struct list
{
    /*头结点*/
    struct list_elem head;
    /*尾节点*/
    struct list_elem tail;
};
// 定义个叫function的函数类型，返回值是int，参数是链表结点指针与一个整形值
typedef bool(function)(struct list_elem *, int arg);
/*链表初始化*/
void list_init(struct list *);
/*头插法*/
void list_insert_before(struct list_elem *cur, struct list_elem *elem);
/*在链表头部插入元素*/
void list_push(struct list *plist, struct list_elem *elem);
/*在链表尾部插入元素*/
void list_append(struct list *plist, struct list_elem *elem);
/*移除元素*/
void list_remove(struct list_elem *pelem);
/*弹出链表头部元素*/
struct list_elem *list_pop(struct list *plist);
/*判断链表是否为空*/
bool list_empty(struct list *plist);
/*获取链表长度*/
uint32_t list_len(struct list *plist);
/*在链表中查找节点*/
bool elem_find(struct list *plist, struct list_elem *obj_elem);
/*判断链表中是否有节点elem
使得func(elem,arg)成立*/
struct list_elem *list_traversal(struct list *plist, function func, int arg);
void list_iterate(struct list *plist);

#endif