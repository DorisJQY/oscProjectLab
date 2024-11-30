/**
 * \author {AUTHOR}
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "config.h"
#include "lib/tcpsock.h"
#include <pthread.h>

/**
 * Implements a sequential test server (only one connection at the same time)
 */
int active_conn = 0;
pthread_mutex_t conn_mutex = PTHREAD_MUTEX_INITIALIZER;
void* client_handle(void* arg) {
    tcpsock_t* client = (tcpsock_t*)arg;
    sensor_data_t data;
    int bytes, result;

    do {
        // read sensor ID
        bytes = sizeof(data.id);
        result = tcp_receive(client, (void*)&data.id, &bytes);

        // read temperature
        bytes = sizeof(data.value);
        result = tcp_receive(client, (void*)&data.value, &bytes);

        // read timestamp
        bytes = sizeof(data.ts);
        result = tcp_receive(client, (void*)&data.ts, &bytes);

        if ((result == TCP_NO_ERROR) && bytes) {
            printf("sensor id = %" PRIu16 " - temperature = %g - timestamp = %ld\n", 
                   data.id, data.value, (long int)data.ts);
        }
    } while (result == TCP_NO_ERROR);

    if (result == TCP_CONNECTION_CLOSED)
        printf("Peer has closed connection\n");
    else
        printf("Error occurred on connection to peer\n");

    tcp_close(&client);
    pthread_mutex_lock(&conn_mutex);
    active_conn--;
    pthread_mutex_unlock(&conn_mutex);
    return NULL;
}


int main(int argc, char *argv[]) {
    tcpsock_t *server, *client;
    int conn_counter = 0;

    if(argc < 3) {
    	printf("Please provide the right arguments: first the port, then the max nb of clients");
    	return -1;
    }

    int MAX_CONN = atoi(argv[2]);
    int PORT = atoi(argv[1]);

    printf("Test server is started\n");
    if (tcp_passive_open(&server, PORT) != TCP_NO_ERROR) exit(EXIT_FAILURE);

    pthread_t tid;
    while (1) {
        pthread_mutex_lock(&conn_mutex);
        if (conn_counter >= MAX_CONN && active_conn == 0) {
            pthread_mutex_unlock(&conn_mutex);
            break;
        }
        pthread_mutex_unlock(&conn_mutex);

        if (conn_counter < MAX_CONN) {
            if (tcp_wait_for_connection(server, &client) != TCP_NO_ERROR) exit(EXIT_FAILURE);
            printf("Incoming client connection\n");

            pthread_mutex_lock(&conn_mutex);
            conn_counter++;
            active_conn++;
            pthread_mutex_unlock(&conn_mutex);

            if (pthread_create(&tid, NULL, client_handle, client) != 0) {
                perror("Failed to create thread");
                tcp_close(&client);

                pthread_mutex_lock(&conn_mutex);
                conn_counter--;
                active_conn--;
                pthread_mutex_unlock(&conn_mutex);
                continue;
            }
            pthread_detach(tid);
        }
    }

    if (tcp_close(&server) != TCP_NO_ERROR) exit(EXIT_FAILURE);
    printf("Test server is shutting down\n");
    return 0;
}




