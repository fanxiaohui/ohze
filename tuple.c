//
//  tuple.c
//  sd15project
//
//  Created by Grupo SD015 on 22/09/14.
//  Copyright (c) 2014 Grupo SD015. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tuple.h"
#include "message-private.h"
#include <assert.h>
#include "general_utils.h"
#include "inet.h"

/* Função que cria um novo tuplo (isto é, que inicializa
 * a estrutura e aloca a memória necessária).
 */
struct tuple_t *tuple_create(int tuple_dim) {
    //checks if required dim is valid
    if ( tuple_dim <= 0)
        return NULL;
    
    //allocs memory
    struct tuple_t * newTuple = (struct tuple_t*) malloc (sizeof(struct tuple_t));
    
    if ( newTuple != NULL ) {
        newTuple->tuple_dimension = tuple_dim;
        newTuple->tuple = (char**) malloc(tuple_dim * sizeof(char*) );
    }
    return newTuple;
}
/*
 * Função idêntica à anterior, mas com uma assinatura diferente.
 */
struct tuple_t *tuple_create2(int tuple_dim, char **tuple) {
    if ( tuple_dim <= 0 || tuple == NULL)
        return NULL;
    
    struct tuple_t * newTuple = tuple_create(tuple_dim);
    //if it was allocated we just need to copy the tuple values
    if ( newTuple != NULL ) {
        int i;
        for ( i=0; i<newTuple->tuple_dimension; i++) {
            newTuple->tuple[i] = tuple[i] == NULL ? NULL :  strdup(tuple[i]);
        }
    }
    return newTuple;
}
struct tuple_t ** tuple_create_array(int tuples_num) {
    if ( tuples_num <= 0 ) return NULL;

    return (struct tuple_t **) malloc ( sizeof(struct tuple_t *) * tuples_num);
}
void tuple_array_destroy(struct tuple_t ** tuples, int num ) {
    if ( tuples != NULL ) {
        int i = 0;
        for ( i = 0; i < num; i++) {
            tuple_destroy(tuples[i]);
        }
        free(tuples);
    }
}

/*
 * Função que destrói um bloco de dados e liberta toda a memoria.
 */
void tuple_destroy(struct tuple_t *tuple) {
    if ( tuple != NULL && tuple->tuple != NULL ) {
        int i;
        for ( i = 0; i < tuple->tuple_dimension; i++ ) {
            if (tuple->tuple[i] != NULL ) {
                free(tuple->tuple[i]);
            }
        }
        free(tuple->tuple);
        free(tuple);
    }
}

/*
 * Função que duplica um tuplo. Quando se criam duplicados
 * é necessário efetuar uma CÓPIA dos dados (e não somente alocar a
 * memória necessária).
 */
struct tuple_t *tuple_dup (struct tuple_t *tuple) {
    if ( tuple == NULL || tuple->tuple == NULL)
        return NULL;
    
    //if tuple is valid
    return tuple_create2(tuple->tuple_dimension, tuple->tuple);
}


/*********   Implementation of tuple-private.h    **********/

/*
 * Method that returns the iElement of a given tuple.
 */
char * tuple_element ( struct tuple_t * tuple, int iElement ) {
    return tuple->tuple[iElement];
}

char * tuple_elem_str(struct tuple_t * tuple, int i) {
    return tuple_element(tuple,i) == NULL ? TUPLE_ELEM_NULL :  tuple_element(tuple,i);
}

/*
 * Method that returns the key of a given tuple.
 */
char * tuple_key (struct tuple_t * tuple ) {
    return tuple_element(tuple,0);
}

/*
 * Method that returns the size of a given tuple.
 */
int tuple_size ( struct tuple_t * tuple ) {
    return tuple->tuple_dimension;
}



int tuple_size_bytes ( struct tuple_t* tuple) {
    
    int nBytes = 0;
    // [dim][size_e1][bytes_e1][size_e2][bytes_e2][size_e3][bytes_e3]
    nBytes+= TUPLE_DIMENSION_SIZE;
    
    int i;
    for ( i = 0; i < tuple_size(tuple); i++) {
        //sums the number of bytes needed to alloc for each element of the tuple
        long elementSize = tuple_element(tuple,i) == NULL ? 1 : strlen(tuple_element(tuple,i));
        nBytes+= TUPLE_ELEMENTSIZE_SIZE + elementSize;
    }
    
    return nBytes;
}

int tuple_size_as_string (struct tuple_t* tuple) {
    int size = 0;
    
    int i;
    for ( i = 0; i < tuple_size(tuple); i++ ) {
        //each elem size is the "" and a space after
        size+= ( 1 + strlen(tuple_elem_str(tuple, i)) + 1 + 1);
    }
    
    return size;
}

