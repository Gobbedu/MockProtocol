#include "ConexaoRawSocket.h"
#include "cliente.h"
#include "Packet.h"


// todos no cliente podem ver a conexao socket
int soquete;


void cds(void)
{
    /* errno: 
        A - No such file or directory : 2
        B - Permission denied : ? 
    */
}

void testes(void){
    double time;
    int bytes, seq;
    clock_t start, end;
    unsigned char buffer[TAM_PACOTE];
    unsigned char *packet = make_packet(0, OK, NULL);
    if(!packet){
        fprintf(stderr, "ERRO NA CRIACAO DO PACOTE\n");
        exit(0);
    }

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100;
    int min = 3;
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    setsockopt(soquete, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);
    setsockopt(soquete, SOL_SOCKET, SO_RCVLOWAT, &min, sizeof(int));
    setsockopt(soquete, SOL_SOCKET, SO_SNDLOWAT, &min, sizeof(int));

    bytes = send(soquete, packet, strlen((char *)packet), 0);           // envia packet para o socket
    if(bytes<0)                                                         // pega erros, se algum
        printf("error: %s\n", strerror(errno));  
    printf("%d bytes enviados no socket %d\n", bytes, soquete);


    start = clock();
    while(1){
        // sniff sniff, recebeu pacote nosso?
        bytes = recv(soquete, buffer, TAM_PACOTE, MSG_PEEK);
        if( bytes < 0)
            printf("\nrecv peek : %s\n", strerror(errno));

        if(bytes>0 && is_our_packet((unsigned char *)buffer))
        {
            bytes = recv(soquete, buffer, TAM_PACOTE, 0);
            if(bytes < 0)
                printf("\nrecv 0 :%s\n", strerror(errno));

            seq = get_packet_sequence(buffer);
            printf("SEQUENCIA: %d com %d bytes\n", seq, bytes);
            if(seq != 0){
                printf("DEU CERTO!!!!\n");      
                break;
            }
        }   
        
        // se demoro timeout!
        end = clock();
        time = ((double) (end - start)) / CLOCKS_PER_SEC;
        printf("\rtime: %10f", time);
        if(time >= 0.1){
            printf("\nTIMEOUT!\n");
            break;
        }
    }
    return;  
}


int main(){
    char pwd[PATH_MAX];
    char comando[COMMAND_BUFF];
    soquete = ConexaoRawSocket("lo");  // abre o socket -> lo vira ifconfig to pc que manda

    // testes();
    while(1){
        if(getcwd(pwd, sizeof(pwd)))    // se pegou pwd, (!NULL)
            printf(GREEN "limbo@anywhere" RESET ":" BLUE "%s" RESET "$ ", pwd);

        fgets(comando, COMMAND_BUFF, stdin);
        client_switch(comando);
    } 

    close(soquete);                                        // fecha socket
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
        parametro = comando+3;                          // remove "cdc"
        ret = chdir((parametro+strspn(parametro, " ")));
        if(ret == -1)
            printf("ERRO: %s, errno: %d  parametro: (%s)\n", strerror(errno), errno, parametro);
    }
    else if(strncmp(comando, "mkdirc", 6) == 0){
        comando[5] = ' ';                               // remove 'c' : mkdirc_[]-> mkdir__[]
        ret = system(comando);
        if(ret == -1)
            printf("ERRO\n");
    }
    else if(strncmp(comando, "cds", 3) == 0){
        gera_pedido(parametro, CD);
    }
    else if(strncmp(comando, "lss", 3) == 0){   
        gera_pedido(parametro, LS);
    }
    else if(strncmp(comando, "mkdirs", 6) == 0){
        gera_pedido(parametro, MKDIR);
    }
    else if (strncmp(comando, "get", 3) == 0){
        gera_pedido(parametro, GET);
        // fazendo função para tratar o get
        // get();
    }
    else if (strncmp(comando, "put", 3) == 0){
        gera_pedido(parametro, PUT);
    }

    else if(strncmp(comando, "exit", 4) == 0){      // sair com estilo
        printf(RED "CLIENT TERMINATED\n" RESET);      
        exit(0);
    }
    else{
        if(comando[0] != 0)     // diferente de um enter
            printf("comando invalido: %s\n", comando);
    }
}
void gera_pedido(char * dados, int tipo){
    // char *complemento = (char*)malloc(64-sizeof(dados));
    // memset(complemento, '0', sizeof(complemento));

    int bytes;

    unsigned char* packet = make_packet(0, tipo, dados);
    if(!packet) // se pacote deu errado
        return;

    // len of packet must be strlen(), sizeof doesnt work
    bytes = send(soquete, packet, strlen((char *)packet), 0);          // envia packet para o socket
    if(bytes<0)                                                     // pega erros, se algum
        printf("error: %s\n", strerror(errno));                     // print detalhes do erro

    printf("%d bytes enviados no socket %d\n", bytes, soquete);

    read_packet(packet);
    free(packet);
}

/*
// função para tratar o get
void get(){
    unsigned char buffer[68];                   // buffer tem no maximo 68 bytes
    int bytes;

    bytes = recv(soquete, buffer, sizeof(buffer), 0);      // recebe dados do socket
    // buffer[sizeof(buffer)] = '\0';                      // fim da string no buffer

    if(bytes>0 && is_our_packet(buffer))
    {   // processa pacote se eh nosso pacote
        if(get_packet_type(buffer) == NACK){

        }
        else if(get_packet_type(buffer) == ERRO){

        }
        else if(get_packet_type(buffer) == DESC_ARQ){

        }
    }
}
*/