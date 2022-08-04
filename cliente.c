#include "ConexaoRawSocket.h"
#include "cliente.h"
#include "Packet.h"


int main(){
    char pwd[PATH_MAX];
    char comando[COMMAND_BUFF];
    while(1){
        if(getcwd(pwd, sizeof(pwd)))    // se pegou pwd, (!NULL)
            printf(GREEN "limbo@anywhere" RESET ":" BLUE "%s" RESET "$ ", pwd);

        fgets(comando, COMMAND_BUFF, stdin);
        client_switch(comando);
    } 
    return 0;
}


void client_switch(char* comando){
    // final do comando c : client
    // final do comando s : server
    int ret;
    char *parametro = comando;      // = comando pro make nn reclama, dpois tiro
    comando[strcspn(comando, "\n")] = 0;                // remove new line


	if(strncmp(comando, "lsc", 3) == 0){
        comando[2] = ' ';                               // remove 'c' : lsc -> ls_
        ret = system(comando);
        if(ret == -1)
            printf("ERRO\n");
    }
    else if(strncmp(comando, "cdc", 3) == 0){
        parametro = comando+4;                          // remove "cdc_"
        ret = chdir(parametro);
        if(ret == -1)
            printf("ERRO: %s\n", strerror(errno));
    }
    else if(strncmp(comando, "mkdirc", 6) == 0){
        comando[5] = ' ';                               // remove 'c' : mkdirc_[]-> mkdir__[]
        ret = system(comando);
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
        // fazendo função para tratar o get
        // get();
    }
    else if (strcmp(comando, "put") == 0){
        gera_pedido(parametro, 10);
    }
    else if(strncmp(comando, "exit", 4) == 0){
        printf(RED "CLIENT TERMINATED\n");
        exit(0);
    }
    else{
        if(comando[0] != 0)
            printf("comando invalido: %s\n", comando);
    }
}
void gera_pedido(char * dados, int tipo){
}
// pro make nn reclamar, dpois vejo
/*
    char *complemento = (char*)malloc(64-sizeof(dados));
    memset(complemento, '0', sizeof(complemento));

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

// função para tratar o get
void get(){
    int sock = ConexaoRawSocket("lo");

    unsigned char buffer[68];                   // buffer tem no maximo 68 bytes
    int bytes;

    bytes = recv(sock, buffer, sizeof(buffer), 0);      // recebe dados do socket
    // buffer[sizeof(buffer)] = '\0';                      // fim da string no buffer

    if(bytes>0 && is_our_packet(buffer))
    {   // processa pacote se eh nosso pacote
        if(get_packet_data(buffer) == NACK){

        }
        else if(get_packet_data(buffer) == ERRO){

        }
        else if(get_packet_data(buffer) == DESC_ARQ){

        }
    }

    close(sock);
}
*/