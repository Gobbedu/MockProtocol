#include "Packet.h"

/* ============================== STREAM FUNCTIONS ============================== */

/*  - [x] juntar os timeouts (envia)
    - [] incrementa ou nao a sequencia na mensagem NACK para a resposta? 
         se mensagem NACK nao chega, timeo no receptor + resend, proxima resposta com sequencia diferente da esperada 
    
*/

#define NTENTATIVAS 3 // numero de vezes que vai tentar ler/enviar um pacote
// SEQUENCIA QUE RECEBEU ACK OU NACK FICA NO CAMPO DE DADOS

/* envia UM pacote e espera UMA resposta,
 * re-envia caso nack ou timeout, 
 * retorna NULL se perde conexao ou o pacote se recebe resposta esperada
 * atualiza expected_seq, se deu certo
 */ 
unsigned char *envia(int soquete, unsigned char* packet, unsigned int *expected_seq)
{
    int bytes, timeout, lost_conn, resend;
    unsigned char resposta[TAM_PACOTE];
    char* data;

    // exit if recebe 3 timeouts da funcao
    timeout = lost_conn = 0;
    while(lost_conn<3){ 
        resend = 0;

        // printf("sending packet:\n");
        // read_packet(packet);
        // printf("\n");
        bytes = send(soquete, packet, TAM_PACOTE, 0);       // envia packet para o socket
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

            bytes = recv(soquete, (void*) resposta, TAM_PACOTE, 0); 
            if(errno == EAGAIN || errno == EWOULDBLOCK){    // se ocorreu timeout
                printf("recv error : (%s); errno: (%d)\n", strerror(errno), errno);
                timeout++;
                continue;
            }

            // VERIFICA //
            if (bytes <= 0) continue;                                   // algum erro no recv
            if (!is_our_packet(resposta)) continue;    // nao eh nosso pacote
            // read_packet(resposta);
            // por enquanto se sequencia errada, ignora
            // talvez o certo seja nack(sequencia esperada)
            // [talvez soh pros dados precise dessa logica]
            if (!check_sequence(resposta, *expected_seq)){              // sequencia incorreta
                printf("client expected %d but got %d as a sequence\n", *expected_seq, get_packet_sequence(resposta));
                continue;
            }

            // SE NACK, REENVIA //
            // precisa checar paridade, servidor responde dnv (???????????) nao sei
            // Paridade diferente: NACK 
            // mensagem recebida mas nao compreendida, reenviar
            if (!check_parity ((unsigned char *)resposta)){             
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
            unsigned char *pacote = calloc(TAM_PACOTE, sizeof(unsigned char));
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
unsigned char *recebe(int soquete, unsigned int *this_seq, unsigned int *expected_seq)
{
    int bytes;
    char *seq;
    unsigned char *resposta;
    unsigned char buffer[TAM_PACOTE];
    bytes = recv(soquete, buffer, TAM_PACOTE, 0);       // recebe dados do socket

    // VERIFICA //
    if (bytes<=0) return NULL;                 // se erro ou vazio, ignora
    if (!is_our_packet(buffer)) return NULL;   // se nao eh nosso pacote, ignora
    if (!check_sequence((unsigned char*) buffer, *expected_seq)){
        // sequencia diferente: ignora
        printf("server expected %d but got %d as a sequence\n", *expected_seq, get_packet_sequence(buffer));
        return NULL;
    }
    
    // paridade diferente: NACK
    if (!check_parity(buffer)){  
        seq = ptoa(buffer);
        resposta = make_packet(*this_seq, NACK, seq, 0); free(seq);       // nack, dados: str(sequencia)
        *this_seq = ((*this_seq)+1)%MAX_SEQUENCE;

        bytes = send(soquete, resposta, TAM_PACOTE, 0);  
        if(bytes <= 0)
            perror("send error at envia ");
            // printf("send error at envia : (%s); errno: (%d)\n", strerror(errno), errno);

        free(resposta);
        return NULL;                                         
    }

    // VERIFICA & !NACK, devolve pacote
    unsigned char *pacote = calloc(TAM_PACOTE, sizeof(unsigned char));
    memcpy(pacote, buffer, TAM_PACOTE);
    
    *(expected_seq) = ((*expected_seq)+1)%MAX_SEQUENCE;
    return pacote;
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

/*
void janela_recebe4(int socket, char *file, unsigned int *this_seq, unsigned int *other_seq)
{
    unsigned char *resposta, buffer[TAM_PACOTE];
    int type, bytes, len_data, wrote, i;
    FILE *escreve_file;
    char *dest, *seq;

    printf("janela recebe de 4 pacotes start\n");
    dest = calloc(6+strlen(file), sizeof(char));
    sprintf(dest, "(copy)%s", file);
    
    escreve_file = fopen(dest, "w");
    free(dest);

    type = -1;            
    while(type != FIM){     // sai do loop se tipo da mensagem != FIM
        i = 0;
        // le no maximo 4 pacotes nossos & responde 
        while(i < 4 && recv(socket, buffer, TAM_PACOTE, MSG_PEEK) > 0){
            bytes = recv(socket, buffer, TAM_PACOTE, 0);
            
            // VALIDA //
            if(bytes <= 0){
                perror("erro recebe4");
                continue;
            }
            if (!is_our_packet(buffer)) continue;  // se nao eh nosso pacote, ignora

            read_packet(buffer);
            type = get_packet_type(buffer);
            if(type == FIM)
                break;

            // NACK //
            if (!check_sequence((unsigned char*) buffer, *other_seq) ||
                !check_parity(buffer))
            {
                if(!check_parity(buffer)){  // dado:(sequencia que esperava)
                    // paridade diferente
                    printf("recebe4 recebeu pacote com erro de paridade, NACK\n");
                    seq = ptoa(buffer);
                    resposta = make_packet(*this_seq, NACK, seq, 2); free(seq);
                }
                else{   //  dado:(sequencia atual)
                    // sequencia diferente
                    printf("recebe4 recebeu (%d) mas esperava (%d) como sequencia\n", *other_seq, get_packet_sequence(buffer));
                    seq = itoa(*other_seq);
                    resposta = make_packet(*this_seq, NACK, seq, 2); free(seq);
                }
            
                // responde NACK & limpa buffer da placa de rede (vai receber novo)
                bytes = send(socket, resposta, TAM_PACOTE, 0);  
                if(bytes <= 0){
                    perror("return, envia4 nack error");
                    return;
                }
                *this_seq = ((*this_seq)+1)%MAX_SEQUENCE;

                empty_netbuff(socket);

                free(resposta);
                break;                                       
            }

            // ACK //
            *other_seq = ((*other_seq)+1)%MAX_SEQUENCE;
            seq = itoa(get_packet_sequence(buffer));        // salva ultima sequencia aceita
            len_data  = get_packet_tamanho(buffer);         // tamanho em bytes a escrever no arquivo

            // data comeca 3 bytes depois do inicio
            wrote = fwrite((void*) (buffer+3), sizeof(char), len_data, escreve_file);
            if(wrote == 0){
                perror("return, build wrote 0 bytes, ");
                return;
            }
            i++;
        }
        // leu 4 ou acabou buffer, responde ok ultima seq
        resposta = make_packet(*this_seq, ACK, seq, 2);

    }

}

void janela_envia4(int socket, FILE *file, unsigned int *this_seq, unsigned int *other_seq)
{
    unsigned char **packet_arr, resposta[TAM_PACOTE];
    int len_arr, base, bytes, seq, i;
    int timeo, lost_con;
    char *data;

    printf("janela envia 4 pacotes start\n");

    rewind(file);       // aponta para o inicio do arquivo
    packet_arr = chunck_file((*this_seq), file, &len_arr);

    i = 0;
    lost_con = false;
    while((i < len_arr) && (lost_con < 3))
    {  
        base = get_packet_sequence(packet_arr[i]);
        for(int x = 0; (i+x < len_arr) && x < 4; x++)
            send(socket, packet_arr[i+x], TAM_PACOTE, 0);
        // send(socket, packet_arr[i+1], TAM_PACOTE, 0);
        // send(socket, packet_arr[i+2], TAM_PACOTE, 0);
        // send(socket, packet_arr[i+3], TAM_PACOTE, 0);

        while(1){
            if(timeo == 3){
                lost_con++;
                break;
            }

            bytes = recv(socket, resposta, TAM_PACOTE, 0);
            if(errno == EAGAIN || errno == EWOULDBLOCK){    // pega timeout
                // printf("recv error : (%s); errno: (%d)\n", strerror(errno), errno);
                perror("ERROR recv envia4");
                timeo++;
                continue;
            }

            // recebu ack ou nack //
            // VERIFICA //
            if (bytes <= 0) continue;                                   // algum erro no recv
            if (!is_our_packet((unsigned char*) resposta)) continue;    // nao eh nosso pacote
            if (!check_sequence(resposta, *other_seq)){                 // sequencia incorreta
                printf("envia4 expected %d but got %d as a sequence, ignore\n", *other_seq, get_packet_sequence(resposta));
                continue;
            }
            read_packet(resposta);

            data = get_packet_data(resposta);       // valor da sequencia em char*
            seq = n_forward(base, atoi(data));      // data estah a (seq) janelas pra frente da (base)
            free(data);

            switch (get_packet_type(resposta))
            {
            case NACK:          // perdeu sequencia x, reenvia x
                i += seq;       // aceita sequencia < x
                timeo = lost_con = 0;
                break;
            
            case ACK:           // aceitou sequencia x, entao envia x+1 
                i += seq + 1;   // aceita sequencia <= x
                timeo = lost_con = 0;
                break;
            }
        }
    }

    if(lost_con == 3)
        printf("Erro de comunicacao, servidor nao responde :(\n");
    else
        printf("arquivo foi enviado com sucesso!\n");

    
    for(int i = 0; i < len_arr; i++)
        free(packet_arr[i]);
    free(packet_arr);
}
*/

int envia_sequencial(int socket, FILE *file, unsigned int *this_seq, unsigned int *other_seq)
{
    int leu_sz, max_data, blocks = 0;
    unsigned char resposta[TAM_PACOTE];
    char *data;
    int leu_bytes = 0;          // numero de bytes lidos do arquivo (deve bater com ls)

    printf("envia sequencial start\n");
    rewind(file);

    // LIMITE CAMPO DADOS //
    max_data = TAM_PACOTE - sizeof(envelope_packet) - 1;        // total - header - paridade
    data = calloc(max_data, sizeof(char));                      // info q vai no campo dados no pacote
    
    // QUEBRA ARQUIVO EM BLOCOS //
    printf("chunking...\n");
    while(1){  
        leu_sz = fread((void *)data, sizeof(char), max_data, file);
        printf("leu %d bytes do arquivo\n", leu_sz);
        leu_bytes += leu_sz;
        blocks++;
        // VALIDA LEITURA //
        if(leu_sz == 0){ // nao leu nada, erro ou eof
            printf("src leu 0 bytes, break\n");
            break;
        }

        // ENVIA PACOTE NA ESPERA DE UM ACK //
        if(!envia_msg(socket, this_seq, DADOS, data, leu_sz)){
            printf("ERRO, nao foi possivel enviar_msg() em envia sequencial\n");
            return false;
        }

        // if(feof(file)){  // termina se fim do arquivo
        //     printf("src feof break\n");
        //     break;
        // }
        
        // ORIGINAL Q FUNCIONA
        int bytes = recv(socket, resposta, TAM_PACOTE, 0);
      
        // tentar fazer do jeito certo abaixo
        // bytes = recv(socket, resposta, TAM_PACOTE, 0);
        // if(errno == EAGAIN || errno == EWOULDBLOCK){     
        //     fprintf(stderr, "tentativa (%d), ", try+1);   
        //     perror("ERRO timeout recebe_msg()");
        //     try++;
        //     continue;
        // }
        // if(bytes <= 0){
        //     try++;
        //     perror("ERRO ao receber pacote em recebe_sequencial");
        //     continue;
        // }
        // if(!is_our_packet(pacote)){
        //     try++;
        //     continue;
        // }try = 0;
        // read_packet(pacote);

        // resposta = recebe_msg(socket);
        // if(!resposta){  // se NULL
        //     printf("envia sequencial recebeu resposta NULL\n");
        //     return false;
        // }
        if(!check_sequence(resposta, *other_seq)){ // se sequencia errada
            printf("recebe_mgs() em envia sequencial esperava sequencia (%d) e recebeu(%d)\n", *other_seq, get_packet_sequence(resposta));
            return false; // por enquanto nao corrige NACK ou ACK
        }
        if(!check_parity(resposta)){    // se paridade errada
            printf("recebe_msg() recebeu mensagem com erro na paridade\n");
            return false;
        }
        read_packet(resposta);

        switch (get_packet_type(resposta)){
            case NACK:  // resetar fseek para enviar mesma mensagem dnv
                printf("caiu no NACK\n");
                if(fseek(file, (long) (-leu_sz), SEEK_CUR) != 0){
                    perror("fseek == 0");
                    // free(resposta);
                    return false;
                }
                break;
            
            case ACK:   // continua loop normalmente
                printf("caiu no ACK\n");
                break;

            default:
                printf("tipo nao definido\n");
                // free(resposta);
                return false;
        }
        next(other_seq);
        // free(resposta);
    }
    // ajeitar ultimo pacote
    if(!envia_msg(socket, this_seq, FIM, NULL, 0)){
        printf("nao foi possivel enviar_msg() FIM\n");
        
    }
    blocks++;
    free(data);
    fclose(file);

    printf("finished chunking (%d) blocks, leu (%d) bytes\n", blocks, leu_bytes);
    return true;
}

int recebe_sequencial(int socket, char *file, unsigned int *this_seq, unsigned int *other_seq){    
    // unsigned char resposta[TAM_PACOTE];
    int wrote, len_data, try, bytes;
    unsigned char pacote[TAM_PACOTE];
    // unsigned char *pacote;
    char *seq;

    printf("recebe sequencial start\n");

    // DESTINO //
    char *dest = calloc(6+strlen(file), sizeof(char));
    sprintf(dest, "(copy)%s", file);
    
    FILE *dst = fopen(dest, "w");
    // FILE *dst = fopen("teste_dst.txt", "w");
    if(!dst){
        printf("could not open destine file, terminate\n");
        return false; 
    }

    // MONTA ARQUIVO //
    try = 0;
    while(try < NTENTATIVAS){    // soh para se recebe pacote com tipo FIM
        // tenta receber pacote 
        // pacote = recebe_msg(socket);
        // if(!pacote){
        //     try = 1;
        //     break;
        // }
        bytes = recv(socket, pacote, TAM_PACOTE, 0);
        if(errno == EAGAIN || errno == EWOULDBLOCK){     
            fprintf(stderr, "tentativa (%d), ", try+1);   
            perror("ERRO timeout recebe_msg()");
            try++;
            continue;
        }
        if(bytes <= 0){
            try++;
            perror("ERRO ao receber pacote em recebe_sequencial");
            continue;
        }
        if(!is_our_packet(pacote)){
            try++;
            continue;
        }try = 0;
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
                printf("recebe4 recebeu (%d) mas esperava (%d) como sequencia\n", *other_seq, get_packet_sequence(pacote));
                seq = itoa(*other_seq);
                envia_msg(socket, this_seq, NACK, seq, 2); free(seq);
                empty_netbuff(socket);
                // free(pacote);
                continue;   // volta a ouvir
        }
        if(!check_parity(pacote)){
                printf("recebe4 recebeu pacote com erro de paridade, NACK\n");
                // sequencia diferente, dado:(sequencia atual)
                seq = itoa(*other_seq);
                envia_msg(socket, this_seq, NACK, seq, 2); free(seq);
                empty_netbuff(socket);
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
    if(try == 1){
        printf("algo deu errado com recebe sequencial..\n");
        return false;
    }
    return true;
}

// retorna array de pacotes (char *), todos tem tamanho cte (67 bytes)
unsigned char **chunck_file(unsigned int start_seq, FILE *src, int *arr_size)
{
    unsigned char **array, **bigger_array;
    int eof, leu_sz, max_data;
    char *data;

    int leu_bytes = 0;          // numero de bytes lidos do arquivo (deve bater com ls)
    int allocd_sz = 1;          // memoria alocada no vetor de retorno
    *arr_size = 0;              // blocos utilizados no vetor
    
    /*
    // ABRE ARQUIVO //
    FILE *src = fopen(file, "r");                           // arquivo a enviar
    if(src == NULL){
        printf("something went wrong opening file (%s)\n return\n", file);
        return NULL;
    }
    */

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

        // AUMENTA VETOR DE PACOTE (se precisar)// 
        if((*arr_size) >= allocd_sz){
            allocd_sz = allocd_sz*2;      // aloca 2, 4, 8, 16, ...
            bigger_array = reallocarray(array, allocd_sz, sizeof(unsigned char**));
            if(bigger_array == 0){
                perror("ERRO ");
                printf("\nwhile increasing array of packets, exit\n");
                free(data);
                free(array);
                return NULL;
            }
            else{ // aumentou tamanho o array
                array = bigger_array;
            }
        }
    }
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

    printf("finished chunking (%d) blocks, leu (%d) bytes\n", *arr_size, leu_bytes);
    
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
    FILE *dst = fopen(file, "w");
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

// envia uma mensagem para o socket, com verificacao de send() bytes > 0
// retorna true se enviou com sucesso, e falso c.c., atualizando a sequencia de acordo
// tenta enviar mensagem NTENTATIVAS vezes
int envia_msg(int socket, unsigned int *this_seq, int tipo, char *parametro, int n_bytes)
{
    int bytes, tentativas;
    unsigned char *packet = make_packet(*this_seq, tipo, parametro, n_bytes);
    if(!packet){
        fprintf(stderr, "ERRO NA CRIACAO DO PACOTE\n");
        return false;
    }

    tentativas = 0;
    while(tentativas < NTENTATIVAS){
        bytes = send(socket, packet, TAM_PACOTE, 0);       // envia packet para o socket
        if(bytes <= 0){                                      // pega erros, se algum
            fprintf(stderr, "(%d) tentativa, ", tentativas);   
            perror("ERROR envia_msg() send <= 0");
            tentativas++;
            continue;
        }
        tentativas = 0;
        break;  // enviou, termina loop
    }
    if(tentativas == NTENTATIVAS){
        free(packet);
        return false;
    }

    next(this_seq);
    free(packet);
    return true;
}

// recebe uma mensagem do socket, tenta receber mensagem NTENTATIVAS vezes, deve receber free()
// verifica se recv deu timeout, bytes > 0 e se eh nosso pacote
// retorna NULL se nao foi possivel receber a msg, e a mensagem c.c.
unsigned char *recebe_msg(int socket)
{
    unsigned char *pacote, resposta[TAM_PACOTE];
    int bytes, tentativas;

    // resposta = calloc(TAM_PACOTE, sizeof(unsigned char));
    tentativas = 0;
    while(tentativas < NTENTATIVAS){
        bytes = recv(socket, (void*) resposta, TAM_PACOTE, 0); 
        // se ocorreu timeout
        if(errno == EAGAIN || errno == EWOULDBLOCK){     
            fprintf(stderr, "tentativa (%d), ", tentativas+1);   
            perror("ERRO timeout recebe_msg()");
            tentativas++;
            continue;
        }

        // algum erro no recv
        if (bytes <= 0){    
            perror("ERRO recebe_mgs() recv <= 0");
            tentativas++;
            continue;
        }                                   

        // nao eh nosso pacote
        if (!is_our_packet(resposta)){
            tentativas++;  
            continue;
        }
        // recebeu bytes > 0 && eh nosso pacote
        tentativas = 0;
        break;        
    } 

    // nao deu certo, bateu no limite de tentativas
    if(tentativas == NTENTATIVAS){
        // if(resposta) free(resposta);
        return NULL;
    }

    // deu certo, um pacote foi lido
    pacote = calloc(TAM_PACOTE, sizeof(unsigned char));
    memcpy(pacote, resposta, TAM_PACOTE);
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
    *sequence = ((*sequence)+n)%MAX_SEQUENCE;
    return *sequence;
}

// transorma sequencia do pacote em string, deve receber free()
char *ptoa(unsigned char *pacote){
    char *text = calloc(2, sizeof(char));
    sprintf(text, "%d", get_packet_sequence(pacote));
    return text;
}

// transorma inteiro em string, deve receber free()
char *itoa(int sequencia){
    char *text = calloc(2, sizeof(char));
    sprintf(text, "%d", sequencia);
    return text;
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
