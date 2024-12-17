#include <stdlib.h>
#include "datamgr.h"
#include "lib/dplist.h"

static dplist_t *list = NULL;
static sensor_temp_data_t *sensor_temp_data_map[UINT16_MAX] = {NULL};

static element_t *get_sensor_node_by_id(sensor_id_t sensor_id)
{
  for (int i = 0; i < dpl_size(list); i++) 
  {
    element_t *data = dpl_get_element_at_index(list, i);
    if (data->sensor_id == sensor_id) 
    {
      return data;
    }
  }
  fprintf(stderr, "Error: Sensor ID %hd not found in the sensor map!\n", sensor_id);
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
  if (element == NULL) 
  {
    return;
  }
  element->running_avg = sum / data->measurement_count;
  element->last_modified = timestamp;
  
  if (element->running_avg > SET_MAX_TEMP) 
  {
    fprintf(stderr, "Warning: Room %hd is too hot! Sensor ID %hd has exceeded max temperature: %.2f°C\n", element->room_id, sensor_id, element->running_avg);
  }
  else if (element->running_avg < SET_MIN_TEMP) 
  {
    fprintf(stderr, "Warning: Room %hd is too cold! Sensor ID %hd has dropped below min temperature: %.2f°C\n", element->room_id, sensor_id, element->running_avg);
  }
}

void datamgr_parse_sensor_files(FILE *fp_sensor_map, FILE *fp_sensor_data)
{
  list = dpl_create(NULL, NULL, NULL);
  uint16_t room_id;
  uint16_t sensor_id;
  double temperature;
  time_t timestamp;
  
  // 解析传感器映射文件
  while (fscanf(fp_sensor_map, "%hd %hd\n", &room_id, &sensor_id) != EOF) 
  {
    element_t *element_data = malloc(sizeof(element_t));
    if (element_data == NULL) {
      fprintf(stderr, "Memory allocation failed for element_data\n");
      return;
    }
    element_data->sensor_id = sensor_id;
    element_data->room_id = room_id;
    element_data->running_avg = 0.0;
    element_data->last_modified = 0;
    dpl_insert_at_index(list, element_data, dpl_size(list), true);

    // Ensure proper initialization of sensor_temp_data_map[sensor_id]
    if (sensor_id < UINT16_MAX) {
      if (sensor_temp_data_map[sensor_id] == NULL) {  // Only allocate if not already allocated
        sensor_temp_data_t *temp_data = malloc(sizeof(sensor_temp_data_t));
        if (temp_data == NULL) {
          fprintf(stderr, "Memory allocation failed for temp_data\n");
          return;
        }
        temp_data->measurement_count = 0;
        sensor_temp_data_map[sensor_id] = temp_data;
      }
    } else {
      fprintf(stderr, "Error: Sensor ID %hd exceeds max limit\n", sensor_id);
    }
  }

  // 解析传感器数据文件
  while (fread(&sensor_id, sizeof(sensor_id), 1, fp_sensor_data)) 
  {
    fread(&temperature, sizeof(temperature), 1, fp_sensor_data);
    fread(&timestamp, sizeof(timestamp), 1, fp_sensor_data);
    
    element_t *data = get_sensor_node_by_id(sensor_id);
    if (data == NULL) 
    {
      continue;
    }

    sensor_temp_data_t *temp_data = sensor_temp_data_map[sensor_id];
    if (temp_data != NULL) {
      update_running_avg(sensor_id, temp_data, temperature, timestamp);
    } else {
      fprintf(stderr, "Error: Sensor ID %hd has no associated temperature data\n", sensor_id);
    }
  }
}


void datamgr_free()
{
  // 释放 dplist 中的所有元素
  for (int i = 0; i < dpl_size(list); i++) 
  {
    element_t *data = dpl_get_element_at_index(list, i);
    free(data);
  }

  // 释放 sensor_temp_data_map 中分配的内存
  for (int i = 0; i < UINT16_MAX; i++) 
  {
    if (sensor_temp_data_map[i] != NULL) 
    {
      // 先释放 measurements 数组
      free(sensor_temp_data_map[i]->measurements);
      // 然后释放 sensor_temp_data_map[i] 本身
      free(sensor_temp_data_map[i]);
    }
  }

  // 释放 dplist
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