//format has to be:
// [dim][size_e1][bytes_e1][size_e2][bytes_e2][size_e3][bytes_e3]
//    4       4             var         4               var          4          4
// total size is 4*5 + sum lenght of e1,e2,e3
int tuple_serialize(struct tuple_t *tuple, char **buffer) {
    
    if ( tuple == NULL)
        return FAILED;
    
    //bytes size needed to be alloc
    int buffer_size = tuple_size_bytes(tuple);
    
    //allocs memory
    *buffer = (char *) malloc(buffer_size );
    //to insert to the buffer
    int offset = 0;
    
    //1. insert tuple dimension
    int tuple_dim_htonl = htonl(tuple_size(tuple));
    //insert to buffer
    memcpy((buffer[0]+offset), &tuple_dim_htonl, TUPLE_DIMENSION_SIZE);
    //moves offset
    offset+=TUPLE_DIMENSION_SIZE;
    
    //serializes each element following the patter [elemSize][elemContent]
    int i;
    for ( i = 0; i < tuple_size(tuple); i++) {
        //gets tuple element information
        char* currentElementValue = tuple_element(tuple, i) == NULL ? TUPLE_ELEM_NULL : tuple_element(tuple, i);
    //    printf("value %d (%s) has size %lu\n",i, currentElementValue, strlen(currentElementValue));
        long currentElementSize = currentElementValue == NULL ? 1 : strlen(currentElementValue);
        
        // 1. first inserts element size
        int tuple_elementSizeI_htonl = htonl(currentElementSize);
        //insert to buffer
        memcpy((buffer[0]+offset), &tuple_elementSizeI_htonl, TUPLE_ELEMENTSIZE_SIZE);
        //moves offset
        offset+=TUPLE_ELEMENTSIZE_SIZE;
        //2. then inserts the string itself
       // printf("tuple_serialize > element %s has size %ld\n", currentElementValue, currentElementSize);
        memcpy((buffer[0]+offset), currentElementValue, currentElementSize);
        
        offset+=currentElementSize;
    }
    
    //to make sure its working
    assert(buffer_size == offset);
    
    return buffer_size;
}

void tuple_print ( struct tuple_t * tuple ) {
    if ( tuple == NULL || tuple->tuple == NULL || tuple->tuple_dimension <= 0)
        printf(" <tuplo nulo> ");
    else {
        printf("<%s,%s,%s>", tuple_element(tuple, 0), tuple_element(tuple, 1),tuple_element(tuple, 2));
    }
}

/*
 *  Gets a buffer with the format:
 *  [dim][size_e1][bytes_e1][size_e2][bytes_e2][size_e3][bytes_e3]
 */
struct tuple_t *tuple_deserialize(char *buffer, int size) {
    
    
    if ( buffer == NULL )
        return NULL;
    
    //offset to read from the buffer
    int offset = 0;
    
    //1. gets the tuple dim (first part of the buffer)
    int tupleSize_nl = 0;
    memcpy(&tupleSize_nl, buffer+offset, TUPLE_DIMENSION_SIZE );
    int tupleSize = ntohl(tupleSize_nl);
    
    //creates a tuple to create with the content from the buffer
    struct tuple_t * tuple = tuple_create(tupleSize);
    //moves  offset
    offset+=TUPLE_DIMENSION_SIZE;
    
    //2.gets first element size
    int i;
    for ( i = 0; i < tupleSize; i++ ) {
        //1. gets i element size
        int elementSize_nl = 0;
        memcpy(&elementSize_nl, buffer+offset, TUPLE_ELEMENTSIZE_SIZE );
        offset+= TUPLE_ELEMENTSIZE_SIZE;
        int elementSize = ntohl(elementSize_nl);
      //  printf("tuple_deserialize > element %d has size nl %d and host %d\n", i, elementSize_nl, elementSize);

        
        //memory security check !!!: if elementSize is bigger then space to
        // read from buffer operation is canceled
        if ( offset + elementSize > size) {
            tuple_destroy(tuple);
            return NULL;
        }
        
        //2. sets the i element value into the tuple
        char * elementValue = (char*) malloc(elementSize);
        
        memcpy(elementValue, (buffer+offset), elementSize);
        tuple->tuple[i] = strncmp(elementValue, TUPLE_ELEM_NULL, 1) == 0 ? NULL : strndup(elementValue, elementSize);
    //    printf("tuple[%d] is %s || elementValue is %s and size %d\n",i, tuple->tuple[i], elementValue, elementSize);
        free(elementValue);
        offset+=elementSize;
    }
    
    //returns it
    return tuple;
}


char * tuple_to_string( struct tuple_t * tuple ) {
    int size = 0;
    int i;
    for (i = 0; i < tuple_size(tuple); i++)
        size+= strlen(tuple_elem_str(tuple, i)) + 3 ; //+3 is the two " " and the space
    
    char * tuple_string = malloc (size);
    sprintf(tuple_string, "\"%s\" \"%s\" \"%s\"", tuple_elem_str(tuple,0), tuple_elem_str(tuple,1), tuple_elem_str(tuple,2));
    tuple_string[size-1] = '\0';
    
    return tuple_string;
}
//


/*
 *  Creates a tuple from user input.
 *
 */
struct tuple_t* create_tuple_from_input (const char *user_input) {
    
    char *token;
    char *search = "\"";
    char* tuple_data[3];
    struct tuple_t * tuple_to_send;
    
    char * user_input_p = strdup(user_input);
    
    // Avanca para o primeiro elemento (salta o nome da operacao)
    strtok(user_input_p, search);
    
    int i;
    for (i = 0; i< TUPLE_DIMENSION; i++ ) {
        token = strtok(NULL, search);
        tuple_data[i] = strcmp(token, TUPLE_ELEM_NULL) == 0 ? NULL : strdup (token);
        strtok(NULL, search);

    }
    //creates new tuple to send
    tuple_to_send = tuple_create2(TUPLE_DIMENSION, tuple_data);
    
    free(user_input_p);
    return tuple_to_send;
}

