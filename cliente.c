#include "ConexaoRawSocket.h"
#include "cliente.h"
#include "Packet.h"

int main()
{
    int sock = ConexaoRawSocket("lo");  // abre o socket -> lo vira ifconfig to pc que manda
    int bytes;

    char * entrada = (char *) calloc(1, sizeof(char));

    while(1){
        fscanf(stdin, "%s", entrada);
        unsigned char* packet = make_packet(0, DADOS, entrada);
        if(!packet) // se pacote deu errado
            return 0;

        // len of packet must be strlen(), sizeof doesnt work
        bytes = send(sock, packet, strlen((char *)packet), 0);          // envia packet para o socket
        if(bytes<0)                                                     // pega erros, se algum
            printf("error: %s\n", strerror(errno));                     // print detalhes do erro

        printf("%d bytes enviados no socket %d\n", bytes, sock);

        read_packet(packet);
        free(packet);
    }

    free(entrada);
    close(sock);                                        // fecha socket
    return 0;
}

void type_process_client(unsigned char* buffer,int buflen)
{
	struct our_packet *pacote = (struct our_packet*)(buffer + sizeof (struct our_packet));
	
	switch (pacote->tipo)    
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