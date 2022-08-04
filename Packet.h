#ifndef PACKET_H
#define PACKET_H

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// VAR GLOBAL //

#define LIMITE_DADOS    63
#define LIMITE_SEQ      15
#define COMMAND_BUFF    100
#define PATH_MAX        100

// TERMINAL BUNITO
#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

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

// variaveis globais definidas no .c, mas declaradas no .h
// respostas de erro no servidor & constantes
extern unsigned char   
                dir_nn_E       , 
                sem_permissao  , 
                dir_ja_E       , 
                arq_nn_E       , 
                sem_espaco     ,
                MARCADOR_INICIO; // 126 -> 0111.1110

struct envelope_packet {
    unsigned char MI;
    unsigned int tamanho : 6;
    unsigned int sequencia : 4;
    unsigned int tipo : 6;
}__attribute__((packed));   // remove padding 

typedef struct envelope_packet envelope_packet;


typedef struct our_packet{

    char MI;    /* 0111.1110 -> ASCII -> '~' = 126 */
    unsigned int tamanho : 6;
    unsigned int sequencia : 4;
    unsigned int tipo : 6;
    /* TODO - data (0-63 bytes) */
    void *data;
    char *complemento;
    char paridade;

} our_packet;

// PROTOTIPOS //

/* ================ packet something ================ */
unsigned char* make_packet(int sequencia, int tipo, char* dados);
int free_packet(unsigned char* packet);

/* ================ packet getters ================ */
char get_packet_MI(unsigned char* buffer);
int get_packet_tamanho(unsigned char* buffer);
int get_packet_sequence(unsigned char* buffer);
int get_packet_type(unsigned char* buffer);
char* get_packet_data(unsigned char* buffer);
int get_packet_parity(unsigned char* buffer);
int get_packet_len(unsigned char* buffer);
char *get_type_packet(unsigned char* buffer);


/* ================ funcoes auxiliares ================ */
void read_packet(unsigned char *buffer);
int is_our_packet(unsigned char *buffer);
int is_valid_type(int tipo);


#endif