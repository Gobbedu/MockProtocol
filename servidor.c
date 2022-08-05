#include "servidor.h"
#include "Packet.h"
#include "ConexaoRawSocket.h"

// global para servidor
int last_packet_sequence = 0;
int soquete;
/* sniff sniff */
int main()
{
    int bytes;
    unsigned char buffer[MAX_PACOTE];                   // buffer tem no maximo 68 bytes
    
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
    unsigned char *resposta;
    int bytes, tipo_lido = get_packet_type(buffer);
    printf("TIPO LIDO %d\n", tipo_lido);

	switch (tipo_lido)
	{
        case OK:
            resposta = make_packet(1, OK, NULL);
            if(!resposta){
                fprintf(stderr, "ERRO NA CRIACAO DO PACOTE\n");
                exit(0);
            }
            printf("CRIOU PACOTE\n");
            read_packet(resposta);
            bytes = send(soquete, resposta, strlen((char *)resposta), 0);           // envia packet para o socket
            if(bytes<0)                                                         // pega erros, se algum
                printf("error: %s\n", strerror(errno));  
            free_packet(resposta);
            break;
        case NACK:
            // funcao q redireciona
            break;
        case ACK:
            // funcao q redireciona
            break;
        case CD:
            // funcao q redireciona
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



