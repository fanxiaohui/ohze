//
//  network_utils.c
//  SD15-Product
//
//  Created by Nuno Alexandre on 28/10/14.
//  Copyright (c) 2014 Nuno Alexandre. All rights reserved.
//

#include <stdio.h>
#include "inet.h"
#include <errno.h>
#include "list-private.h"
#include "message-private.h"
#include <netdb.h> //hostent
#include "general_utils.h"
#include "network_utils.h"

/*
 * address_and_port is a string containing a host address and a port
 * with the format host_address:port.
 * returns port.
 */
char * get_port (const char * address_and_port) {
    char * address_and_port_pointer = strdup(address_and_port);
    strtok(address_and_port_pointer, ":");
    char * port = strdup(strtok(NULL,":"));
    free(address_and_port_pointer);
    return port;
}

char * get_address (const char * address_and_port) {
    //guarda valor de IP ou endereço de servidor
    char * address_and_port_pointer = strdup(address_and_port);
    // fica com o endereço
    char * address = strdup(strtok(address_and_port_pointer, ":"));
    free(address_and_port_pointer);
    return address;
}

int reads_server_portnumber ( const char * stringWithPortNumber ) {
    return atoi(stringWithPortNumber);
}

int portnumber_is_invalid (int portNumber ) {
    return portNumber <= 0 || ((portNumber >=1 && portNumber<=1023) || (portNumber >=49152 && portNumber<=65535));
}


/*
 Get ip from domain name
 */
int hostname_to_ip(char * hostname , char* ip){
    
    struct hostent *host_entry;
    struct in_addr **addr_list;
    int i;
    
    if ( (host_entry = gethostbyname( hostname ) ) == NULL){
        // get the host info
        puts("HOSTNAME TO IP ERROR: ");
        return TASK_FAILED;
    }
    
    addr_list = (struct in_addr **) host_entry->h_addr_list;
    
    for(i = 0; addr_list[i] != NULL; i++){
        //Return the first one;
        strcpy(ip , inet_ntoa(*addr_list[i]) );
        return ip[i];
    }
    
    return TASK_FAILED;
}

/*
 * Ensures that all nbytesToWrite of the buffer are written to the socket_fd.
 * The only case it doesn't happen is if some EINTR EPIPE happens.
 * Returns the number of bytes written so if its different
 * than nbytesToWrite something went wrong.
 */
int write_all(int socket_fd, const void *buffer, int bytesToWrite) {
    
    int bufsize = bytesToWrite;
    while(bytesToWrite>0) {
        int writtenBytes = (int) write(socket_fd, buffer, bytesToWrite);
        if(writtenBytes<0) {
            if(errno==EINTR) continue;
            perror("network_server > write_all > failed");
            return writtenBytes;
        }
        //moves buffer pointer
        buffer += writtenBytes;
        //bytes to write
        bytesToWrite -= writtenBytes;
    }
    return bufsize;
}

/*
 * Ensures that all nbytesToRead are readed from the socket
 * and moved into the buffer.
 * Returns the number of bytes copied so if its different
 * than nbytesToRead something went wrong.
 */
int read_all( int socket_fd, void *buffer, int nBytesToRead ) {
    int bufsize = nBytesToRead;
    
    while ( nBytesToRead > 0 ) {
        int nReadedBytes = (int) read(socket_fd, buffer, nBytesToRead);
        if ( nReadedBytes < 0 ) {
            if(errno==EINTR) continue;
            perror("network_server > read_all > failed");
            return nReadedBytes;
        }
        //moves buffer pointer
        buffer += nReadedBytes;
        //bytes to write
        nBytesToRead -= nReadedBytes;
    }
    return bufsize;
}

/*
 * Sends a given message to the connection_socket_fd.
 * First sends an integer with the message buffer size and then the message itself.
 * In error case returns TASK_FAILED, TASK_SUCCEEDED otherwise.
 */
int send_message (int connection_socket_fd, struct message_t * messageToSend) {
    
    if ( messageToSend == NULL )
        return TASK_FAILED;
    
    printf("Sending message: "); message_print(messageToSend); printf("\n");
    
    //the buffer
    char * messageToSend_buffer = NULL;
    //the message size
    int message_size = -1;
    //fills buffer with the serialized messageToSend
    if ( (message_size = message_to_buffer(messageToSend, &messageToSend_buffer)) == -1 ) {
        printf("send message > error on message_to_buffer\n");
        return TASK_FAILED;
    }
    
    //converts it to network format
    int message_size_n = htonl(message_size);
    //sends the size of the message
    if ( write_all(connection_socket_fd, &message_size_n, BUFFER_INTEGER_SIZE) != BUFFER_INTEGER_SIZE ) {
        return TASK_FAILED;
    }
    //and sends the message
    if ( write_all(connection_socket_fd, messageToSend_buffer, message_size ) != message_size ) {
        return TASK_FAILED;
    }
    
    //frees the local buffer
    free(messageToSend_buffer);
        
    return TASK_SUCCEEDED;
}


/*
 * Expects to receives an integer of 4 bytes followd by a serialized message_t from connection_socket_fd.
 * In error case returns NULL, otherwise returns the deserialized message.
 */
struct message_t* receive_message (int connection_socket_fd) {
    
    int size_of_msg_received = 0;

    // 1. Lê tamanho da mensagem que a ser recebida
    if ( (read_all(connection_socket_fd,&size_of_msg_received, BUFFER_INTEGER_SIZE)) != BUFFER_INTEGER_SIZE ) {
        perror("receive_message -> failed to read message size\n");
        return NULL;
    }
     // 1.1 Converte tamanho da mensagem para formato cliente
    int size_of_msg_received_NTOHL = ntohl(size_of_msg_received);
    //safety check
    if ( size_of_msg_received_NTOHL  <= 0 || size_of_msg_received_NTOHL > MAX_MSG ) {
        return NULL;
    }
    
    // 2. Recebe a mensagem
    // allocs a buffer with the size that was informed from the other host
    char * message_buffer = (char*) malloc(size_of_msg_received_NTOHL);
    // Marca a terminação do buffer/string
    message_buffer[size_of_msg_received_NTOHL+1] = '\0';
    
    // lê a mensagem enviada
    if( (read_all(connection_socket_fd,message_buffer, size_of_msg_received_NTOHL) != size_of_msg_received_NTOHL)  ) {
        perror("receive_message -> failed to read message\n");
        free(message_buffer);
        return NULL;
    }
    
    // 2.4 Converte buffer para Mensagem
    struct message_t * message_received = buffer_to_message(message_buffer, size_of_msg_received);
    // 2.5 Verifica se a mensagem foi bem criada */
    if ( message_received == NULL ) {
        perror("receive_message -> failed to buffer_to_message (returned null)\n");
        free(message_buffer);
        return NULL;
    }
    
    printf("Received message: "); message_print(message_received); printf("\n");
    
    //frees the local buffer
    free(message_buffer);
    
    return message_received;
}

