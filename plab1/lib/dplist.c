

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "dplist.h"




/*
 * The real definition of struct list / struct node
 */

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


dplist_t *dpl_create(// callback functions
        void *(*element_copy)(void *src_element),
        void (*element_free)(void **element),
        int (*element_compare)(void *x, void *y)
) {
    dplist_t *list;
    list = malloc(sizeof(struct dplist));
    list->head = NULL;
    list->element_copy = element_copy;
    list->element_free = element_free;
    list->element_compare = element_compare;
    return list;
}

void dpl_free(dplist_t **list, bool free_element) {

    //TODO: add your code here
    if (list == NULL) return;
    if (*list == NULL) return;

    dplist_node_t* p1 = (*list)->head;
    dplist_node_t* p2 = NULL;

     while(p1 != NULL)
    {
      p2 = p1;
      p1 = p1->next;
      if(free_element == true)
      {
        if(p2->element != NULL)
        {
        (*list)->element_free(&(p2->element));
        }
      }
      free(p2);
    }
    free(*list);
    *list = NULL;
}

dplist_t *dpl_insert_at_index(dplist_t *list, void *element, int index, bool insert_copy) {

    //TODO: add your code here
    dplist_node_t *ref_at_index, *list_node;
    if (list == NULL) return NULL;

    list_node = malloc(sizeof(dplist_node_t));
    if (insert_copy == true)
    {
      list_node->element = list->element_copy(element);
    }
    else
    {
      list_node->element = element;
    }

    if (list->head == NULL)
    {
      list_node->prev = NULL;
      list_node->next = NULL;
      list->head = list_node;
    }
    else if (index <= 0)
    {
      list_node->prev = NULL;
      list_node->next = list->head;
      list->head->prev = list_node;
      list->head = list_node;
    }
    else
    {
      ref_at_index = dpl_get_reference_at_index(list, index);
      if (index < dpl_size(list))
      {
        list_node->prev = ref_at_index->prev;
        list_node->next = ref_at_index;
        if (ref_at_index->prev != NULL)
        {
          ref_at_index->prev->next = list_node;
        }
        ref_at_index->prev = list_node;
      }
      else
      {
        list_node->next = NULL;
        list_node->prev = ref_at_index;
        if (ref_at_index != NULL)
        {
          ref_at_index->next = list_node;
        }
      }
    }
    return list;
}

dplist_t *dpl_remove_at_index(dplist_t *list, int index, bool free_element) {

    //TODO: add your code here
    if(list == NULL) return list;
    if(list->head == NULL) return list;

    int size = dpl_size(list);
    dplist_node_t *dummy = list->head;
    dplist_node_t *dummyp = NULL;
    dplist_node_t *dummyn = NULL;

    if(index <= 0)
    {
      list->head = dummy->next;
      if (list->head != NULL)
      {
        list->head->prev = NULL;
      }
      if(free_element == true && dummy->element != NULL)
       {
         list->element_free(&(dummy->element));
       }
      free(dummy);
      dummy = NULL;
    }
    else if(index >= size-1)
    {
      while(dummy->next != NULL)
      {
       dummy = dummy->next;
      }
       dummyp = dummy->prev;
       if(dummyp != NULL)
       {
         dummyp->next = NULL;
       }
       else
       {
         list->head = NULL;
       }
       if(free_element == true && dummy->element != NULL)
       {
         list->element_free(&(dummy->element));
       }
       free(dummy);
       dummy = NULL;
    }
    else
    {
      for(int i = 0;i < index;i++)
      {
       dummy = dummy->next;
      }
      if(dummy == NULL) return list;
      dummyp = dummy->prev;
      dummyn = dummy->next;
      if(dummyp != NULL)
      {
        dummyp->next = dummyn;
      }
      if(dummyn != NULL)
      {
        dummyn->prev = dummyp;
      }
      if(free_element == true && dummy->element != NULL)
      {
        list->element_free(&(dummy->element));
      }
      free(dummy);
      dummy = NULL;
    }
    return list;
}

int dpl_size(dplist_t *list) {

    //TODO: add your code here
    if(list == NULL) return -1;
    if(list->head == NULL) return 0;

    dplist_node_t *dummy = list->head;
    int count = 0;
    while(dummy != NULL)
    {
      dummy = dummy->next;
      count++;
    }
    return count;
}



void *dpl_get_element_at_index(dplist_t *list, int index) {

    //TODO: add your code here
    if(list == NULL) return NULL;
    if(list->head == NULL) return NULL;

    dplist_node_t *dummy = list->head;
    int count = 0;
    int size = dpl_size(list);
    if(index <= 0)
    {
      return dummy->element;
    }
    else if(index >= size)
    {
      while(dummy->next != NULL)
      {
       dummy = dummy->next;
      }
      return dummy->element;
    }
    else
    {
      while(count<index)
      {
        dummy = dummy->next;
        count++;
      }
       return dummy->element;
    }
}

int dpl_get_index_of_element(dplist_t *list, void *element) {

    //TODO: add your code here
    if(list == NULL) return -1;

    int index = 0;
    dplist_node_t *dummy = list->head;

    while(dummy != NULL)
    {
      if(list->element_compare(dummy->element,element) == 0)
      {
        return index;
      }
      dummy = dummy -> next;
      index++;
    }
    return -1;
}

dplist_node_t *dpl_get_reference_at_index(dplist_t *list, int index) {

    //TODO: add your code here
    if(list == NULL) return NULL;
    if(list->head == NULL) return NULL;

    dplist_node_t *dummy = list->head;
    int count = 0;
    int size = dpl_size(list);
    if(index <= 0)
    {
      return dummy;
    }
    else if(index >= size)
    {
      while(dummy->next != NULL)
      {
       dummy = dummy->next;
      }
      return dummy;
    }
    else
    {
      while(count<index)
      {
        dummy = dummy->next;
        count++;
      }
       return dummy;
    }
}

void *dpl_get_element_at_reference(dplist_t *list, dplist_node_t *reference) {

    //TODO: add your code here
    if(list == NULL) return NULL;
    if(list->head == NULL) return NULL;
    if(reference == NULL) return NULL;

    dplist_node_t *dummy = list->head;

    while(dummy != NULL)
    {
      if(dummy == reference)
      {
        return dummy->element;
      }
      dummy = dummy -> next;
    }
    return NULL;
}


