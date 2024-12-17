/**
 * \author {AUTHOR}
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include "sbuffer.h"
#include "config.h"
/**
 * basic node for the buffer, these nodes are linked together to create the buffer
 */
typedef struct sbuffer_node {
    struct sbuffer_node *next;  /**< a pointer to the next node*/
    sensor_data_t data;         /**< a structure containing the data */
} sbuffer_node_t;

/**
 * a structure to keep track of the buffer
 */
struct sbuffer {
    sbuffer_node_t *head;       /**< a pointer to the first node in the buffer */
    sbuffer_node_t *tail;       /**< a pointer to the last node in the buffer */
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

    if (pthread_mutex_init(&bufferMut, NULL) != 0)
    {
      free(*buffer);
      return SBUFFER_FAILURE;
    }
    if (pthread_mutex_init(&endMut, NULL) != 0)
    {
      pthread_mutex_destroy(&bufferMut);
      free(*buffer);
      return SBUFFER_FAILURE;
    }
    if (pthread_cond_init(&bufferCond, NULL) != 0)
    {
      pthread_mutex_destroy(&endMut);
      pthread_mutex_destroy(&bufferMut);
      free(*buffer);
      return SBUFFER_FAILURE;
    }

    return SBUFFER_SUCCESS;
}

int sbuffer_free(sbuffer_t **buffer) {
    sbuffer_node_t *dummy;
    if ((buffer == NULL) || (*buffer == NULL)) {
        return SBUFFER_FAILURE;
    }

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

int sbuffer_remove(sbuffer_t *buffer, sensor_data_t *data) {
    sbuffer_node_t *dummy;
    if (buffer == NULL) return SBUFFER_FAILURE;

    pthread_mutex_lock(&bufferMut);
    //if (buffer->head == NULL) return SBUFFER_NO_DATA;
    while (buffer->head == NULL && !done)
    {
      pthread_cond_wait(&bufferCond, &bufferMut);
    }
    if (buffer->head == NULL && done)
    {
      printf("End marker detected by reader. Exiting.\n");
      pthread_mutex_unlock(&bufferMut);
      return SBUFFER_NO_DATA;
    }

    *data = buffer->head->data;
    dummy = buffer->head;
    if (buffer->head == buffer->tail) // buffer has only one node
    {
        buffer->head = buffer->tail = NULL;
    } else  // buffer has many nodes empty
    {
        buffer->head = buffer->head->next;
    }
    free(dummy);

    if (data->id == 0)
    {
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
    if (buffer->tail == NULL) // buffer empty (buffer->head should also be NULL
    {
        buffer->head = buffer->tail = dummy;
    } else // buffer not empty
    {
        buffer->tail->next = dummy;
        buffer->tail = buffer->tail->next;
    }
    pthread_cond_signal(&bufferCond);
    pthread_mutex_unlock(&bufferMut);

    return SBUFFER_SUCCESS;
}
