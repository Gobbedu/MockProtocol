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

/* ============================== STREAM FUNCTIONS ============================== */


// retorna array de pacotes (char *), todos tem tamanho cte (67 bytes)
unsigned char** chunck_file(unsigned int start_seq, char *file, int *arr_size)
{
    unsigned char **array, **bigger_array;
    int eof, leu_sz, max_data;
    char *data;

    int leu_bytes = 0;          // numero de bytes lidos do arquivo (deve bater com ls)
    int allocd_sz = 1;          // memoria alocada no vetor de retorno
    *arr_size = 0;              // blocos utilizados no vetor
    
    // ABRE ARQUIVO //
    FILE *src = fopen(file, "r");                           // arquivo a enviar
    if(src == NULL){
        printf("something went wrong opening file (%s)\n return\n", file);
        return NULL;
    }

    // LIMITE CAMPO DADOS //
    max_data = TAM_PACOTE - sizeof(envelope_packet) - 1;        // total - header - paridade
    data = calloc(max_data, sizeof(char));                      // info q vai no campo dados no pacote
    array = calloc(allocd_sz, sizeof(unsigned char*));          // inicia tamanho do vetor de pacotes em 1
    
    // QUEBRA ARQUIVO EM BLOCOS //
    eof = false;
    printf("chunking\n");
    while(!eof){  
        leu_sz = fread((void *)data, sizeof(char), max_data, src);
        leu_bytes += leu_sz;

        // VALIDA LEITURA //
        if(feof(src)){  // termina se fim do arquivo
            printf("src feof break\n");
            eof = true;
        }
        if(leu_sz == 0){ // nao leu nada, erro ou eof
            printf("src leu 0 bytes, break\n");
            break;
        }

        // SALVA PACOTE NO VETOR //
        array[(*arr_size)++] = make_packet(start_seq, DADOS, data, leu_sz);     // incrementa arr_size
        start_seq = (start_seq+1)%MAX_SEQUENCE;                                 // incrementa sequencia

        // AUMENTA VETOR DE PACOTE (se precisa)// 
        if((*arr_size) >= allocd_sz){
            allocd_sz = allocd_sz*2;      // aloca 2, 4, 8, 16, ...
            bigger_array = reallocarray(array, allocd_sz, sizeof(unsigned char**));
            if(bigger_array == 0){
                printf("error while increasing array of packets, exit\n");
                exit(-1);
            }
            else{ // aumentou tamanho o array
                array = bigger_array;
            }
        }
    }
    printf("finished chunking (%d) blocks, leu (%d) bytes\n", *arr_size, leu_bytes);

    // ajeitar ultimo pacote
    array[(*arr_size)++] = make_packet(start_seq, FIM, NULL, 0);

    // REMOVE ESPACO EXTRA //
    unsigned char **fit_array;
    fit_array = reallocarray((void*)array, (*arr_size), sizeof(unsigned char**));

    if(fit_array == NULL){
        printf("error while fitting array of packets, exit\n");
        exit(-1);
    }
    else{
        array = fit_array;
    }
    
    free(data);
    fclose(src);
    return array;
} 

// cria arquivo (file) com base no conteudo lido de (packet_array) com (array_size) blocos
int build_file(char *file, unsigned char **packet_array, int array_size)
{
    int wrote, len_data;
    // unsigned char **packet_array;

    // DESTINO //
    FILE *dst = fopen("Rick_copy.mp4", "w");
    // FILE *dst = fopen("teste_dst.txt", "w");
    if(!dst){
        printf("could not open destine file, terminate\n");
        return false;
    }

    // MONTA ARQUIVO //
    for(int i = 0; i < array_size; i++){
        // ignorar (nome da parada q completa campo dados se dados pequeno), so escrever dados
        len_data  = get_packet_tamanho(packet_array[i]);

        // data comeca 3 bytes depois do inicio
        wrote = fwrite((void*) (packet_array[i]+3), sizeof(char), len_data, dst);
        if(wrote == 0){
            printf("dst wrote 0 bytes, break\n");
            break;
        }
        free(packet_array[i]);
    }
    free(packet_array);
    return true;
}

/*
// MODELO DE USO FUNCIONA //
int main(){
    int len_pacotes;
    unsigned char **pacotes;
    
    pacotes = chunck_file(0, "Rick_Roll.mp4", &len_pacotes);    // quebra file em vetor de pacotes
    if(!pacotes){
        printf("could not chunk file corectly, terminate\n");
        return -1;
    }

    // RECONSTROI // 
    if(build_file("Rick_copy.mp4", pacotes, len_pacotes))       // com vetor de pacotes, cria arquivo
        printf("pacote reconstruido com sucesso\n");
    else 
        printf("erro ao reconstruir pacote\n");

    return 0;
}
*/

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
    unsigned char *packet   =   malloc(TAM_PACOTE);                         // aloca mem pro pacote, retorna string
    memset(packet, 0, TAM_PACOTE);                                          // limpa lixo na memoria alocada

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

// printa o header + data + paridade do pacote
void read_packet(unsigned char *buffer)
{   // destrincha o pacote para formato legivel

    printf("packet MI       : %c\n", get_packet_MI(buffer));
    printf("packet Tamanho  : %d\n", get_packet_tamanho(buffer));
    printf("packet Sequencia: %d\n", get_packet_sequence(buffer));
    printf("packet Tipo     : %s\n", get_type_packet(buffer));              // string com tipo
    printf("packet Dados    : %.*s\n",get_packet_tamanho(buffer), get_packet_data(buffer));   // print n bytes da string em data
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
    int paradis = 0;
    int header = sizeof(envelope_packet);
    int len = get_packet_tamanho(buffer) + header;

    for(int i = header; i < len; i++)   // desde o inicio ate o final de dados (exclui fill)
        paradis ^= buffer[i];

    return paradis == get_packet_parity(buffer);
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
