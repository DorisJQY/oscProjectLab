#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "datamgr.h"
#include "lib/dplist.h"


static dplist_t *list = NULL;
static sensor_temp_data_t *sensor_temp_data_map[UINT16_MAX] = {NULL};

static void element_free(void **element) 
{
  free(*element);
  *element = NULL;
}

static element_t *get_sensor_node_by_id(sensor_id_t sensor_id) 
{
  for (int i = 0; i < dpl_size(list); i++) 
  {
    element_t *data = dpl_get_element_at_index(list, i);
    if (data != NULL && data->sensor_id == sensor_id) 
    {
      return data;
    }
  }
  fprintf(stderr, "Error: Sensor ID %d not found.\n", sensor_id);
  return NULL;
}

static void update_running_avg(uint16_t sensor_id, sensor_temp_data_t *data, double new_temperature, time_t timestamp) 
{
  if (data->measurement_count < RUN_AVG_LENGTH) 
  {
    data->measurements[data->measurement_count++] = new_temperature;
  } 
  else 
  {
    for (int i = 1; i < RUN_AVG_LENGTH; i++) 
    {
      data->measurements[i - 1] = data->measurements[i];
    }
    data->measurements[RUN_AVG_LENGTH - 1] = new_temperature;
  }

  double sum = 0.0;
  for (int i = 0; i < data->measurement_count; i++) 
  {
    sum += data->measurements[i];
  }

  element_t *element = get_sensor_node_by_id(sensor_id);
  if (element != NULL) 
  {
    element->running_avg = sum / data->measurement_count;
    element->last_modified = timestamp;

    if (element->running_avg > SET_MAX_TEMP) 
    {
      fprintf(stderr, "Warning: Room %hu is too hot! (%.2f°C)\n", element->room_id, element->running_avg);
    } 
    else if (element->running_avg < SET_MIN_TEMP) 
    {
      fprintf(stderr, "Warning: Room %hu is too cold! (%.2f°C)\n", element->room_id, element->running_avg);
    }
  }
}

void datamgr_parse_sensor_files(FILE *fp_sensor_map, FILE *fp_sensor_data) 
{
  if (list == NULL) 
  {
    list = dpl_create(NULL, element_free, NULL);
  }

  uint16_t room_id;
  uint16_t sensor_id;
  double temperature;
  time_t timestamp;

  while (fscanf(fp_sensor_map, "%hu %hu", &room_id, &sensor_id) == 2) 
  {
    element_t *element_data = malloc(sizeof(element_t));
    if (element_data == NULL) 
    {
      fprintf(stderr, "Error: Memory allocation failed for element_data.\n");
      exit(EXIT_FAILURE);
    }

    element_data->sensor_id = sensor_id;
    element_data->room_id = room_id;
    element_data->running_avg = 0.0;
    element_data->last_modified = 0;

    list = dpl_insert_at_index(list, element_data, dpl_size(list), false);

    sensor_temp_data_map[sensor_id] = malloc(sizeof(sensor_temp_data_t));
    if (sensor_temp_data_map[sensor_id] == NULL) 
    {
      fprintf(stderr, "Error: Memory allocation failed for sensor_temp_data_map[%hu].\n", sensor_id);
      exit(EXIT_FAILURE);
    }
    sensor_temp_data_map[sensor_id]->measurement_count = 0;
  }

  while (fread(&sensor_id, sizeof(sensor_id), 1, fp_sensor_data)) 
  {
    fread(&temperature, sizeof(temperature), 1, fp_sensor_data);
    fread(&timestamp, sizeof(timestamp), 1, fp_sensor_data);

    if (sensor_temp_data_map[sensor_id] != NULL) 
    {
      update_running_avg(sensor_id, sensor_temp_data_map[sensor_id], temperature, timestamp);
    } 
    else 
    {
      fprintf(stderr, "Warning: Sensor ID %hu not initialized.\n", sensor_id);
    }
  }
}

void datamgr_free() 
{
  for (int i = 0; i < UINT16_MAX; i++) 
  {
    if (sensor_temp_data_map[i] != NULL) 
    {
      free(sensor_temp_data_map[i]);
    }
  }
  dpl_free(&list, true);
}

uint16_t datamgr_get_room_id(sensor_id_t sensor_id) 
{
  element_t *data = get_sensor_node_by_id(sensor_id);
  if (data == NULL) return 0;
  return data->room_id;
}

sensor_value_t datamgr_get_avg(sensor_id_t sensor_id) 
{
  element_t *data = get_sensor_node_by_id(sensor_id);
  if (data == NULL) return 0;
  return data->running_avg;
}

time_t datamgr_get_last_modified(sensor_id_t sensor_id) 
{
  element_t *data = get_sensor_node_by_id(sensor_id);
  if (data == NULL) return 0;
  return data->last_modified;
}

int datamgr_get_total_sensors() 
{
  return dpl_size(list);
}







