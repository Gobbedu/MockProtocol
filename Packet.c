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
                MARCADOR_INICIO = '~';  // 126 -> 0111.1110


/* ===== TODO ===== */
// sequencializacao
// controle de fluxo
// deteccao de erros (Junto a recv e send das mensagens XOR byte a byte)

/* ============================== PACKET FUNCTIONS ============================== */

/* cria e seta pacote inteiro do zero, retorna nullo se houve ERRO
 * compoe enquadramento & calculo de paridade
 */
unsigned char* make_packet(unsigned int sequencia, int tipo, char* dados, int bytes_dados)
{
    // VALIDA //
    if(dados)
    {   // se existe dados
        if(bytes_dados > MAX_DADOS){
            printf("ERRO: tamanho da mensagem excede limite de dados do pacote: %d\n", bytes_dados);
            return NULL;
        }
    }

    if(sequencia > MAX_SEQUENCE){
        printf("ERRO: tamanho da sequencia excede limite do pacote: %d\n", sequencia);
        return NULL;
    }

    if(!is_valid_type(tipo)){     // tipo invalido
        printf("ERRO: tipo nao pertence aos tipos do pacote: %d\n", tipo);
        return NULL;
    }

    // CRIA PACOTE //
    // aloca memoria para o pacote
    int len_header          =   sizeof(envelope_packet);                    // tamanho do header (MI, tamanho, sequencia, tipo)
    unsigned char *packet   =   calloc(TAM_PACOTE, 1);           // aloca mem pro pacote, retorna string
    // memset(packet, 0, TAM_PACOTE);                                          // limpa lixo na memoria alocada

    // define informacao do header
    envelope_packet header_t;
    header_t.MI         = MARCADOR_INICIO;      // 0111.1110 -> Marcador de Inicio
    header_t.tamanho    = bytes_dados;          // evita strlen para enviar bytes de arquivos
    header_t.sequencia  = sequencia;
    header_t.tipo       = tipo;

    // cria ponteiro de header e faz cast para ponteiro pra char
    envelope_packet *header_p = &header_t;
    unsigned char *header = (unsigned char*) header_p;

    packet[0] = header[0];      // salva MI em pacote[0] (8bits | 1byte)
    packet[1] = header[1];      // Tamanho + sequencia + tipo   = 2 bytes
    packet[2] = header[2];      // 6 bits  +  4 bits   + 6 bits = 2 bytes

    for(int i = 0; i < bytes_dados; i++){   // calcula paridade somente dos dados
        packet[len_header+i] = dados[i];    // salva 'data' no pacote 
        packet[TAM_PACOTE-1] ^= dados[i];   // paridade vertical para deteccao de erros (XOR)
    }

    // complemento nao entra na paridade
    char fill = ' ';
    for(int i = len_header+bytes_dados; i < TAM_PACOTE-1; i++)  // a partir de onde dados parou
        packet[i] = fill;                                       // ate penultimo byte do packet

    return packet;
}

int free_packet(unsigned char* packet)
{   
    if(!packet) // if packet nn existe (NULL)
        return 0;

    free(packet);
    return 1;
}

/* =========================== FUNCOES AUXILIARES =========================== */

void data_asint(unsigned char *buffer){
    int len = get_packet_tamanho(buffer);
    for(int i = 0; i < len; i++)
        printf("%d|", buffer[3+i]);
    printf("fim\n");
}
// printa o header + data + paridade do pacote
void read_packet(unsigned char *buffer)
{   // destrincha o pacote para formato legivel

    printf("packet MI       : %c\n", get_packet_MI(buffer));
    printf("packet Tamanho  : %d\n", get_packet_tamanho(buffer));
    printf("packet Sequencia: %d\n", get_packet_sequence(buffer));
    printf("packet Tipo     : %s\n", get_type_packet(buffer));              // string com tipo
    // printf("packet Dados    : %.*s\n",get_packet_tamanho(buffer), get_packet_data(buffer));   // print n bytes da string em data
    printf("packet Dados: ");
    data_asint(buffer);
    printf("packet Paridade : %d\n", get_packet_parity(buffer));            // paridade int 8bits   (pacote[-1])
    printf("Total-----------: %d Bytes\n", get_packet_len(buffer));         // tamanho total do pacote

    return;
}

