#ifndef PACKET_H
#define PACKET_H

#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// VAR GLOBAL //

#define TAM_PACOTE      67
#define MAX_DADOS       63
#define MAX_SEQUENCE    16  // para usar em modulo, limite eh na verdade 15
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

extern unsigned int now_sequence, last_sequence;

struct envelope_packet {
    unsigned char MI;
    unsigned int tamanho : 6;
    unsigned int sequencia : 4;
    unsigned int tipo : 6;
}__attribute__((packed));   // remove padding 

typedef struct envelope_packet envelope_packet;


// PROTOTIPOS //

/* ================ stream functions ================ */
unsigned char** chunck_file(unsigned int start_seq, char *file, int *arr_size);
int build_file(char *file, unsigned char **packet_array, int array_size);
void janela_recebe4(int socket, FILE *file, unsigned int this_seq, unsigned int other_seq);
void janela_envia4 (int socket, FILE *file, unsigned int this_seq, unsigned int other_seq);



/* ================ packet functions ================ */
unsigned char* make_packet(unsigned int sequencia, int tipo, char* dados, int bytes_dados);

int check_sequence(unsigned char *buffer, int expected_seq);
int check_parity(unsigned char *buffer);

int calc_packet_parity(unsigned char *buffer);
int free_packet(unsigned char* packet);


// sequencializacao EH LOCAL
// unsigned int sequencia(void);
// unsigned int get_seq(void);
// unsigned int get_lastseq(void);
// unsigned int next_seq(void);

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