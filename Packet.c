#include "Packet.h"

/* estrutura do packet
 * MI 8b | Tamanho 6b | Sequencia 4b | Tipo 6b | Dados 0 - 63 bytes(6b tamanho) | Paridade 8b |||
 */

// variaveis globais declaradas no .h, definidas no .c
// respostas de erro no servidor
unsigned char   dir_nn_E        = 'A', 
                sem_permissao   = 'B', 
                dir_ja_E        = 'C', 
                arq_nn_E        = 'D', 
                sem_espaco      = 'E',
                MARCADOR_INICIO = '~'; // 126 -> 0111.1110


// por enquanto sequencia eh sempre zero
unsigned char *build_generic_packet(unsigned char *data)
{   // unsigned usa todos os bits do byte(precisa), sem sinal no byte 
    int len_header = sizeof(envelope_packet);
    unsigned int bytes_data = strlen((char *)data) ;        // nao precisa contar ate \0
    int len_packet = (len_header + bytes_data + 1);         // 3 + dados + 1 -> 3 pro header e 1 pra paridade 
    unsigned char *packet = malloc(len_packet);             // aloca mem pro pacote
    memset(packet, 0, len_packet);                          // limpa lixo na memoria alocada

    envelope_packet header_t;
    header_t.MI = MARCADOR_INICIO;      // 0111.1110 -> Marcador de Inicio
    header_t.tamanho = bytes_data;
    header_t.sequencia = 0;
    header_t.tipo = DADOS;

    // cria ponteiro de header e faz cast para ponteiro pra char
    envelope_packet *header_p = &header_t;
    unsigned char *header = (unsigned char*) header_p;

    printf("pointed MI       : %d\n", header_p->MI);
    printf("pointed tamanho  : %d\n", header_p->tamanho);
    printf("pointed sequencia: %d\n", header_p->sequencia);
    printf("pointed tipo     : %d\n", header_p->tipo);

    packet[0] = header[0];      // salva MI em pacote[0] (8bits | 1byte)
    packet[1] = header[1];      // Tamanho + sequencia + tipo   = 2 bytes
    packet[2] = header[2];      // 6 bits  +  4 bits   + 6 bits = 2 bytes

    // copia dados na sessao dados, excluindo o \0 de 'data'
    packet[len_header+bytes_data] = data[0];
    packet[len_header] = data[0];
    for(int i = 1; i < bytes_data; i++)
    {   // salva 'data' no pacote e calcula paridade vertical
        packet[len_header+i] = data[i];
        packet[len_header+bytes_data] ^= data[i];   // paridade vertical para deteccao de erros (XOR)
    }

    printf("pointed data: %s\n", (packet + len_header));
    printf("pointed paridade: %d\n", packet[len_header+bytes_data]);

    packet[len_header+bytes_data+1] = '\0';         // indica final do pacote para strlen()
    return packet;
}



void enquadramento(void){

}


// enquadramento
// sequencializacao
// deteccao de erros
// controle de fluxo

