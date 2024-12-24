/**
 * \author Bert Lagaisse
 */

#ifndef _SENSOR_DB_H_
#define _SENSOR_DB_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "config.h"

FILE * open_db(char * filename, bool append);
int insert_sensor(FILE * f, sensor_data_t *sensor_data);
int close_db(FILE * f);


#endif /* _SENSOR_DB_H_ */
