/* sla */

#ifndef __PACKET__
#define __PACKET__

#include <string.h>
#include <unistd.h>

enum TIPOS {
    OK = 1,
    NACK = 2,
    ACK = 3,
    CD = 6,
    LS = 7,
    MKDIR = 8,
    GET = 9,
    PUT = 10,
    ERRO = 17,
    DESC_ARQ = 24,
    DADOS = 32,
    FIM = 46,
    SHOW_NA_TELA = 63,
    };

// respostas de erro no servidor
unsigned char   dir_nn_E        = 'A', 
                sem_permissao   = 'B', 
                dir_ja_E        = 'C', 
                arq_nn_E        = 'D', 
                sem_espaco      = 'E';

unsigned char MARCADOR_INICIO = 126;


typedef struct {
    unsigned int tamanho : 6;
    unsigned int sequencia : 4;
    unsigned int tipo : 6;
} envelope_packet;


typedef struct our_packet{

    char MI;    /* 0111.1110 -> ASCII -> '~' = 126 */
    unsigned int tamanho : 6;
    unsigned int sequencia : 4;
    unsigned int tipo : 6;
    /* TODO - data (0-63 bytes) */
    void *data;
    char paridade;

} our_packet;


void enquadramento(void);
unsigned char *build_generic_packet(unsigned char *data); // unsigned usa todos os bits do byte (precisa)

#endif