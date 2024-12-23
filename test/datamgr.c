#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <stdbool.h>
#include "datamgr.h"
#include "lib/dplist.h"


static dplist_t *list = NULL;

static void element_free(void **element) {
  free(*element);
  *element = NULL;
}

element_t *get_sensor_node_by_id(sensor_id_t sensor_id) {
  for (int i = 0; i < dpl_size(list); i++) {
    element_t *data = dpl_get_element_at_index(list, i);
    if (data != NULL && data->sensor_id == sensor_id) return data;
  }
  fprintf(stderr, "Error: Sensor ID %d not found.\n", sensor_id);
  return NULL;
}

void update_running_avg(sensor_data_t *sensor_data) {
  element_t *element = get_sensor_node_by_id(sensor_data->id);
  if (element != NULL) {
    if (element->count < RUN_AVG_LENGTH)
      element->values[element->count++] = sensor_data->value;
    else {
      for (int i = 1; i < RUN_AVG_LENGTH; i++) {
        element->values[i - 1] = element->values[i];
      }
      element->values[RUN_AVG_LENGTH - 1] = sensor_data->value;
    }

    double sum = 0.0;
    for (int i = 0; i < element->count; i++) {
      sum += element->values[i];
    }

    if (element->count < RUN_AVG_LENGTH)
      element->running_avg = sum / element->count;
    else
      element->running_avg = sum / RUN_AVG_LENGTH;
    
    if (element->running_avg > SET_MAX_TEMP) 
      fprintf(stderr, "Warning: Room %hu is too hot! (%.2f°C)\n", element->room_id, element->running_avg);
    else if (element->running_avg < SET_MIN_TEMP) 
      fprintf(stderr, "Warning: Room %hu is too cold! (%.2f°C)\n", element->room_id, element->running_avg);
  }
}

void datamgr_parse_sensor_files(FILE *fp_sensor_map) {
  if (list == NULL)
    list = dpl_create(NULL, element_free, NULL);

  uint16_t room_id;
  uint16_t sensor_id;

  while (fscanf(fp_sensor_map, "%hu %hu", &room_id, &sensor_id) == 2) {
    element_t *element_data = malloc(sizeof(element_t));
    if (element_data == NULL) {
      fprintf(stderr, "Error: Memory allocation failed for element_data.\n");
      exit(EXIT_FAILURE);
    }

    element_data->sensor_id = sensor_id;
    element_data->room_id = room_id;
    element_data->running_avg = 0.0;
    element_data->count = 0;

    list = dpl_insert_at_index(list, element_data, dpl_size(list), false);
  }
}

void datamgr_free() {
  dpl_free(&list, true);
}


int datamgr_get_total_sensors() {
  return dpl_size(list);
}

bool sensor_in_map(sensor_id_t sensor_id) {
  element_t* result = get_sensor_node_by_id(sensor_id);
  if (result == NULL) return false;
  return true;
}

void print_datamgr_contents() {
  if (list == NULL) {
    printf("Data manager list is empty.\n");
    return;
  }

  printf("Contents of the data manager:\n");

  for (int i = 0; i < dpl_size(list); i++) {
    element_t *data = dpl_get_element_at_index(list, i);
    if (data != NULL)
      printf("Sensor ID: %hu, Room ID: %hu, Running Avg: %.2f, Count: %d\n", data->sensor_id, data->room_id, data->running_avg, data->count);
  }
}





