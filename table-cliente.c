#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h> //For errno - the error number
#include <netdb.h> //hostent
#include <signal.h>

#include "inet.h"
#include "table.h"
#include "network_cliente.h"
#include "general_utils.h"
#include "network_utils.h"
#include "client_stub.h"

/*
 *  Acts according with user command
 *  returns 0 if success, -1 otherwise
 */
int proceed_with_command (int opcode, struct rtable_t *rtable_to_consult, void *message_content){

    int task = TASK_FAILED;
    int keep_tuples = -1; //apenas para inicializar
    int one_or_all = -1; //apenas para inicializar
    
    switch (opcode) {
        case OC_SIZE: //TABLE SIZE REQUEST
            task = rtable_size(rtable_to_consult);
            if (task == TASK_FAILED){
                return TASK_FAILED;
            }
//            puts("proceed_with_command > OC_SIZE > PASSEI AQUI!");
            break;
            
        case OC_OUT: //TABLE OUT REQUEST
            task = rtable_out(rtable_to_consult, message_content);
            if (task == TASK_FAILED){
                return TASK_FAILED;
            }
//            puts("proceed_with_command > OC_OUT > PASSEI AQUI!");
            break;
            
        case OC_IN: //TABLE IN REQUEST
            keep_tuples = NO;
            one_or_all = NO; //retira apenas 1?
            task = TASK_SUCCEEDED;
//            puts("proceed_with_command > OC_IN > PASSEI AQUI!");
            break;
            
        case OC_IN_ALL: //TABLE IN_ALL REQUEST
            keep_tuples = NO;
            one_or_all = YES; //retira todos?
            task = TASK_SUCCEEDED;
//            puts("proceed_with_command > OC_IN_ALL > PASSEI AQUI!");
            break;
            
        case OC_COPY: //TABLE COPY REQUEST
            keep_tuples = YES;
            one_or_all = NO; //copia apenas 1?
            task = TASK_SUCCEEDED;
//            puts("proceed_with_command > OC_COPY > PASSEI AQUI!");
            break;
            
        case OC_COPY_ALL: //TABLE COPY_ALL REQUEST
            keep_tuples = YES;
            one_or_all = YES; //copia todos?
            task = TASK_SUCCEEDED;
//            puts("proceed_with_command > OC_COPY_ALL > PASSEI AQUI!");
            break;
            
        default:
            task = TASK_FAILED;
//            puts("proceed_with_command > DEFAULT > PASSEI AQUI!");
            break;
    }
    
    //GETTER REQUESTS
    if ((opcode != OC_SIZE ||opcode != OC_OUT) && ((one_or_all != -1 ) && (keep_tuples != -1))){
        struct tuple_t **received_tuples; //será que tem de ser inicializado?
        received_tuples = rtable_get(rtable_to_consult, message_content, keep_tuples, one_or_all);
    }
    
    return task;
}

/*
 * Processes user command
 * Returns 0 if sucess, -1 otherwise
 */
int process_command (const char* command, struct rtable_t* rtable_to_consult){
    
    int task = TASK_FAILED;
    int opcode = find_opcode(command);
    int ctype = assign_ctype(opcode);
    
    void * message_content = NULL;
    
    //defines message type content - CT_TUPLE
    if (ctype == CT_TUPLE){
        message_content = create_tuple_from_input(command);
    }

    //defines message type content - CT_RESULT
    if( ctype == CT_RESULT){
        int resultValue = 0;
        message_content = &resultValue;
    }
    
    task = proceed_with_command (opcode, rtable_to_consult, message_content);
    
    return task;
}

/*
 *  Message if port number is invalid
 */
void invalid_client_input () {
    puts("\n\n####### SD15-CLIENT ##############");
    puts("Sorry, your input was not valid.");
    puts("Uso: ./SD15_CLIENT <servidor>:<porto>");
    puts("Exemplo de uso 1: ./SD15_CLIENT 10.10.10.10:1250");
    puts("Exemplo de uso 2: ./SD15_CLIENT wwww.example.com:1250");
    puts("####### SD15-CLIENT ##############\n\n");
}

