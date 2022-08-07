#include "servidor.h"
#include "Packet.h"
#include "ConexaoRawSocket.h"

// global para servidor
int soquete;
/* sniff sniff */
int main()
{
    int bytes;
    unsigned char buffer[TAM_PACOTE];       // mensagem de tamanho constante
    // soquete = ConexaoRawSocket("lo");
    soquete = ConexaoRawSocket("enp2s0f1"); // abre o socket -> lo vira ifconfig to pc que recebe

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    setsockopt(soquete, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

    while(1){
        bytes = recv(soquete, buffer, sizeof(buffer), 0);           // recebe dados do socket
        if(bytes>0 && is_our_packet(buffer))
        {   // processa pacote se eh nosso pacote
            read_packet(buffer);
            server_switch(buffer);
        }
    }

    close(soquete);
    printf("servidor terminado\n");
}

void server_switch(unsigned char* buffer)
{
    int tipo_lido = get_packet_type(buffer);

	switch (tipo_lido)
	{
        case OK:
            // funcao q redireciona
            break;
        case NACK:
            // funcao q redireciona
            break;
        case ACK:
            // funcao q redireciona
            break;
        case CD:
            cdc(buffer);
            break;
        case LS:
            // funcao q redireciona
            break;
        case MKDIR:
            mkdirc(buffer);
            break;
        case GET:
            get(buffer);
            break;
        case PUT:
            // funcao q redireciona
            break;
        case ERRO:
            // funcao q redireciona
            break;
        case DESC_ARQ:
            // funcao q redireciona
            break;
        case DADOS:
            // funcao q redireciona
            break;
        case FIM:
            // funcao q redireciona
            break;
        case SHOW_NA_TELA:
            // funcao q redireciona
            break;

		default:
			break;
	}
}


void cdc(unsigned char* buffer){
    int resultado, bytes, ret;
    unsigned char *resposta;
    char *cd, flag[1];

    cd = (char *) get_packet_data(buffer);
    char *d = malloc(strcspn(cd, " ")*sizeof(char));    // remove espaco no final da mensagem, se tem espaco da ruim 
    char pwd[PATH_MAX];                                 // "cd ..     " -> "..    " nn existe 
    strncpy(d, cd, strcspn(cd, " "));

    if(getcwd(pwd, sizeof(pwd)))printf("before: %s\n", pwd);
    ret = chdir(d);
    if(getcwd(pwd, sizeof(pwd)))printf("after: %s\n", pwd);

    if(ret == -1){
        printf("erro foi : %s\n", strerror(errno));
        switch (errno){
            case 2:
                flag[0] = dir_nn_E;
                break;
            case 13:
                flag[0] = sem_permissao;
                break;
            default:
                flag[0] = '?';
                break;
        };
    }
    else{
        resultado = OK;
    }
    resposta = make_packet(sequencia(), resultado, flag, resultado == ERRO);
    if(!resposta) return;   // se pacote deu errado

    bytes = send(soquete, resposta, TAM_PACOTE, 0);                 // envia packet para o socket
    if(bytes<0)                                                     // pega erros, se algum
        printf("error: %s\n", strerror(errno));                     // print detalhes do erro

    free(resposta);
    free(cd);
    free(d);
}

// trata_tipos: case switch p/ cada ENUM uma funcao
// Tipo == ls -> funcao_ls(dados){ dados = a ou l ... } .... 

void mkdirc(unsigned char* buffer){
    int bytes, ret, resultado;
    unsigned char *resposta;
    char *mkdir, flag[1];

    mkdir = get_packet_data(buffer);
    ret = system(mkdir);

    if(ret != 0){        
        resultado = ERRO;
        switch (ret){       // errno da 11, que nao eh erro esperado
            case 256:       // ret devolve ($?)*256 de mkdir em system(mkdir)
                flag[0] = dir_ja_E;
                break;
            case 13*256:    // nunca acontece 
                flag[0] = sem_permissao;
                break;
            default:
                flag[0] = '?';
                break;
        };
        printf("erro %d foi : %s ; flag (%s)\n",errno, strerror(errno), flag);
    }
    else{
        resultado = OK;
    }

    resposta = make_packet(sequencia(), resultado, flag, resultado == ERRO);    // mostra flag somente se tem erro, bytes_dados = 1
    if(!resposta) return;   // se pacote deu errado

    bytes = send(soquete, resposta, TAM_PACOTE, 0);     // envia packet para o socket
    if(bytes<0)                                         // pega erros, se algum
        printf("error: %s\n", strerror(errno));         // print detalhes do erro

    free_packet(resposta);
    free(mkdir);
}

void get(unsigned char *buffer){
    // int resultado;
    // unsigned char *resposta;
    // char *get, flag[1];
    // int bytes;
    // int ret;

    // get = (char *) get_packet_data(buffer);
    
}