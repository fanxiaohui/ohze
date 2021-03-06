//
//  network_client-private.h
//  SD15-Product
//
//  Created by Nuno Alexandre on 25/10/14.
//  Copyright (c) 2014 Nuno Alexandre. All rights reserved.
//

#ifndef SD15_Product_network_client_private_h
#define SD15_Product_network_client_private_h

#include "general_utils.h"

/*
 *the struct server_t has an ip adress,
 *a port number and a sock file descriptor
 */
struct server_t {
    char * ip_address;
    int port;
    int socketfd;
};

/*
 * If something happens with the connection, the client tries to
 * reconnect with server using the same mechanisms that network_connect function
 * returns server to reconnect
 */
struct server_t *network_reconnect(struct server_t *server_to_connect);

/*
 * Função que decide se vai haver nova tentativa de ligação ou
 * reenvio de msg
 */
int network_retransmit (int socketfd);


//module property
int retry_connection;

void network_reset_retransmissions();

#endif


