#include "ConexaoRawSocket.h"
#include "cliente.h"
#include "Packet.h"


// todos no cliente podem ver a conexao socket
int soquete;
int sequencia_cliente, sequencia_servidor, lastseq_cliente, lastseq_servidor;

int seq_cli(void){
    lastseq_cliente = sequencia_cliente;
    sequencia_cliente = (sequencia_cliente+1)%LIMITE_SEQ;
    return sequencia_cliente;
}


void testes(void){
    int bytes, seq;
    unsigned char buffer[TAM_PACOTE];
    unsigned char *packet = make_packet(0, CD, "..");
    if(!packet){
        fprintf(stderr, "ERRO NA CRIACAO DO PACOTE\n");
        exit(0);
    }

    bytes = send(soquete, packet, strlen((char *)packet), 0);           // envia packet para o socket
    if(bytes<0)                                                         // pega erros, se algum
        printf("error: %s\n", strerror(errno));  
    printf("%d bytes enviados no socket %d\n", bytes, soquete);

    int num_ruim = 0;
    while(1){
        // sniff sniff, recebeu pacote nosso?
        bytes = recv(soquete, buffer, TAM_PACOTE, 0);
        if( bytes < 0){
            printf("recv peek : %s\n", strerror(errno));
            num_ruim++;
        }
        // printf("%d\n", num_ruim);
        if(bytes>0 && is_our_packet((unsigned char *)buffer))
        {
            seq = get_packet_sequence(buffer);
            printf("TIPO: %s com dado: %s \n", get_type_packet(buffer), get_packet_data(buffer));
            if(seq != 0){
                printf("DEU CERTO!!!!\n");      
                break;
            }
        }   

        if(num_ruim>3)
        {
            printf("TIMEOUT!\n");
            break;
        }
    
    }
    return;  
}


int main(){
    char pwd[PATH_MAX];
    char comando[COMMAND_BUFF];

    lastseq_cliente = 0;
    sequencia_cliente = 0;
    soquete = ConexaoRawSocket("lo");        // abre o socket -> lo vira ifconfig to pc que manda
    // soquete = ConexaoRawSocket("enp1s0f1");     // abre o socket -> lo vira ifconfig to pc que manda

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    setsockopt(soquete, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

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

	if(strncmp(comando, "lsc", 3) == 0)
    {
        comando[2] = ' ';                               // remove 'c' : lsc -> ls_
        ret = system(comando);
        if(ret == -1)
            printf("ERRO\n");
    }
    else if(strcmp(comando, "teste") == 0 ){testes();}

    else if(strncmp(comando, "cdc", 3) == 0)
    {
        parametro = comando+3;                          // remove "cdc"
        ret = chdir((parametro+strspn(parametro, " ")));
        if(ret == -1)
            printf("ERRO: %s, errno: %d  parametro: (%s)\n", strerror(errno), errno, parametro);
    }
    else if(strncmp(comando, "mkdirc", 6) == 0)
    {
        comando[5] = ' ';                               // remove 'c' : mkdirc_[]-> mkdir__[]
        ret = system(comando);
        if(ret == -1)
            printf("ERRO\n");
    }
    else if(strncmp(comando, "cds", 3) == 0)
    {
        cds(comando);
    }
    else if(strncmp(comando, "lss", 3) == 0)
    {   

    }
    else if(strncmp(comando, "mkdirs", 6) == 0)
    {
        
    }
    else if (strncmp(comando, "get", 3) == 0)
    {
        get(comando);
    }
    else if (strncmp(comando, "put", 3) == 0)
    {
        
    }
    else if(strncmp(comando, "exit", 4) == 0)
    {      // sair com estilo
        printf(RED "CLIENT TERMINATED\n" RESET);      
        exit(0);
    }
    else
    {
        if(comando[0] != 0)     // diferente de um enter
            printf("comando invalido: %s\n", comando);
    }
}

void cds(char *comando){
    /* errno: 
        A - No such file or directory : 2
        B - Permission denied : 13
    */
    int bytes, timeout, seq, ok, lost_conn;
    unsigned char resposta[TAM_PACOTE];

    /* filtra pacote, envia somente parametro do cd */
    comando += 3;                       // remove "cds"
    comando += strspn(comando, " ");    // remove ' '  no inicio do comando

    /* cria pacote com parametro para cd no server */
    unsigned char *packet = make_packet(seq_cli(), CD, comando);
    if(!packet)
        fprintf(stderr, "ERRO NA CRIACAO DO PACOTE\n");

    // read_packet(packet);

    timeout = ok = lost_conn = 0;
    // exit if ok received or 3 timeouts
    while(!ok && lost_conn<3){ 

        bytes = send(soquete, packet, TAM_PACOTE, 0);       // envia packet para o socket
        if(bytes < 0){                                      // pega erros, se algum
            printf("error: %s\n", strerror(errno));  
        }
        printf("%d bytes enviados no socket %d\n", bytes, soquete);
        // recv(soquete, packet, TAM_PACOTE, 0); //pra lidar com loop back

        /* said do loop soh se server responde ok */
        timeout = 0;
        while(1)
        {   
            if(timeout == 3){
                lost_conn++;
                break;
            }
            bytes = recv(soquete, resposta, TAM_PACOTE, 0);
            // if(errno == EAGAIN)    // nao tem oq ler
            if(errno)
            {
                printf("recv error : %s; errno: %d\n", strerror(errno), errno);
                timeout++;
            }

            seq = get_packet_sequence(resposta);
            if( bytes>0 && 
                is_our_packet((unsigned char *)resposta) && 
                sequencia_cliente != seq  
            ){
                switch (get_packet_type(resposta))
                {
                case OK:
                    ok = 1;  
                    printf("SEQUENCIA: %d com %d bytes\n", seq, bytes);
                    printf("mensagem: %s\n", get_packet_data(resposta));
                    printf("DEU CERTO ?!!!!\n");    
                    return;

                case NACK:
                    break;  // exit response loop & re-send 

                case ERRO:
                    printf("erro: servidor respondeu %s\n", get_packet_data(resposta));
                    return;
                }
            }  
        }
    }

    if(!(timeout<3))
        printf("Erro de comunicacao, servidor nao responde :(\n");
    if(ok)
        printf("Comando CD aceito no server\n");
}

// função para tratar o get
void get(char *comando){
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