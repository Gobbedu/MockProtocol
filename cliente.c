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
    char final[64];

    // final c = client, final s = servidor
	if(strcmp(comando, "lsc") == 0){
        strcat(strcpy(final, "ls "), parametro);
        int ret = system(final);
        if(ret == -1)
            printf("ERRO\n");
    }
    else if(strcmp(comando, "cdc") == 0){
        int ret = chdir(parametro);
        if(ret == -1)
            printf("ERRO: %s\n", strerror(errno));
    }
    else if(strcmp(comando, "mkdirc") == 0){
        strcat(strcpy(final, "mkdir "), parametro);
        int ret = system(final);
        if(ret == -1)
            printf("ERRO\n");
    }
    else if(strcmp(comando, "cds") == 0){
        gera_pedido(parametro, 6);
    }
    else if(strcmp(comando, "lss") == 0){
        gera_pedido(parametro, 7);
    }
    else if(strcmp(comando, "mkdirs") == 0){
        gera_pedido(parametro, 8);
    }
    else if (strcmp(comando, "get") == 0){
        gera_pedido(parametro, 9);
    }
    else if (strcmp(comando, "put") == 0){
        gera_pedido(parametro, 10);
    }
}

void gera_pedido(char * dados, int tipo){
    char *complemento = (char*)malloc(64-sizeof(dados));
    memset(complemento, '0', sizeof(aux));

    int sock = ConexaoRawSocket("lo");  // abre o socket -> lo vira ifconfig to pc que manda
    int bytes;

    unsigned char* packet = make_packet(0, tipo, dados, complemento);
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