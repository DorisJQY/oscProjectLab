#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include "sbuffer.h"
#include "config.h"
#include "lib/dplist.h"

typedef struct sbuffer_node {
    struct sbuffer_node *next; 
    sensor_data_t data;         
    bool data_read;
    bool storage_read;
} sbuffer_node_t;

struct sbuffer {
    sbuffer_node_t *head;   
    sbuffer_node_t *tail;  
};

pthread_mutex_t bufferMut;
pthread_mutex_t endMut;
pthread_cond_t bufferCond;
bool done = false;

int sbuffer_init(sbuffer_t **buffer) {
  *buffer = malloc(sizeof(sbuffer_t));
  if (*buffer == NULL) return SBUFFER_FAILURE;
  (*buffer)->head = NULL;
  (*buffer)->tail = NULL;

  if (pthread_mutex_init(&bufferMut, NULL) != 0) {
    free(*buffer);
    return SBUFFER_FAILURE;
  }
  if (pthread_mutex_init(&endMut, NULL) != 0) {
    pthread_mutex_destroy(&bufferMut);
    free(*buffer);
    return SBUFFER_FAILURE;
  }
  if (pthread_cond_init(&bufferCond, NULL) != 0) {
    pthread_mutex_destroy(&endMut);
    pthread_mutex_destroy(&bufferMut);
    free(*buffer);
    return SBUFFER_FAILURE;
  }

  return SBUFFER_SUCCESS;
}

int sbuffer_free(sbuffer_t **buffer) {
  sbuffer_node_t *dummy;
  if ((buffer == NULL) || (*buffer == NULL)) return SBUFFER_FAILURE;

  while ((*buffer)->head) {
    dummy = (*buffer)->head;
    (*buffer)->head = (*buffer)->head->next;
    free(dummy);
  }
  free(*buffer);
  *buffer = NULL;

  if (pthread_mutex_destroy(&bufferMut) != 0) return SBUFFER_FAILURE;
  if (pthread_cond_destroy(&bufferCond) != 0) return SBUFFER_FAILURE;
  if (pthread_mutex_destroy(&endMut) != 0) return SBUFFER_FAILURE;

  return SBUFFER_SUCCESS;
}

int sbuffer_remove(sbuffer_t *buffer, sensor_data_t *data, bool storage_manager) {
  sbuffer_node_t *dummy;
  if (buffer == NULL) return SBUFFER_FAILURE;

  pthread_mutex_lock(&bufferMut);
  while (buffer->head == NULL && !done) {
    pthread_cond_wait(&bufferCond, &bufferMut);
  }
  if (buffer->head == NULL && done) {
    printf("End marker detected by reader. Exiting.\n");
    pthread_mutex_unlock(&bufferMut);
    return SBUFFER_NO_DATA;
  }

  *data = buffer->head->data;
  dummy = buffer->head;
    
  // storage/data manager should wait if it already read  
  if ((storage_manager && dummy->storage_read) || (!storage_manager && dummy->data_read)) {
    pthread_mutex_unlock(&bufferMut);
    return SBUFFER_WAIT;
  }
    
  // set flag to true after read  
  if (storage_manager)
    dummy->storage_read = true;
  else 
    dummy->data_read = true;
    
  // node can be removed after both manager read it
  if (dummy->data_read && dummy->storage_read) {
    if (buffer->head == buffer->tail)
      buffer->head = buffer->tail = NULL;
    else
      buffer->head = buffer->head->next;
    free(dummy);
  }

  if (data->id == 0) {
    printf("End marker detected by reader. Exiting.\n");
    done = true;
    pthread_cond_signal(&bufferCond);
    pthread_mutex_unlock(&bufferMut);
    return SBUFFER_NO_DATA;
  }
  pthread_cond_signal(&bufferCond);
  pthread_mutex_unlock(&bufferMut);
  
  return SBUFFER_SUCCESS;
}

int sbuffer_insert(sbuffer_t *buffer, sensor_data_t *data) {
  sbuffer_node_t *dummy;
  if (buffer == NULL) return SBUFFER_FAILURE;
  dummy = malloc(sizeof(sbuffer_node_t));
  if (dummy == NULL) return SBUFFER_FAILURE;

  pthread_mutex_lock(&bufferMut);
  dummy->data = *data;
  dummy->next = NULL;
    
  dummy->data_read = false;
  dummy->storage_read = false;
    
  if (buffer->tail == NULL)
    buffer->head = buffer->tail = dummy;
  else {
    buffer->tail->next = dummy;
    buffer->tail = buffer->tail->next;
  }
  pthread_cond_signal(&bufferCond);
  pthread_mutex_unlock(&bufferMut);

  return SBUFFER_SUCCESS;
}
