#include <stdio.h>
#include <string.h>
#include "Packet.h"

// estrutura do packet
//  MI 8b | Tamanho 6b | Sequencia 4b | Tipo 6b | Dados 0 - 63 bytes(6b tamanho) | Paridade 8b |||

// por enquanto sequencia eh sempre zero
unsigned char *build_generic_packet(unsigned char *data)
{   // unsigned usa todos os bits do byte (precisa)
    int bytes_data = strlen(data) + 1;
    int sequencia = 0;
    int tipo = DADOS;
    int paridade = 0;

    char packet[4 + bytes_data];

    packet[0] = MARCADOR_INICIO; // 0111.1110 -> Marcador de Inicio

    envelope_packet header;

    
    
    return data;
}


void enquadramento(void){

}


// enquadramento
// sequencializacao
// deteccao de erros
// controle de fluxo

