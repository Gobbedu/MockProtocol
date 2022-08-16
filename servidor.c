#include "servidor.h"
#include "Packet.h"
#include "ConexaoRawSocket.h"
#include <sys/stat.h>

// - [] CHECAR FUNCAO check_packet, MUDA CALCULO DA PARIDADE int paradis = 1;
// - [] (TODO) VERIFICAR SE SEQUENCIA CORRETA, ENVIA DE VOLTA SEQUENCIA NACK 
// - [] tratar da sequencia de mkdirc e cdc, caso algo de errado, senao sequencia nunca emparelha dnv
// - [] tratar next_seq do cliente e do servidor, update quando aceita
// - [] sequencia do GET ta quebrada

// global para servidor
int soquete;
int serv_seq = 0;   // current server sequence
int nxts_cli = 0;   // expected next client sequence


/* sniff sniff */
int main()
{
    int bytes;
    // int check;
    unsigned char *resposta, buffer[TAM_PACOTE];       // mensagem de tamanho constante
    soquete = ConexaoRawSocket("lo");
    // soquete = ConexaoRawSocket("enp2s0f1"); // abre o socket -> lo vira ifconfig to pc que recebe

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    setsockopt(soquete, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

    while(1){
        bytes = recv(soquete, buffer, sizeof(buffer), 0);           // recebe dados do socket

        // VERIFICA //
        if (bytes<=0) continue;                 // se erro ou vazio, ignora
        if (!is_our_packet(buffer)) continue;   // se nao eh nosso pacote, ignora
        if (!check_sequence((unsigned char*) buffer, nxts_cli)){
            // sequencia diferente: ignora
            printf("server expected %d but got %d as a sequence\n", nxts_cli, get_packet_sequence(buffer));
            continue;
        }
        
        // PROCESSA //
        if (!check_parity(buffer)){             
            // paridade diferente: NACK
            resposta = make_packet(sequencia(), NACK, NULL, 0);
            send(soquete, resposta, TAM_PACOTE, 0);  
            continue;                                         
        }

        // tudo ok, redireciona comando
        nxts_cli = (nxts_cli+1)%MAX_SEQUENCE;
        read_packet(buffer);
        server_switch(buffer);
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
        resultado = ERRO;
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
        printf("erro %d foi : %s ; flag (%c)\n",errno, strerror(errno), flag[0]);
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
        printf("erro %d foi : %s ; flag (%c)\n",errno, strerror(errno), flag[0]);
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
    int bytes, resultado;
    unsigned char *resposta;
    char *get, *flag;

    get = get_packet_data(buffer);
    
    // MEXER AQUI !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    FILE *arquivo;

    arquivo = fopen(get, "r");

    if(arquivo == NULL){        
        resultado = ERRO;
        flag = (char*)malloc(sizeof(char));
        switch (errno){       // errno da 11, que nao eh erro esperado
            case 2:       // ret devolve ($?)*256 de mkdir em system(mkdir)
                flag = (char*) &arq_nn_E;
                break;
            case 13*256:    // nunca acontece 
                flag = (char*) &sem_permissao;
                break;
            default:
                flag = "?";
                break;
        };
        printf("erro %d foi : %s ; flag (%s)\n",errno, strerror(errno), flag);
    }
    else{
        resultado = DESC_ARQ;
        stat(get, &st);
        flag = calloc(10, sizeof(char));
        sprintf(flag, "%ld", st.st_size);
    }

    int len_msg = (resultado == ERRO) ? 1 : strlen(flag);
    resposta = make_packet(sequencia(), resultado, flag, resultado);
    if(!resposta) return;   // se pacote deu errado

    bytes = send(soquete, resposta, TAM_PACOTE, 0);     // envia packet para o socket
    if(bytes<0)                                         // pega erros, se algum
        printf("error: %s\n", strerror(errno));         // print detalhes do erro

    // FAZER AQUI A LOGICA DA JANELA DESLIZANTE
}

// atualiza e retorna proxima sequencia
unsigned int sequencia(void)
{
    int now = serv_seq;
    serv_seq = (serv_seq+1)%MAX_SEQUENCE;
    return now;
}