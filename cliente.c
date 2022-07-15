#include <sys/socket.h>
// #include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include "Packet.h"
#include "ConexaoRawSocket.h"

int main()
{
    int sock = ConexaoRawSocket("lo");  // abre o socket -> lo vira ifconfig to pc que manda
    unsigned char *data = "mamamia o buffer tem q ser >= a 14 bytes pra passar :)";
    int bytes;

    unsigned char *packet = build_generic_packet(data);

    // len of packet must be strlen(), sizeof doesnt work
    bytes = send(sock, packet, strlen(packet) + 1, 0);  // envia packet para o socket
    if(bytes<0)                                         // pega erros, se algum
        printf("error: %s\n", strerror(errno));

    close(sock);                                        // fecha socket
    printf("%d bytes enviados no socket %d\n", bytes, sock);
}
