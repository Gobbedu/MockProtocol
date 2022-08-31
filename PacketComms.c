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
unsigned char*envia(int soquete,unsigned char * packet, unsigned int *expected_seq)
{
    int bytes, timeout, lost_conn, resend;
    // char resposta[TAM_PACOTE];
    unsigned char *resposta;
    unsigned char* data;

    // exit if recebe 3 timeouts da funcao
    timeout = lost_conn = 0;
    while(lost_conn<3){ 
        resend = 0;

        // printf("sending packet:\n");
        // read_packet(packet);
        // printf("\n");
        int len_byte = sizeof(unsigned short int);
        unsigned short int mask[TAM_PACOTE];
        memset(mask, 0, len_byte*TAM_PACOTE);
        for(int i = 0; i < TAM_PACOTE; i++)
            mask[i] = (unsigned short int) packet[i];

        bytes = send(soquete, mask, len_byte*TAM_PACOTE, 0);       // envia packet para o socket
        if(bytes < 0){                                      // pega erros, se algum
            perror("ERROR envia pacote em envia()");
        }

        // said do loop soh se server responde nack, (ok e erro retorna da funcao)
        timeout = 0;
        while(!resend)
        {   
            if(timeout == 3){   // recv deu timeout 3 vezes, timeout da funcao
                lost_conn++;
                break;
            }

            resposta = recebe_msg(soquete);
            if(!resposta){
                lost_conn++;
                break;
            }

            if (!check_sequence(resposta, *expected_seq)){              // sequencia incorreta
                printf("client expected %d but got %d as a sequence\n", *expected_seq, get_packet_sequence(resposta));
                continue;
            }

            // SE NACK, REENVIA //
            // precisa checar paridade, servidor responde dnv (???????????) nao sei
            // Paridade diferente: NACK 
            // mensagem recebida mas nao compreendida, reenviar
            if (!check_parity (resposta)){             
                data = get_packet_data(resposta);
                printf("resposta: (%s) ; mensagem: (%s)\n", get_type_packet(resposta), data);

                lost_conn = timeout = 0;
                resend = true;
                free(data);
                break;  // exit response loop & re-send 
            }

            // SE VERIFICADO & !NACK //
            // devolve resposta //
            // printf("pacote enviado recebeu resposta!\n");
            unsigned char*pacote = calloc(TAM_PACOTE, sizeof(char ));
            // read_packet(resposta);
            memcpy(pacote, resposta, TAM_PACOTE);
            *expected_seq = ((*expected_seq)+1)%MAX_SEQUENCE;
            return pacote;
        }
    }

    printf("Erro de comunicacao, servidor nao responde :(\n");
    return NULL;
}

// recebe UM pacote, bloqueante (?) 
// retorna NULL se nn recebeu nada ou aloca um pacote se recebe pacote esperado
// pacote recebido deve ser liberado da memoria, free(pacote recebido)
char*recebe(int soquete, unsigned int *this_seq, unsigned int *expected_seq)
{
    int bytes;
    // char *seq;
    // unsigned char *resposta;
    unsigned char buffer[TAM_PACOTE];
    bytes = recv(soquete, buffer, TAM_PACOTE, 0);       // recebe dados do socket

    // VERIFICA //
    if (bytes<=0) return NULL;                 // se erro ou vazio, ignora
    if (!is_our_packet(buffer)) return NULL;   // se nao eh nosso pacote, ignora

    // VERIFICA & !NACK, devolve pacote
    char*pacote = calloc(TAM_PACOTE, sizeof(char ));
    memcpy(pacote, buffer, TAM_PACOTE);
    
    *(expected_seq) = ((*expected_seq)+1)%MAX_SEQUENCE;
    return pacote;
}

/* envia UM pacote com dados especificados e espera UMA resposta,
 * tenta NTENTATIVAS vezes as operacoes envolvidas
 * retorna NULL se perde conexao ou retorna o pacote se recebe resposta esperada
 * atualiza send_seq e recv_seq se deu certo, retorno deve receber free()
 */ 
