#include "servidor.h"
#include "Packet.h"
#include "ConexaoRawSocket.h"

// global para servidor
int last_packet_sequence = 0;

/* sniff sniff */
int main()
{
    // abre o socket -> lo vira ifconfig to pc que recebe
    int sock = ConexaoRawSocket("lo");
    unsigned char buffer[68];                   // buffer tem no maximo 68 bytes
    int bytes;

    while(1){
        bytes = recv(sock, buffer, sizeof(buffer), 0);      // recebe dados do socket
        buffer[bytes]=(buffer[bytes]==69)?0:buffer[bytes];  // WORKAROUND remove append 'E' do recv/send

        if(bytes>0 && is_our_packet(buffer))
        {   // processa pacote se eh nosso pacote
            read_packet(buffer);
        }
    }

    close(sock);
    printf("servidor terminado\n");
}

// check_paridade_vertical sobre o campo Dados -> NACK ou type_process

void type_process_server(unsigned char* buffer,int buflen)
{
	struct our_packet *pacote = (struct our_packet*)(buffer + sizeof (struct our_packet));
	/* we will se UDP Protocol only*/ 
	switch (pacote->tipo)    //see /etc/protocols file 
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



