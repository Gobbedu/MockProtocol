#include <stdio.h>
#include "Packet.h"
#include "ConexaoRawSocket.h"

int main()
{
    // char device[3] = {'l', 'o', '\0'};
    // char *device = "lo\0";
    char *device = "lo";
    int sock = ConexaoRawSocket("lo");
    printf("sock out: %d\n", sock);

    for(int i = 0; i < 10; i++)
    {

    }
}
