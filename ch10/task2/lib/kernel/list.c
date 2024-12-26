#include "list.h"
#include "interrupt.h"

/*初始化双向链表*/
void list_init(struct list *list)
{
    list->head.prev = NULL;
    list->head.next = &list->tail;
    list->tail.prev = &list->head;
    list->tail.next = NULL;
}
/*把链表元素elem插入在元素cur之前*/
void list_insert_before(struct list_elem *cur, struct list_elem *elem)
{
    // 插入元素之前先关中断，确保元素插入过程中不会被中断影响
    enum intr_status old_status = intr_disable();

    elem->prev = cur->prev;
    elem->next = cur;
    cur->prev->next = elem;
    cur->prev = elem;

    // 恢复关中断之前的状态
    intr_set_status(old_status);
}

/*在链表头部插入元素*/
void list_push(struct list *plist, struct list_elem *elem)
{
    list_insert_before(plist->head.next, elem);
}
/*在链表尾部添加元素*/
void list_append(struct list *plist, struct list_elem *elem)
{
    list_insert_before(&plist->tail, elem);
}

/*删除链表元素节点*/
void list_remove(struct list_elem *pelem)
{
    enum intr_status old_status = intr_disable();

    pelem->prev->next = pelem->next;
    pelem->next->prev = pelem->prev;

    intr_set_status(old_status);
}
/*弹出链表头部元素*/
struct list_elem *list_pop(struct list *plist)
{
    struct list_elem *elem = plist->head.next;
    list_remove(elem);
    return elem;
}
/*在链表中查找元素obj_elem*/
bool elem_find(struct list *plist, struct list_elem *obj_elem)
{
    struct list_elem *ptr = plist->head.next;
    while (ptr != &plist->tail)
    {
        if (ptr == obj_elem)
            return true;
        ptr = ptr->next;
    }
    return false;
}
/*判断链表是否为空*/
bool list_empty(struct list *plist)
{
    return (plist->head.next == &plist->tail);
}
/*返回链表长度*/
uint32_t list_len(struct list *plist)
{
    struct list_elem *ptr = plist->head.next;
    uint32_t len = 0;
    while (ptr != &plist->tail)
    {
        ++len;
        ptr = ptr->next;
    }
    return len;
}
/* 把链表plist中的每个元素elem和arg传给回调函数func,
 * arg给func用来判断elem是否符合条件.
 * 本函数的功能是遍历列表内所有节点elem
 * 逐个判断链表中是否有节点elem使得func(elem,arg)成立。
 * 找到符合条件的元素返回元素指针,否则返回NULL. */
struct list_elem *list_traversal(struct list *plist, function func, int arg)
{
    struct list_elem *ptr = plist->head.next;
    if (list_empty(plist))
        return NULL;
    while (ptr != &plist->tail)
    {
        if (func(ptr, arg))
            return ptr;
        ptr = ptr->next;
    }
    return NULL;
}
