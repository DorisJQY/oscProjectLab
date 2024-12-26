#ifndef DATAMGR_H_
#define DATAMGR_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "config.h"

#ifndef RUN_AVG_LENGTH
#define RUN_AVG_LENGTH 5
#endif

#ifndef SET_MAX_TEMP
#error SET_MAX_TEMP not set
#endif

#ifndef SET_MIN_TEMP
#error SET_MIN_TEMP not set
#endif


/*
 * Use ERROR_HANDLER() for handling memory allocation problems, invalid sensor IDs, non-existing files, etc.
 */
#define ERROR_HANDLER(condition, ...)    do {                       \
                      if (condition) {                              \
                        printf("\nError: in %s - function %s at line %d: %s\n", __FILE__, __func__, __LINE__, __VA_ARGS__); \
                        exit(EXIT_FAILURE);                         \
                      }                                             \
                    } while(0)

typedef struct {
    uint16_t sensor_id;
    uint16_t room_id;
    double running_avg;
    double values[RUN_AVG_LENGTH];
    int count;
} element_t;

// parse map file
void datamgr_parse_sensor_files(FILE *fp_sensor_map);

// clean up datamgr
void datamgr_free();

// return total amount of sensors
int datamgr_get_total_sensors();

element_t *get_sensor_node_by_id(sensor_id_t sensor_id);

// calculate average temperature
void update_running_avg(sensor_data_t *sensor_data);

// debug function
void print_datamgr_contents();

#endif  //DATAMGR_H_
