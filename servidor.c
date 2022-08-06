#include "servidor.h"
#include "Packet.h"
#include "ConexaoRawSocket.h"

// global para servidor
int last_seq;
int soquete;
int sequencia_servidor;
int sequencia_cliente;
/* sniff sniff */
int main()
{
    int bytes, seq;
    unsigned char buffer[TAM_PACOTE];                   // buffer tem no maximo 68 bytes
    sequencia_servidor = 0;
    // abre o socket -> lo vira ifconfig to pc que recebe
    soquete = ConexaoRawSocket("lo");

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    setsockopt(soquete, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

    /* gera loop no send/recv quando client acaba
        todo: corrigir 
    */
    while(1){
        bytes = recv(soquete, buffer, sizeof(buffer), 0);           // recebe dados do socket
        if(bytes>0 && is_our_packet(buffer))
        {   // processa pacote se eh nosso pacote
            buffer[bytes]=(buffer[bytes]==69)?0:buffer[bytes];      // WORKAROUND remove append 'E' do recv/send
            seq = get_packet_sequence(buffer);
            read_packet(buffer);
            server_switch(buffer);
        }
    }

    close(soquete);
    printf("servidor terminado\n");
}

// check_paridade_vertical sobre o campo Dados -> NACK ou type_process

void server_switch(unsigned char* buffer)
{
    int resultado;
    unsigned char *resposta;
    char *cd, flag[1];
    int bytes, tipo_lido = get_packet_type(buffer);
    int ret;

	switch (tipo_lido)
	{
        case OK:
            resposta = make_packet(sequencia_servidor, OK, NULL);
            if(!resposta){
                fprintf(stderr, "ERRO NA CRIACAO DO PACOTE\n");
                exit(0);
            }
            sequencia_servidor++;
            // read_packet(resposta);
            bytes = send(soquete, resposta, TAM_PACOTE, 0);           // envia packet para o socket
            if(bytes<0)                                                         // pega erros, se algum
                printf("error: %s\n", strerror(errno));  
            recv(soquete, resposta, TAM_PACOTE, 0); // remove do queue
            free_packet(resposta);
            break;
        case NACK:
            // funcao q redireciona
            break;
        case ACK:
            // funcao q redireciona
            break;
        case CD:
            cd = (char *) get_packet_data(buffer);
            // ret = chdir((cd+strspn(cd, " ")));
            char *d = malloc(strcspn(cd, " ")*sizeof(char));    // remove espaco no final da mensagem, se tem espaco da ruim 
            char pwd[PATH_MAX];                                 // "cd ..     " -> "..    " nn existe 
            strncpy(d, cd, strcspn(cd, " "));

            if(getcwd(pwd, sizeof(pwd)))printf("before: %s\n", pwd);
            ret = chdir(d);
            if(getcwd(pwd, sizeof(pwd)))printf("after: %s\n", pwd);

            free(d);
            if(ret == -1){
                resultado = ERRO;
                printf("erro foi : %s\n", strerror(errno));
                switch (errno){
                    case 2:
                        printf("here\n");
                        flag[0] = dir_nn_E;
                        break;
                    case 13:
                        flag[0] = sem_permissao;
                        break;
                    default:
                        break;
                };
            }
            else{
                resultado = OK;
            }
            int bytes;
            resposta = make_packet(sequencia_servidor, resultado, flag);
            if(!resposta) // se pacote deu errado
                return;

            sequencia_servidor++;
            // len of packet must be strlen(), sizeof doesnt work
            bytes = send(soquete, resposta, TAM_PACOTE, 0);                 // envia packet para o socket
            if(bytes<0)                                                     // pega erros, se algum
                printf("error: %s\n", strerror(errno));                     // print detalhes do erro

            free(resposta);

            break;
        case LS:
            // funcao q redireciona
            break;
        case MKDIR:
            // funcao q redireciona
            break;
        case GET:
            // funcao q redireciona
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

// trata_tipos: case switch p/ cada ENUM uma funcao
// Tipo == ls -> funcao_ls(dados){ dados = a ou l ... } .... 



