#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include "config.h"
#include "lib/tcpsock.h"
#include "sbuffer.h"

extern sbuffer_t* sbuffer;

void* client_handle(void* arg) {
    tcpsock_t* client = (tcpsock_t*)arg;
    sensor_data_t data;
    int bytes, result;
    bool first_packet = true;

    int sockfd;
    if (tcp_get_sd(client, &sockfd) != TCP_NO_ERROR) {
        fprintf(stderr, "Failed to get socket descriptor.\n");
        tcp_close(&client);
        return NULL;
    }

    // initialize timeval structure for setting receive timeout
    struct timeval tv;
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;

    // set receive timeout option on the socket
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) {
        fprintf(stderr, "setsockopt failed: %s\n", strerror(errno));
        tcp_close(&client);
        return NULL;
    }

    do {
        // read sensor ID
        bytes = sizeof(data.id);
        result = tcp_receive(client, (void*)&data.id, &bytes);
        if (result != TCP_NO_ERROR) break;

        // read temperature
        bytes = sizeof(data.value);
        result = tcp_receive(client, (void*)&data.value, &bytes);
        if (result != TCP_NO_ERROR) break;

        // read timestamp
        bytes = sizeof(data.ts);
        result = tcp_receive(client, (void*)&data.ts, &bytes);
        if (result != TCP_NO_ERROR) break;

        if ((result == TCP_NO_ERROR) && bytes) {
            if (first_packet) {
                char msg[SIZE];
                snprintf(msg, SIZE, "Sensor node %hu has opened a new connection.", data.id);
                write_to_pipe(msg);
                first_packet = false;
            }
            // insert data to sbuffer
            sbuffer_insert(sbuffer, &data);
            // print inserted data to terminal
            printf("sensor id = %hu - temperature = %.6f - timestamp = %ld\n", data.id, data.value, (long int)data.ts);
        }
    } while (result == TCP_NO_ERROR);

    if (result == TCP_CONNECTION_CLOSED) {
        printf("Peer has closed connection\n");
        char msg[SIZE];
        snprintf(msg, SIZE, "Sensor node %hu has closed the connection.", data.id);
        write_to_pipe(msg);
    }
    else if (result == TCP_TIMEOUT_ERROR) {
        printf("Connection time out for sensor node %hu\n", data.id);
        char msg[SIZE];
        snprintf(msg, SIZE, "Sensor node %hu connection time out.", data.id);
        write_to_pipe(msg);
    }
    else
      printf("Error occurred on connection to peer\n");

    tcp_close(&client);
    return NULL;
}


int connmgr_run(int port, int max_conn)
{
    tcpsock_t *server, *client;
    int conn_counter = 0;

    printf("Test server is started\n");
    if (tcp_passive_open(&server, port) != TCP_NO_ERROR) {
        fprintf(stderr,"Error: tcp_passive_open() failed.\n");
        return -1;
    }

    pthread_t tid[max_conn];
    
    while (conn_counter < max_conn) {
        if (tcp_wait_for_connection(server, &client) != TCP_NO_ERROR) {
            fprintf(stderr,"Error: tcp_wait_for_connection() failed.\n");
            tcp_close(&server);
            return -1;
        }
        printf("Incoming client connection\n");

        if (pthread_create(&tid[conn_counter], NULL, client_handle, client) != 0) {
            fprintf(stderr, "Failed to create thread: %s\n", strerror(errno));
            tcp_close(&client);
            continue;
        }
        conn_counter++;
    }

    for (int i = 0; i < conn_counter; i++) {
        pthread_join(tid[i], NULL);
    }

    sensor_data_t end_marker = {0, 0.0, 0};
    sbuffer_insert(sbuffer, &end_marker);
    printf("End marker created by writer.\n");

    if (tcp_close(&server) != TCP_NO_ERROR) {
        fprintf(stderr,"Error: tcp_close() failed on server.\n");
        return -1;
    }

    printf("Test server is shutting down\n");
    return 0;
}

