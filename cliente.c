#include "ConexaoRawSocket.h"
#include "cliente.h"
#include "Packet.h"

int main()
{
    int sock = ConexaoRawSocket("lo");  // abre o socket -> lo vira ifconfig to pc que manda
    int bytes;

    // char *data = "0123456789";
    // unsigned char *packet = build_generic_packet((unsigned char*)data);

    unsigned char* packet = make_packet(0, DADOS, "0123456789");
    if(!packet) // se pacote deu errado
        return 0;

    // len of packet must be strlen(), sizeof doesnt work
    bytes = send(sock, packet, strlen((char *)packet), 0);          // envia packet para o socket
    if(bytes<0)                                                     // pega erros, se algum
        printf("error: %s\n", strerror(errno));                     // print detalhes do erro

    close(sock);                                        // fecha socket
    printf("%d bytes enviados no socket %d\n", bytes, sock);

    read_packet(packet);
    free(packet);

    return 0;
}
