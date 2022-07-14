#include <sys/socket.h>
// #include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include "Packet.h"
#include "ConexaoRawSocket.h"

int main()
{
    char *buffer = "mamamia o buffer tem q ser >= a 14 bytes pra passar :)";
    int sock = ConexaoRawSocket("lo");                  // abre o socket
    int bytes;

    bytes = send(sock, buffer, strlen(buffer) + 1, 0);  // envia buffer para o socket
    if(bytes<0)                                         // pega erros, se algum
        printf("error: %s\n", strerror(errno));

    close(sock);                                        // fecha socket
    printf("%d bytes enviados no socket %d\n", bytes, sock);
}
