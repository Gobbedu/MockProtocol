#include "Packet.h"

/* ============================== STREAM FUNCTIONS ============================== */

/*  - [x] juntar os timeouts (envia)
    - [] incrementa ou nao a sequencia na mensagem NACK para a resposta? 
         se mensagem NACK nao chega, timeo no receptor + resend, proxima resposta com sequencia diferente da esperada 
    - [] se cliente nn conseguiu conversar com o servidor, nao incrementar o contador    
*/

#define NTENTATIVAS 3 // numero de vezes que vai tentar ler/enviar um pacote
// SEQUENCIA QUE RECEBEU ACK OU NACK FICA NO CAMPO DE DADOS

/* envia UM pacote com dados especificados e espera UMA resposta,
 * tenta NTENTATIVAS vezes as operacoes envolvidas
 * retorna NULL se perde conexao ou retorna o pacote se recebe resposta esperada
 * atualiza send_seq e recv_seq se deu certo, retorno deve receber free()
 */ 
unsigned char *envia_recebe(int soquete, unsigned int *send_seq, unsigned int *recv_seq, unsigned char *dados, int tipo, int bytes_dados)
{
    int trySend, tryRecv, tryParadis, trySeq;
    unsigned char *resposta;

    // exit if recebe 3 timeouts da funcao
    trySend = tryRecv = tryParadis = trySeq = 0;
    while(  trySend<NTENTATIVAS && tryRecv<NTENTATIVAS && 
            tryParadis<NTENTATIVAS && trySeq<NTENTATIVAS )
    { 
        resposta = NULL;
        if(!envia_msg(soquete, send_seq, tipo, dados, bytes_dados)){
            fprintf(stderr, "TIMEO (%d) envia_msg em envia_recebe\n;", trySend+1);
            trySend++;
            continue;
        }   trySend = 0;        // enviou msg

        resposta = recebe_msg(soquete);
        if(!resposta){
            fprintf(stderr, "TIMEO (%d) recebe_msg em envia_recebe\n", tryRecv+1);
            tryRecv++;
            continue;
        }   tryRecv = 0;        // recebeu msg

        if (!check_parity(resposta)){             
            fprintf(stderr, "ERRO PARIDADE (%d); resposta: (%s) ; mensagem: (%.*s)\n", 
            tryParadis+1, get_type_packet(resposta), MAX_DADOS, resposta+TAM_HEADER);
            tryParadis++;
            continue;
        }   tryParadis = 0;     // paridade ok

        if(!check_sequence(resposta, *recv_seq)){
            fprintf(stderr, "ERRO SEQUENCIA (%d); esperava: (%d) recebeu: (%d)\n", //mensagem: (%.*s)\n", 
            trySeq+1, *recv_seq, get_packet_sequence(resposta)); //,get_type_packet(resposta)); //, MAX_DADOS, resposta+TAM_HEADER);
            trySeq++;
            continue;
        }   trySeq = 0;

        // VERIFICADO & !NACK //
        // aloca resposta para retorno
        unsigned char *pacote = calloc(TAM_PACOTE, sizeof(unsigned char));
        if(!pacote){    
            perror("Erro ao alocar pacote envia_recebe:");
            return NULL;
        }

        memcpy(pacote, resposta, TAM_PACOTE);   // copia respota em pacote alocado
        next(send_seq);                         // aumenta sequencia de enviar
        next(recv_seq);                         // aumenta sequencia esperada ao receber

        // read_packet(resposta);
        return pacote;                          // devolve resposta
    }

    if(trySend==NTENTATIVAS)     fprintf(stderr, "Erro de comunicacao, envia_recebe nao conseguiu enviar  :(\n");
    if(tryRecv==NTENTATIVAS)     fprintf(stderr, "Erro de comunicacao, envia_recebe nao conseguiu receber :(\n");
    if(tryParadis==NTENTATIVAS)  fprintf(stderr, "Erro de comunicacao, envia_recebe recebeu muitos NACKs  :(\n");
    if(trySeq==NTENTATIVAS)      fprintf(stderr, "Erro de comunicacao, envia_recebe recebeu muitas sequencias erradas :(\n");
        
    return NULL;
}

// dado uma base na janela (1a msg) procura quantas msg a frente
// estah a msg com a sequencia desejada
int n_forward(unsigned int base, unsigned int target)
{
    // procura target dentro de janela de 4
    // base 0 | 1 | 2 | 3 | fora da janela ... 
    for(int i = 0; i < 4; i++)
        if(target == moven(&base, i))
            return i;
    
    // target fora da janela
    return -1;
}

void empty_netbuff(int socket)
{
    char buffer[TAM_PACOTE];
    while(recv(socket, buffer, TAM_PACOTE, MSG_PEEK) > 0)
        recv(socket, buffer, TAM_PACOTE, 0);
}