/*
 *  Message if command is invalid
 */
void invalid_command () {
    puts("\n\n####### SD15-CLIENT ##############");
    puts("Sorry, your command was not valid.");
    puts("IN | IN_ALL | COPY | COPY_ALL | OUT \"elem1\" \"elem2\" \"elem3\"");
    puts("SIZE | QUIT");
    puts("####### SD15-CLIENT ##############\n\n");
}

/*
 * tests input number of arguments
 */
int test_input(int argc){
    if (argc != 2){
        invalid_client_input();
        return TASK_FAILED;
    }
    return TASK_SUCCEEDED;
}

int main(int argc , char *argv[]) {

    int task = TASK_SUCCEEDED;
    
    /* 0. SIGPIPE Handling */
    struct sigaction s;
    //what must do with a signal - ignore
    s.sa_handler = SIG_IGN;
    //set what to do when gets the SIGPIPE
    sigaction(SIGPIPE, &s, NULL);
    
    /* 1. Testar input de utilizador*/
    if ( test_input (argc) == TASK_FAILED ) return TASK_FAILED;
    
    //to keep the active
    int keepGoing = YES;
    //para guardar o comando do utilizador
    char input [MAX_MSG];
    
    //the remote table to consult
    struct rtable_t *rtable_to_consult;
    
    //the server address from input
    char * server_address_and_port = strdup(argv[1]);
    
    /* 2. BINDS WITH RTABLE_TO_CONSULT */
    rtable_to_consult = rtable_bind(server_address_and_port);
    
    //verifica se o rtable_bind funcionou
    if (rtable_to_consult == NULL){
//        puts ("TABLE_CLIENT > ERROR WHILE CONNECTING TO RTABLE");
        task = TASK_FAILED;
        }
    
    //ligação foi bem sucessida
    if (task == TASK_SUCCEEDED){
        
        /* 3. LÊ COMANDO DO UTILIZADOR E FAZ CONSULTAS À TABELA */
        while ( keepGoing ) {
            
            puts("\n---- Introduzir comando ----");
            //reads user command
            if (fgets (input, MAX_MSG-1, stdin) == NULL) {
                free(server_address_and_port);
                return TASK_FAILED;
            }
            
            // Processes user command according with objectives - OUT, GET, SIZE
            int command_opcode = find_opcode(input);
            
            // User wants to quit
            if (command_opcode == OC_QUIT) {
                keepGoing = NO;
            }
            
            // User instruction is wrong
            else if ( command_opcode == OC_DOESNT_EXIST ) {
                invalid_command();
            }
            
            else{
                /******* O comando foi correcto e vai proceder à consulta da tabela *******/
                //            puts("TABLE_CLIENT > STARTING TO PROCESS USER COMMAND!");
                task = process_command(input, rtable_to_consult); //processa o pedido do utilizador
 
                //se pedido falhou e não é para repetir pedido tenta terminar de forma controlada
                if (task == TASK_FAILED){
                    keepGoing = NO; //para sair de forma controlada apesar de erro de ligação?
                    puts("\nFAILED TO CONSULT TABLE!/n");
                }
                //            puts("TABLE_CLIENT > ENDED TO PROCESS USER COMMAND!");
            } //termina o processamento do user_command
        } //sai do while(keepGoing)
    }
    
    //para uma terminação controlada
    puts("A terminar sessão...");
    sleep(2); //para fica mais bonito
    
    free(server_address_and_port);

    int unbind = TASK_SUCCEEDED;
    
    if (rtable_to_consult != NULL){
        unbind = rtable_unbind(rtable_to_consult);
    }

    if (unbind == TASK_FAILED){
        puts("TABLE_CLIENT > Unable to unbind!");
        return TASK_FAILED;
    }

    puts("\nSessão terminada!\n");
    puts("SEE YOU LATER ALLIGATOR!");
    //termina processo com sinal de successo
    return task;
}