unsigned char *envia_recebe(int soquete, unsigned int *send_seq, unsigned int *recv_seq, unsigned char *dados, int tipo, int bytes_dados)
{
    int lost_conn;
    unsigned char *resposta;

    // exit if recebe 3 timeouts da funcao
    lost_conn = 0;
    while(lost_conn<NTENTATIVAS){ 

        if(!envia_msg(soquete, send_seq, tipo, dados, bytes_dados)){
            lost_conn++;
            continue;
        }

        // said do loop soh se server responde nack, (ok e erro retorna da funcao)
        while(1)
        {   

            resposta = recebe_msg(soquete);
            if(!resposta){
                lost_conn++;
                break;
            }

            if (!check_sequence(resposta, *recv_seq)){              // sequencia incorreta
                printf("envia_recebe esperava recv_seq com %d mas recebeu %d\n", *recv_seq, get_packet_sequence(resposta));
                free(resposta);
                break;
            }

            // SE NACK, REENVIA //
            // precisa checar paridade, servidor responde dnv (???????????) nao sei
            // Paridade diferente: NACK 
            // mensagem recebida mas nao compreendida, reenviar
            if (!check_parity (resposta)){             
                printf("resposta: (%s) ; mensagem: (%.*s)\n", get_type_packet(resposta), get_packet_tamanho(resposta), resposta+TAM_HEADER);
                free(resposta);
                break;  // exit response loop & re-send 
            }
            lost_conn = 0;

            // SE VERIFICADO & !NACK //
            // devolve resposta //
            // printf("pacote enviado recebeu resposta!\n");
            unsigned char*pacote = calloc(TAM_PACOTE, sizeof(char ));
            // read_packet(resposta);
            memcpy(pacote, resposta, TAM_PACOTE);
            next(recv_seq);
            return pacote;
        }
    }

    printf("Erro de comunicacao, servidor nao responde :(\n");
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
            moven(this_seq, -1);
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
        // next(this_seq);

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
    if(!envia_msg(socket, this_seq, FIM, NULL, 0)){
        printf("nao foi possivel enviar_msg() FIM\n");
        return false;
    }
        
    // next(this_seq);
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
        // tenta receber pacote 
        pacote = recebe_msg(socket);
        if(!pacote){
            try++;
            continue;
        }   try = 0;
        read_packet(pacote);

        // FIM //
        if(get_packet_type(pacote) == FIM){
            next(other_seq);
            // free(pacote);
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
        printf("enviou ACK\n");

        // free(pacote);
    }

    fclose(dst);
    if(try == NTENTATIVAS){
        printf("algo deu errado com recebe sequencial..\n");
        return false;
    }
    return true;
}

// envia uma mensagem para o socket, com verificacao de send() bytes > 0
// retorna true se enviou com sucesso, e falso c.c., ATUALIZA SEQUENCIA
// tenta enviar mensagem NTENTATIVAS vezes
int envia_msg(int socket, unsigned int *this_seq, int tipo, unsigned char *parametro, int n_bytes)
{
    int bytes, len_byte, i;
    // CRIA PACOTE
    unsigned char *packet = make_packet(*this_seq, tipo, parametro, n_bytes);
    if(!packet){
        fprintf(stderr, "ERRO NA CRIACAO DO PACOTE\n");
        return false;
    }

    // COLOCA MASCARA
    len_byte = sizeof(unsigned long);
    unsigned long mask[TAM_PACOTE];
    memset(mask, -1, len_byte*TAM_PACOTE); // mascarar com 0, 1, 170, 85, 255, nao funciona
    for(int i = 0; i < TAM_PACOTE; i++)
        mask[i] = (unsigned long) packet[i];
    
    // ENVIA MASCARA
    for(i = 0; i < len_byte*NTENTATIVAS; i++){
        bytes = send(socket, mask, len_byte*TAM_PACOTE, 0);       // envia packet para o socket
        if(bytes == len_byte*TAM_PACOTE)
            break;
    }

    // NAO ENVIOU MASCARA
    if(i == NTENTATIVAS){
        fprintf(stderr, "NAO FOI POSSIVEL ENVIAR MASCARA\n");
        free(packet);
        return false;
    }

    // MASCARA FOI ENVIADA
    printf("SENT (%d) BYTES mas de real foram(%d) \n", bytes, bytes/len_byte);
    read_packet(packet);
    next(this_seq);
    free(packet);
    return true;
}

// recebe uma mensagem do socket, tenta receber mensagem NTENTATIVAS vezes, deve receber free()
// verifica se recv deu timeout, bytes > 0 e se eh nosso pacote, NAO ATUALIZA SEQUENCIA
// retorna NULL se nao foi possivel receber a msg, e a mensagem c.c.
unsigned char *recebe_msg(int socket)
{
    unsigned long buffer[TAM_PACOTE];
    int bytes, len_byte, i;

    len_byte = sizeof(unsigned long);

    // VERIFICA //
    for(i = 0; i < NTENTATIVAS; i++){
        memset(buffer, 0, len_byte*TAM_PACOTE);                     // limpa lixo de memoria antes de receber
        bytes = recv(socket, buffer, len_byte*TAM_PACOTE, 0);       // recebe dados do socket
        // if (bytes == len_byte*TAM_PACOTE)                           // recebeu tamanho do pacote
        if(bytes > 0)
            if (is_our_mask(buffer))                                // e eh nosso pacote
                break;
    }

    // nao recebeu pacote valido
    if(i == NTENTATIVAS) 
        return NULL;

    // recebeu pacote valido //
    unsigned char *pacote = calloc(TAM_PACOTE, sizeof(char ));

    // remove mascara do pacote
    for(int j = 0; j < TAM_PACOTE; j++)
        pacote[j] = (unsigned char) buffer[j];
        
    printf("RECEBEU (%d) BYTES\n", bytes);
    read_packet(pacote);

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
