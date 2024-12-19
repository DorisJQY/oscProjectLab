#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "lib/dplist.h"
#include "datamgr.h"
#include <time.h>

int main(){
    printf("Hello World\n");

    FILE * map = fopen("room_sensor.map", "r");

    if(map == NULL) return -1;

    datamgr_parse_sensor_files(map);

    datamgr_free();

    fclose(map);
}
