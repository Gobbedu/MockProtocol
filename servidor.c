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
            sequencia_cliente = get_packet_sequence(buffer);
        }
    }

    close(soquete);
    printf("servidor terminado\n");
}

// check_paridade_vertical sobre o campo Dados -> NACK ou type_process

void server_switch(unsigned char* buffer)
{
    int resultado;
    unsigned char flag = '\0';
    unsigned char *resposta;
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
            bytes = send(soquete, resposta, strlen((char *)resposta), 0);           // envia packet para o socket
            if(bytes<0)                                                         // pega erros, se algum
                printf("error: %s\n", strerror(errno));  
            recv(soquete, resposta, strlen((char *)resposta), 0); // remove do queue
            free_packet(resposta);
            break;
        case NACK:
            // funcao q redireciona
            break;
        case ACK:
            // funcao q redireciona
            break;
        case CD:
            unsigned char* packet = make_packet(sequencia_cliente, CD, NULL);
            char *cd = get_packet_data(packet);
            ret = chdir((cd+strspn(cd, " ")));
            if(ret == -1){
                resultado = ERRO;
                switch (errno){
                    case 2:
                        flag = dir_nn_E;
                        break;
                    case 13:
                        flag = sem_permissao;
                        break;
                    default:
                        break;
                };
            }
            else{
                resultado = OK;
            }
            int bytes;
            resposta = make_packet(sequencia_servidor, resultado, (char *)flag);
            if(!resposta) // se pacote deu errado
                return;

            sequencia_servidor++;
            // len of packet must be strlen(), sizeof doesnt work
            bytes = send(soquete, resposta, strlen((char *)resposta), 0);          // envia packet para o socket
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



