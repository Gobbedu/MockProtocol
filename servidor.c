#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include "Packet.h"
#include "ConexaoRawSocket.h"

/* sniff sniff */
int main()
{
    int sock = ConexaoRawSocket("lo");
    char buffer[512];                   // pega 512 com sizeof(), pega 1 com strlen()
    int bytes;

    while(1){
        bytes = recv(sock, buffer, sizeof(buffer), 0);      // recebe dados do socket
        buffer[sizeof(buffer)] = '\0';                      // fim da string no buffer
        printf("recebi %d bytes: '%s'\n",bytes, buffer);    // print dados recebidos
    }

    close(sock);
    printf("servidor terminado\n");
}

// recvfrom .... pacote
// .
// .    (acho MI) (le tamanho)
// .    
// .
// quebra pacote -> MI | tamanho | Tipo | Dados | Paridade 
// check_paridade_vertical sobre o campo Dados -> NACK ou type_process

void type_process(unsigned char* buffer,int buflen)
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