int envia_sequencial(int socket, FILE *file, unsigned int *this_seq, unsigned int *other_seq)
{
    int leu_sz, blocks = 0;
    // char resposta[TAM_PACOTE];
    unsigned char *resposta;
    int leu_bytes = 0;          // numero de bytes lidos do arquivo (deve bater com ls)

    printf("envia sequencial start\n");
    rewind(file);

    // LIMITE CAMPO DADOS //
    int max_data = TAM_PACOTE - sizeof(envelope_packet) - 1;        // total - header - paridade
    unsigned char *data = calloc(max_data, sizeof(char));                      // info q vai no campo dados no pacote
    
    // QUEBRA ARQUIVO EM BLOCOS //
    printf("chunking...\n");
    int tentativas = 0;
    int enviou = true;
    while(tentativas < NTENTATIVAS){  
        if(enviou){
        memset(data, 0, max_data);
        leu_sz = fread((void *)data, sizeof(char), max_data, file);
        printf("leu %d bytes do arquivo\n", leu_sz);
        leu_bytes += leu_sz;
        // VALIDA LEITURA //
        if(leu_sz == 0){ // nao leu nada, erro ou eof
            printf("src leu 0 bytes, break\n");
            break;
        }
        blocks++;
        enviou = false;
        }

        // ENVIA PACOTE NA ESPERA DE UM ACK //
        if(!envia_msg(socket, this_seq, DADOS, data, leu_sz)){
            printf("timeo envia_msg() em envia_sequencial (DADOS)\n");
            tentativas++;
            continue;
        }   

        resposta = recebe_msg(socket);
        if(!resposta){
            printf("timeo recebe_msg() envia_sequencial (ACK, NACK)\n");
            tentativas++;
            continue;
        }   tentativas = 0;
       
        read_packet(resposta);

        if(!check_sequence(resposta, *other_seq)){ // se sequencia errada
            printf("recebe_mgs() em envia sequencial esperava sequencia (%d) e recebeu(%d)\n", *other_seq, get_packet_sequence(resposta));
            return false; // por enquanto nao corrige NACK ou ACK
        }
        if(!check_parity(resposta)){    // se paridade errada
            printf("recebe_msg() recebeu mensagem com erro na paridade\n");
            return false;
        }   
        enviou = true;
        next(this_seq);

        switch (get_packet_type(resposta)){
            case NACK:  // resetar fseek para enviar mesma mensagem dnv
                printf("caiu no NACK\n");
                if(fseek(file, (long) (-leu_sz), SEEK_CUR) != 0){
                    perror("fseek == 0");
                    free(resposta);
                    return false;
                }
                moven(this_seq, -1);
                break;
            
            case ACK:   // continua loop normalmente
                printf("caiu no ACK\n");
                break;

            default:
                printf("tipo nao definido\n");
                free(resposta);
                return false;
        }
        next(other_seq);
        free(resposta);
    }

    if(tentativas == NTENTATIVAS){
        printf("perdeu conexao em envia_sequencial, return\n");
        free(data);
        return false;
    }

    // ajeitar ultimo pacote
    int i;
    for(i = 0; i < NTENTATIVAS; i++)
        if(envia_msg(socket, this_seq, FIM, NULL, 0))
            break;
    if(i == NTENTATIVAS){
        printf("nao foi possivel enviar_msg() FIM, perda de conexao, return\n");
        free(data);
        return false;
    }
        
    next(this_seq);
    free(data);
    blocks++;
    printf("finished chunking (%d) blocks, leu (%d) bytes\n", blocks, leu_bytes);
    return true;
}

int recebe_sequencial(int socket, unsigned char *file, unsigned int *this_seq, unsigned int *other_seq){    
    // charresposta[TAM_PACOTE];
    int wrote, len_data, try;
    // char pacote[TAM_PACOTE];
    unsigned char *pacote;
    unsigned char *seq;

    printf("recebe sequencial start\n");

    // DESTINO //
    char *dest = calloc(6+strlen((char*)file), sizeof(char));
    sprintf(dest, "(copy)%s", file);
    
    FILE *dst = fopen(dest, "w");
    free(dest);
    // FILE *dst = fopen("teste_dst.txt", "w");
    if(!dst){
        printf("could not open destine file, terminate\n");
        return false; 
    }

    // MONTA ARQUIVO //
    try = 0;
    while(try < NTENTATIVAS){    // soh para se recebe pacote com tipo FIM
        pacote = recebe_msg(socket);
        if(!pacote){ 
            try++; 
            continue; 
        }
        read_packet(pacote);

        // FIM //
        if(get_packet_type(pacote) == FIM){
            next(other_seq);
            free(pacote);
            break;
        }

        // NACK //
        if (!check_sequence(pacote, *other_seq)){
                // paridade diferente, dado: (sequencia esperada)
                printf("recebe recebeu (%d) mas esperava (%d) como sequencia\n", *other_seq, get_packet_sequence(pacote));
                seq = itoa(*other_seq);
                envia_msg(socket, this_seq, NACK, seq, 2); free(seq);
                // empty_netbuff(socket);
                
                // free(pacote);
                continue;   // volta a ouvir
        }
        if(!check_parity(pacote)){
                printf("recebe recebeu pacote com erro de paridade, NACK\n");
                // sequencia diferente, dado:(sequencia atual)
                seq = itoa(*other_seq);
                envia_msg(socket, this_seq, NACK, seq, 2); free(seq);
                // empty_netbuff(socket);
                
                // free(pacote);
                continue;   // volta a ouvir
        }
        
        // ACK // (soh e possivel retornar NACK & ACK transmitindo dados)
        next(other_seq);

        // data comeca 3 bytes depois do inicio
        len_data  = get_packet_tamanho(pacote);         // tamanho em bytes a escrever no arquivo
        wrote = fwrite((void*) (pacote+3), sizeof(char), len_data, dst);
        if(wrote == 0){
            perror("return, build wrote 0 bytes, ");
            return false;
        }
        seq = ptoa(pacote);
        envia_msg(socket, this_seq, ACK, seq , 2); free(seq);
        printf("\t\t\t\tenviou ACK\n");
        // next(this_seq);
        // free(pacote);
    }

    fclose(dst);
    if(try == 1){
        printf("algo deu errado com recebe sequencial..\n");
        return false;
    }
    return true;
}

