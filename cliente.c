#include "ConexaoRawSocket.h"
#include "cliente.h"
#include "Packet.h"

int main(){
    char * comando = (char *) calloc(1, sizeof(char));
    char * parametro = (char *) calloc(1, sizeof(char));
    while(1){
        memset(parametro, '\0', sizeof(char));

        fscanf(stdin, "%s", comando);
        fscanf(stdin, "%s", parametro);


        type_process_client(comando, parametro);
    }
    return 0;

    free(comando);
    free(parametro);
}

void type_process_client(char* comando, char* parametro){
    int tam = strlen(comando) + strlen(parametro);
    char final[tam];
    char *aux = (char*)calloc(1, 64-sizeof(parametro));
    strcat(parametro, aux);

    // final c = client, final s = servidor
	if(strcmp(comando, "lsc") == 0){
        strcat(strcpy(final, "ls "), parametro);
        system(final);
    }
    else if(strcmp(comando, "cdc") == 0){
        strcat(strcpy(final, "cd "), parametro);
        printf("cd: %s\n", final);
        system(final);
    }
    else if(strcmp(comando, "mkdirc") == 0){
        strcat(strcpy(final, "mkdir "), parametro);
        system(final);
    }
    else if(strcmp(comando, "lss") == 0){
        strcat(parametro, aux);
        gera_pedido(parametro, 7);
    }
    else if(strcmp(comando, "cds") == 0){
        strcat(parametro, aux);
        gera_pedido(parametro, 6);
    }
    else if(strcmp(comando, "mkdirs") == 0){
        strcat(parametro, aux);
        gera_pedido(parametro, 8);
    }
}

void gera_pedido(char * dados, int tipo){
    int sock = ConexaoRawSocket("lo");  // abre o socket -> lo vira ifconfig to pc que manda
    int bytes;

    unsigned char* packet = make_packet(0, tipo, dados);
    if(!packet) // se pacote deu errado
        return;

    // len of packet must be strlen(), sizeof doesnt work
    bytes = send(sock, packet, strlen((char *)packet), 0);          // envia packet para o socket
    if(bytes<0)                                                     // pega erros, se algum
        printf("error: %s\n", strerror(errno));                     // print detalhes do erro

    printf("%d bytes enviados no socket %d\n", bytes, sock);

    read_packet(packet);
    free(packet);

    close(sock);                                        // fecha socket
}