int is_our_packet(unsigned char *buffer)
{   // retorna 1 se sim, 0 caso contrario
    envelope_packet *header = (envelope_packet*) buffer;
    return header->MI == MARCADOR_INICIO;
}

int is_valid_type(int tipo){
    if( tipo > 63)  // tipo maior que 6 bits
        return 0;
    switch (tipo)  
	{
        case OK:
        case NACK:
        case ACK:
        case CD:
        case LS:
        case MKDIR:
        case GET:
        case PUT:
        case ERRO:
        case DESC_ARQ:
        case DADOS:
        case FIM:
        case SHOW_NA_TELA:
            return 1;
            break;
        default:
            return 0;
            break;
    };
    return 0; // alguma coisa deu errada
}

int calc_packet_parity(unsigned char *buffer)
{
    int header = sizeof(envelope_packet);
    int len = get_packet_tamanho(buffer);
    int paradis = 0;

    for(int i = header; i < len+header; i++)
        paradis ^= buffer[i];

    return paradis;    
}

int check_parity(unsigned char* buffer){
    return calc_packet_parity(buffer) == get_packet_parity(buffer);
}

// se paridade nao bate, retorna falso, c.c. verdadeiro
int check_sequence(unsigned char *buffer, int expected_seq)
{
    if(expected_seq == get_packet_sequence(buffer))
        return true;

    return false;  
}
/* ============================== PACKET GETTERS ============================== */
// retorna marcador de inicio do pacote
char get_packet_MI(unsigned char* buffer){
    envelope_packet *header = (envelope_packet*) buffer;
    return header->MI;
}

// retorna sessao tamanho do pacote (tamanho da sessao Dados em bytes)
int get_packet_tamanho(unsigned char* buffer){
    envelope_packet *header = (envelope_packet*) buffer;
    return header->tamanho;
}

// retorna sessao sequencia do pacote
int get_packet_sequence(unsigned char* buffer){
    envelope_packet *header = (envelope_packet*) buffer;
    return header->sequencia;
}

// retorna sessao tipo do pacote como inteiro
int get_packet_type(unsigned char* buffer){  
    envelope_packet *header = (envelope_packet*) buffer;
    return header->tipo;
}

// retorna sessao dados do pacote como string (deve receber free)
char* get_packet_data(unsigned char* buffer){               // empurra ponteiro
    int size = get_packet_tamanho(buffer);
    char *data = malloc(size*sizeof(char));
    strncpy(data, (char *)(buffer+sizeof(envelope_packet)), size);
    data[size] = '\0';
    return data;
}

// retorna sessao de paridade do pacote
int get_packet_parity(unsigned char* buffer){   // comeca a contar do zero, nn precisa de + 1
    return buffer[TAM_PACOTE-1];                // ultima posicao do buffer len(header) + len(dados)
}

// retorna tamanho de todo o pacote em bytes
int get_packet_len(unsigned char* buffer){  // conta quantos bytes tem antes do \0 (excluindo o \0)
    // return strlen((char*)buffer);        // final de string em c sempre acaba em \0
    return TAM_PACOTE;
};

// retorna sessao tipo do pacote como string (char *)
char *get_type_packet(unsigned char* buffer){
    envelope_packet *header = (envelope_packet*) buffer;
    
	switch (header->tipo)    
	{
        case OK:
            return "OK";
            break;
        case NACK:
            return "NACK";
            break;
        case ACK:
            return "ACK";
            break;
        case CD:
            return "CD";
            break;
        case LS:
            return "LS";
            break;
        case MKDIR:
            return "MKDIR";
            break;
        case GET:
            return "GET";
            break;
        case PUT:
            return "PUT";
            break;
        case ERRO:
            return "ERRO";
            break;
        case DESC_ARQ:
            return "DESC_ARQ";
            break;
        case DADOS:
            return "DADOS";
            break;
        case FIM:
            return "FIM";
            break;
        case SHOW_NA_TELA:
            return "MOSTRA NA TELA";
            break;

		default:
            return "nao especificado";
			break;
	}
}