// envia uma mensagem para o socket, com verificacao de send() bytes > 0
// retorna true se enviou com sucesso, e falso c.c., NAO ATUALIZA SEQUENCIA
// tenta enviar mensagem NTENTATIVAS vezes
int envia_msg(int socket, unsigned int *this_seq, int tipo, unsigned char *parametro, int n_bytes)
{
    int bytes, tentativas;
    unsigned char *packet = make_packet(*this_seq, tipo, parametro, n_bytes);
    if(!packet){
        fprintf(stderr, "ERRO NA CRIACAO DO PACOTE\n");
        return false;
    }

    read_packet(packet);
    unsigned int *mascara = calloc(TAM_PACOTE, sizeof(unsigned int));
    for(int i = 0; i < TAM_PACOTE; i++){
        mascara[i] = (unsigned int) packet[i];
        printf("%d ", mascara[i]);
    }

    // tentativas = 0;
    // while(tentativas < NTENTATIVAS){
    for(tentativas = 0; tentativas < NTENTATIVAS; tentativas++){
        bytes = send(socket, mascara, sizeof(unsigned int)*TAM_PACOTE, 0);       // envia packet para o socket
        if(bytes > 0)
            break;
        else 
            perror("oq deu:");
    }

    if(tentativas == NTENTATIVAS){
        free(packet);
        free(mascara);
        printf("\nnn deu \n");
        return false;
    }

    // printf("sent (%d) bytes\n", bytes);
    // read_packet(packet);
    free(packet);
    free(mascara);
    return true;
}

// recebe uma mensagem do socket, tenta receber mensagem NTENTATIVAS vezes, deve receber free()
// verifica se recv deu timeout, bytes > 0 e se eh nosso pacote, NAO ATUALIZA SEQUENCIA
// retorna NULL se nao foi possivel receber a msg, e a mensagem c.c.
unsigned char *recebe_msg(int socket)
{
    // unsigned char buffer[TAM_PACOTE];
    unsigned int buffer[TAM_PACOTE];
    int bytes, i;

    // VERIFICA //
    memset(buffer, 0, sizeof(unsigned int)*TAM_PACOTE);
    for(i = 0; i < NTENTATIVAS; i++){
        bytes = recv(socket, buffer, sizeof(unsigned int)*TAM_PACOTE, 0);        // recebe dados do socket
        if (bytes >= TAM_PACOTE)                           // recebeu tamanho do pacote
            // if (is_our_packet(buffer))                      // e eh nosso pacote
                break;
    }

    // nao recebeu pacote valido
    if(i == NTENTATIVAS) 
        return NULL;

    printf("leu (%d) bytes\n", bytes);

    unsigned char *pacote = calloc(TAM_PACOTE, sizeof(unsigned char));
    for(int i = 0; i < TAM_PACOTE; i++)
        pacote[i] = (unsigned char) buffer[i];

    // recebeu pacote valido //
    // memcpy(pacote, buffer, TAM_PACOTE);
    return pacote;
}

// atualiza sequencia dada para a proxima sequencia
unsigned int next(unsigned int *sequence){
    *sequence = ((*sequence)+1)%MAX_SEQUENCE;
    return *sequence;
}

// retorna sequencia n sequencias depois da especificada, n pertence a [-15, +inf)
unsigned int moven(unsigned int *sequence, int n)
{
    *sequence = ((*sequence)+MAX_SEQUENCE+n)%MAX_SEQUENCE;
    return *sequence;
}

// transorma sequencia do pacote em string, deve receber free()
unsigned char *ptoa(unsigned char *pacote){
    unsigned char *text = calloc(2, sizeof(char));
    sprintf((char*)text, "%d", get_packet_sequence(pacote));
    return text;
}

// transorma inteiro em string, deve receber free()
unsigned char *itoa(int sequencia){
    unsigned char *text = calloc(2, sizeof(char));
    sprintf((char*)text, "%d", sequencia);
    return text;
}
