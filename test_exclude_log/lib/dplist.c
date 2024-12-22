#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include "dplist.h"

// 定义节点和链表结构
struct dplist_node {
    dplist_node_t *prev, *next;
    void *element;
};

struct dplist {
    dplist_node_t *head;
    void *(*element_copy)(void *src_element);
    void (*element_free)(void **element);
    int (*element_compare)(void *x, void *y);
};

// 创建链表
dplist_t *dpl_create(void *(*element_copy)(void *src_element),
                     void (*element_free)(void **element),
                     int (*element_compare)(void *x, void *y)) {
    dplist_t *list = malloc(sizeof(dplist_t));
    if (list == NULL) {
        fprintf(stderr, "Error: Unable to allocate memory for list.\n");
        exit(EXIT_FAILURE);
    }
    list->head = NULL;
    list->element_copy = element_copy;
    list->element_free = element_free;
    list->element_compare = element_compare;
    return list;
}

// 释放链表
void dpl_free(dplist_t **list, bool free_element) {
    if (list == NULL || *list == NULL) return;

    dplist_node_t *current = (*list)->head;
    while (current != NULL) {
        dplist_node_t *next = current->next;
        if (free_element && (*list)->element_free != NULL && current->element != NULL) {
            (*list)->element_free(&(current->element));
        }
        free(current);
        current = next;
    }
    free(*list);
    *list = NULL;
}

// 插入节点
dplist_t *dpl_insert_at_index(dplist_t *list, void *element, int index, bool insert_copy) {
    if (list == NULL) return NULL;

    dplist_node_t *new_node = malloc(sizeof(dplist_node_t));
    if (new_node == NULL) {
        fprintf(stderr, "Error: Unable to allocate memory for new node.\n");
        exit(EXIT_FAILURE);
    }
    new_node->element = (insert_copy && list->element_copy) ? list->element_copy(element) : element;

    if (list->head == NULL) { // 链表为空
        new_node->prev = NULL;
        new_node->next = NULL;
        list->head = new_node;
    } else if (index <= 0) { // 插入到头部
        new_node->prev = NULL;
        new_node->next = list->head;
        list->head->prev = new_node;
        list->head = new_node;
    } else { // 插入到中间或尾部
        dplist_node_t *current = list->head;
        int count = 0;
        while (current->next != NULL && count < index) {
            current = current->next;
            count++;
        }
        new_node->prev = current;
        new_node->next = current->next;
        if (current->next != NULL) {
            current->next->prev = new_node;
        }
        current->next = new_node;
    }
    return list;
}

// 删除节点
dplist_t *dpl_remove_at_index(dplist_t *list, int index, bool free_element) {
    if (list == NULL || list->head == NULL) return list;

    dplist_node_t *current = list->head;
    int count = 0;

    if (index <= 0) { // 删除头节点
        list->head = current->next;
        if (list->head != NULL) list->head->prev = NULL;
    } else { // 删除中间或尾部节点
        while (current->next != NULL && count < index) {
            current = current->next;
            count++;
        }
        if (current->prev != NULL) current->prev->next = current->next;
        if (current->next != NULL) current->next->prev = current->prev;
    }

    if (free_element && list->element_free != NULL && current->element != NULL) {
        list->element_free(&(current->element));
    }
    free(current);
    return list;
}

// 获取链表大小
int dpl_size(dplist_t *list) {
    if (list == NULL) return 0;

    int count = 0;
    dplist_node_t *current = list->head;
    while (current != NULL) {
        current = current->next;
        count++;
    }
    return count;
}

// 获取指定索引的元素
void *dpl_get_element_at_index(dplist_t *list, int index) {
    if (list == NULL || list->head == NULL) return NULL;

    dplist_node_t *current = list->head;
    int count = 0;

    while (current->next != NULL && count < index) {
        current = current->next;
        count++;
    }
    return current->element;
}

// 获取指定引用的元素
void *dpl_get_element_at_reference(dplist_t *list, dplist_node_t *reference) {
    if (list == NULL || list->head == NULL || reference == NULL) return NULL;

    dplist_node_t *current = list->head;

    while (current != NULL) {
        if (current == reference) {
            return current->element;
        }
        current = current->next;
    }
    return NULL;
}

// 获取元素索引
int dpl_get_index_of_element(dplist_t *list, void *element) {
    if (list == NULL || list->head == NULL) return -1;

    int index = 0;
    dplist_node_t *current = list->head;

    while (current != NULL) {
        if (list->element_compare && list->element_compare(current->element, element) == 0) {
            return index;
        }
        current = current->next;
        index++;
    }
    return -1;
}

// 获取指定索引的节点引用
dplist_node_t *dpl_get_reference_at_index(dplist_t *list, int index) {
    if (list == NULL || list->head == NULL) return NULL;

    dplist_node_t *current = list->head;
    int count = 0;

    while (current->next != NULL && count < index) {
        current = current->next;
        count++;
    }
    return current;
}






