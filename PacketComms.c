#include "Packet.h"

/* ============================== STREAM FUNCTIONS ============================== */

/*  - [x] juntar os timeouts (envia)
    - [] incrementa ou nao a sequencia na mensagem NACK para a resposta? 
         se mensagem NACK nao chega, timeo no receptor + resend, proxima resposta com sequencia diferente da esperada 
    - [] se cliente nn conseguiu conversar com o servidor, nao incrementar o contador    
*/

#define NTENTATIVAS 15 // numero de vezes que vai tentar ler/enviar um pacote
// SEQUENCIA QUE RECEBEU ACK OU NACK FICA NO CAMPO DE DADOS

/* envia UM pacote com dados especificados e espera UMA resposta,
 * tenta NTENTATIVAS vezes as operacoes envolvidas
 * retorna NULL se perde conexao ou retorna o pacote se recebe resposta esperada
 * atualiza send_seq e recv_seq se deu certo, retorno deve receber free()
 */ 
unsigned char *envia_recebe(int soquete, unsigned int *send_seq, unsigned int *recv_seq, unsigned char *dados, int tipo, int bytes_dados)
{
    int lost_conn, try;
    unsigned char *resposta;

    // exit if recebe 3 timeouts da funcao
    try = lost_conn = 0;
    while(lost_conn<NTENTATIVAS){ 

        if(!envia_msg(soquete, send_seq, tipo, dados, bytes_dados)){
            lost_conn++;
            continue;
        }
        try = 0;
        // said do loop soh se server responde nack, (ok e erro retorna da funcao)
        while(try<NTENTATIVAS)
        {   

            resposta = recebe_msg(soquete);
            if(!resposta){
                fprintf(stderr, "timeo envia_recebe no recebe_msg()\n");
                try++;
                continue;
            }

            if (!check_sequence(resposta, *recv_seq)){              // sequencia incorreta
                printf("envia_recebe esperava recv_seq com %d mas recebeu %d\n", *recv_seq, get_packet_sequence(resposta));
                free(resposta);
                try++;
                continue;
            }

            // SE NACK, REENVIA //
            // precisa checar paridade, servidor responde dnv (???????????) nao sei
            // Paridade diferente: NACK 
            // mensagem recebida mas nao compreendida, reenviar
            if (!check_parity (resposta)){             
                printf("resposta: (%s) ; mensagem: (%.*s)\n", get_type_packet(resposta), get_packet_tamanho(resposta), resposta+TAM_HEADER);
                free(resposta);
                try++;
                continue;  // exit response loop & re-send 
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
        moven(send_seq, -1);
        lost_conn++;
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

int envia_sequencial(int socket, FILE *file, u_int *this_seq, u_int *other_seq, long total)
{
    time_t start, end;
    long leu_sz, blocks = 0;
    unsigned char *resposta;
    long leu_bytes = 0;          // numero de bytes lidos do arquivo (deve bater com ls)

    printf("envia sequencial start\n");
    rewind(file);

    // LIMITE CAMPO DADOS //
    int max_data = TAM_PACOTE - sizeof(envelope_packet) - 1;        // total - header - paridade
    unsigned char *data = calloc(max_data, sizeof(char));                      // info q vai no campo dados no pacote
    
    // QUEBRA ARQUIVO EM BLOCOS //
    int tentativas = 0;
    int enviou = true;
    int porcento = -1;
    // clock_t start = clock();
    time(&start);
    while(tentativas < NTENTATIVAS){  
        if(enviou){
            memset(data, 0, max_data);
            leu_sz = fread((void *)data, sizeof(char), max_data, file);
            // printf("leu %d bytes do arquivo\n", leu_sz);
            leu_bytes += leu_sz;
            // VALIDA LEITURA //
            if(leu_sz == 0){ // nao leu nada, erro ou eof
                // printf("src leu 0 bytes, break\n");
                break;
            }
            blocks++;
            enviou = false;
            if(total && (leu_bytes*100/total) > porcento )
            {
                // printf("(%ld):(%ld) ", leu_bytes, total);
                // fflush(stdout);
                ProgressBar("Enviando ", leu_bytes, total);
                porcento++;
            }
        }

        // ENVIA PACOTE NA ESPERA DE UM ACK //
        if(!envia_msg(socket, this_seq, DADOS, data, leu_sz)){
            printf("\ntimeo envia_msg() em envia_sequencial (DADOS)\n");
            tentativas++;
            continue;
        }   

        resposta = recebe_msg(socket);
        if(!resposta){
            printf("\ntimeo recebe_msg() envia_sequencial (ACK, NACK)\n");
            tentativas++;
            moven(this_seq, -1);
            continue;
        }   tentativas = 0;
       
        // read_packet(resposta);

        if(!check_sequence(resposta, *other_seq)){ // se sequencia errada
            printf("\recebe_mgs() em _sequencial esperava sequencia (%d) e recebeu(%d)\n", *other_seq, get_packet_sequence(resposta));
            return false; // por enquanto nao corrige NACK ou ACK
        }
        if(!check_parity(resposta)){    // se paridade errada
            printf("\recebe_msg() em envia_sequencial recebeu mensagem com erro na paridade\n");
            return false;
        }   
        // next(this_seq);

        // QUAL MENSAGEM RECEBEU ACK/NACK 
        if(atoi((char*)resposta+3) == peekn(*this_seq, -2)){
            // se recebeu ack/nack mensagem anterior
            printf("\nrecebe_msg() em envia_sequencial recebeu confirmacao da msg anterior\n");
            moven(this_seq, -1);
            continue;
        }   enviou = true;

        switch (get_packet_type(resposta)){
            case NACK:  // resetar fseek para enviar mesma mensagem dnv
                // printf("caiu no NACK\n");
                if(fseek(file, (long) (-leu_sz), SEEK_CUR) != 0){
                    perror("\nfseek == 0");
                    free(resposta);
                    return false;
                }
                moven(this_seq, -1);
                break;
            
            case ACK:   // continua loop normalmente
                // printf("caiu no ACK\n");
                break;

            default:
                printf("\ntipo nao definido\n");
                read_packet(resposta);
                free(resposta);
                return false;
        }
        next(other_seq);
        free(resposta);
    }

    if(tentativas == NTENTATIVAS){
        printf("\nperdeu conexao em envia_sequencial, return\n");
        free(data);
        moven(other_seq, -1);
        return false;
    }

    // ajeitar ultimo pacote
    if(!envia_msg(socket, this_seq, FIM, NULL, 0)){
        printf("\nnao foi possivel enviar_msg() FIM\n");
        return false;
    }
        
    // next(this_seq);
    free(data);
    // clock_t end = clock(); //(float)(end-start)/CLOCKS_PER_SEC
    time(&end);
    printf("\nTempo de Transferencia: (%.2f) segundos\n", difftime(end,start));
    // blocks++;
    // printf("\nfinished chunking (%d) blocks, leu (%d) bytes\n", blocks, leu_bytes);
    return true;
}

// nao damos fclose no file 
int recebe_sequencial(int socket,unsigned char *file, unsigned int *this_seq, unsigned int *other_seq, long total){    
    // charresposta[TAM_PACOTE];
    int wrote, len_data, try, w_all;
    unsigned char *pacote;
    unsigned char seq[2];

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
    // rewind(file);

    // MONTA ARQUIVO //
    try = w_all = 0;
    u_char memoria_recebe[TAM_PACOTE];  // mensagem recebida anteriormente
    int memoria_envia = ACK;               // dados mensagem enviada anteriormente
    int porcento = -1;
    while(try < NTENTATIVAS){    // soh para se recebe pacote com tipo FIM
        // tenta receber pacote 
        pacote = recebe_msg(socket);
        if(!pacote){
            try++;
            continue;
        }   try = 0;
        // read_packet(pacote);

        // FIM //
        if(get_packet_type(pacote) == FIM){
            next(other_seq);
            // free(pacote);
            break;
        }

        // ENVIA PERDEU RESPOSTA DO RECEBE //
        if(memcmp(memoria_recebe, pacote, TAM_PACOTE) == 0){
            printf("\nrecebe_sequencial recebeu o mesmo pacote de antes\n");
            moven(this_seq, -1);
            sprintf((char*)seq, "%d", *this_seq);
            envia_msg(socket, this_seq, memoria_envia, seq , 2); 
            continue;
        }

        // NACK //
        sprintf((char*) seq, "%d", *other_seq);
        if (!check_sequence(pacote, *other_seq)){
                // if(get_packet_sequence(pacote) == peekn(*other_seq, -1))
                // {   // recebeu a mesma mensagem de antes, reenvia resposta
                //     moven(this_seq, -1);
                //     sprintf((char*) seq, "%d", peekn(*other_seq, -1));
                //     envia_msg(socket, this_seq, ACK, seq, 2); // free(seq);
                //     memoria_envia = ACK;
                //     continue;
                // }
                // paridade diferente, dado: (sequencia esperada)
                printf("recebe recebeu (%d) mas esperava (%d) como sequencia\n", *other_seq, get_packet_sequence(pacote));
                // seq = itoa(*other_seq);
                envia_msg(socket, this_seq, NACK, seq, 2); // free(seq);
                // empty_netbuff(socket);
                printf("ENVIOU NACK, erro de sequencia\n");
                // free(pacote);
                continue;   // volta a ouvir

        }
        if(!check_parity(pacote)){
                printf("recebe recebeu pacote com erro de paridade, NACK\n");
                // sequencia diferente, dado:(sequencia atual)
                // seq = itoa(*other_seq);
                envia_msg(socket, this_seq, NACK, seq, 2); // free(seq);
                // empty_netbuff(socket);
                printf("ENVIOU NACK, erro de paridade\n");
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
        w_all += wrote;

        if(total && (w_all*100/total) > porcento )
        {
            ProgressBar("Recebendo ", w_all, total);
            porcento++;
        }

        // seq = ptoa(pacote);
        sprintf((char*) seq, "%d", get_packet_sequence(pacote));
        envia_msg(socket, this_seq, ACK, seq , 2); //free(seq);
        free(pacote);
    }

    fclose(dst); 
    if(try == NTENTATIVAS){
        printf("algo deu errado com recebe sequencial..\n");
        moven(this_seq, -1); // nao recebeu as varias respostas do server ou perdeu oq agnt enviou
        return false;
    }

    // printf("escreveu %d !\n", w_all);
    return true;
}

// envia uma mensagem para o socket, com verificacao de send() bytes > 0
// retorna true se enviou com sucesso, e falso c.c., ATUALIZA SEQUENCIA
// tenta enviar mensagem NTENTATIVAS vezes
int envia_msg(int socket, u_int *this_seq, int tipo, u_char *parametro, int n_bytes)
{
    int bytes, len_byte, i;
    // CRIA PACOTE
    u_char *packet = make_packet(*this_seq, tipo, parametro, n_bytes);
    if(!packet){
        fprintf(stderr, "ERRO NA CRIACAO DO PACOTE\n");
        return false;
    }

    // COLOCA MASCARA
    len_byte = sizeof(unsigned short);
    unsigned short mask[TAM_PACOTE];
    // memset(mask, -1, len_byte*TAM_PACOTE); // mascarar com 0, 1, 170, 85, 255, nao funciona
    memset(mask, 0, len_byte*TAM_PACOTE);
    for(int i = 0; i < TAM_PACOTE; i++) {
        mask[i] = (unsigned short) packet[i];
        if (packet[i] == 0x88 || packet[i] == 0x81) {
            mask[i] += 0xff00;
        }
    }
    free(packet);

    // ENVIA MASCARA
    for(i = 0; i < NTENTATIVAS;){
        bytes = send(socket, mask, len_byte*TAM_PACOTE, 0);       // envia packet para o socket
        if(bytes == len_byte*TAM_PACOTE)
            break;
        if(bytes <= 0)
        {
            perror("erro no send em envia_msg");
            i++;
        }
    //         break;
    //     usleep(0);
    }

    // NAO ENVIOU MASCARA
    if(i == NTENTATIVAS){
        fprintf(stderr, "NAO FOI POSSIVEL ENVIAR MASCARA\n");
        return false;
    }

    // MASCARA FOI ENVIADA
    // printf("SENT (%d) BYTES mas de real foram(%d) \n", bytes, bytes/len_byte);
    // read_packet((u_char *)mask);
    next(this_seq);
    return true;
}

// recebe uma mensagem do socket, tenta receber mensagem NTENTATIVAS vezes, deve receber free()
// verifica se recv deu timeout, bytes > 0 e se eh nosso pacote, NAO ATUALIZA SEQUENCIA
// retorna NULL se nao foi possivel receber a msg, e a mensagem c.c.
u_char *recebe_msg(int socket)
{
    unsigned short buffer[TAM_PACOTE];
    int bytes, len_byte, i;

    len_byte = sizeof(unsigned short);

    // VERIFICA //
    for(i = 0; i < NTENTATIVAS*NTENTATIVAS;){
    // while (1) {
        memset(buffer, 0, len_byte*TAM_PACOTE);                     // limpa lixo de memoria antes de receber
        bytes = recv(socket, buffer, len_byte*TAM_PACOTE, 0);       // recebe dados do socket
        if(bytes == len_byte*TAM_PACOTE){       // recebeu tamanho do pacote  
            if(is_our_mask(buffer))             // e eh nosso pacote
                break;                          // retorna
        }
        // if(errno == EAGAIN || errno == EWOULDBLOCK){                // se ocorreu timeout
        if(errno == EAGAIN){
            i++;
            // printf("%d \r", i);
            // perror("recebe_msg");
        }

            // perror("recv error in recebe_msg");
        // usleep(0);
    }

    // nao recebeu pacote valido
    if(i == NTENTATIVAS) 
        return NULL;

    // recebeu pacote valido //
    unsigned char *pacote = calloc(TAM_PACOTE, sizeof(char ));

    // remove mascara do pacote
    for(int j = 0; j < TAM_PACOTE; j++)
        pacote[j] = (unsigned char) buffer[j];
        
    // printf("RECEBEU (%d) BYTES\n", bytes);
    // read_packet(pacote);

    // memcpy(pacote, buffer, TAM_PACOTE);
    return pacote;
}

// atualiza sequencia dada para a proxima sequencia
unsigned int next(unsigned int *sequence){
    *sequence = ((*sequence)+1)%MAX_SEQUENCE;
    return *sequence;
}

// funciona para n em [-MAX_SEQUENCE, +inf)
// retorna sequencia movida n posicoes SEM alterar
unsigned int peekn(unsigned int sequence, int n)
{
    return (sequence+MAX_SEQUENCE+n)%MAX_SEQUENCE;
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

void ProgressBar( char label[], long step, long total )
{
    //progress width
    const int pwidth = 72;

    //minus label len
    int width = pwidth - strlen( label );
    if(!step || !width || !total)
        return;

    int pos = ( step * width ) / total ;
    
    int percent = ( step * 100 ) / total;

    printf( "%s["GREEN, label );

    //fill progress bar with =
    for ( int i = 0; i < pos; i++ )  printf( "%c", '=' );

    //fill progress bar with spaces
    printf(RESET"%*c", width - pos + 1, ']' );
    printf( " %3d%%\r", percent );
    fflush(stdout);
}