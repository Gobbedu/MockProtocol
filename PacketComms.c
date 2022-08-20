#include "Packet.h"

/* ============================== STREAM FUNCTIONS ============================== */

/*  - [x] juntar os timeouts (envia)
    - [] incrementa ou nao a sequencia na mensagem NACK para a resposta? 
         se mensagem NACK nao chega, timeo no receptor + resend, proxima resposta com sequencia diferente da esperada 
    
*/


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

        printf("sending packet:\n");
        read_packet(packet);
        printf("\n");
        bytes = send(soquete, packet, TAM_PACOTE, 0);       // envia packet para o socket
        if(bytes < 0){                                      // pega erros, se algum
            printf("error: %s\n", strerror(errno));  
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
            printf("envia() recebeu resposta de (%d) bytes\n", bytes);
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
        resposta = make_packet(*this_seq, NACK, NULL, 0);
        *this_seq = ((*this_seq)+1)%MAX_SEQUENCE;

        bytes = send(soquete, resposta, TAM_PACOTE, 0);  
        if(bytes <= 0)
            printf("send error at envia : (%s); errno: (%d)\n", strerror(errno), errno);

        free(resposta);
        return NULL;                                         
    }

    // VERIFICA & !NACK, devolve pacote
    unsigned char *pacote = calloc(TAM_PACOTE, sizeof(unsigned char));
    memcpy(pacote, buffer, TAM_PACOTE);
    
    *(expected_seq) = ((*expected_seq)+1)%MAX_SEQUENCE;
    return pacote;
}



void janela_recebe4(int socket, char *file, unsigned int *this_seq, unsigned int *other_seq)
{
    printf("janela de 4 pacotes start\n");
/*
    char *dest;
    sprintf(dest, "(copy)%s", file);
    
    FILE *escreve_file;
    escreve_file = fopen(dest, "w");
  unsigned 

  while(resposta(dado) != FIM){
    int i = 0;
    while(recv(peek)){
      // valida (paridade, sequencia, erro timeo)//

      

      recebeu[i] = recv()
    }
  }
*/
}

void janela_envia4(int socket, FILE *file, unsigned int *this_seq, unsigned int *other_seq)
{

    //   int len_arr;
    // recebe char *file, nao FILE *file
    // unsigned char **packet_arr = chunck_file((*this_seq), file, &len_arr);
  
    printf("janela envia 4 pacotes start\n");
    rewind(file);       // aponta para o inicio do arquivo

/*
  int i = 0
  lost_con = false
  while((i < len_arr) && lost_con < 3)
  {  
    base = packet_arr[i].sequencia
    send(i)
    send(i+1)
    send(i+2)
    send(i+3)

    while(!resend)
    {
      if(timeo == 3)
        lost_con++
        break

      bytes = recv(soquete, resposta, TAM_PACOTE, 0);
      if(errno == EAGAIN || errno == EWOULDBLOCK){    // pega timeout
          printf("recv error : (%s); errno: (%d)\n", strerror(errno), errno);
          timeout++;
          continue;
      }

      // recebu ack ou nack //
      // VERIFICA //
      if (bytes <= 0) continue;                                   // algum erro no recv
      if (!is_our_packet((unsigned char*) resposta)) continue;    // nao eh nosso pacote
      if (!check_sequence(resposta, otherseq)){                 // sequencia incorreta
          printf("client expected %d but got %d as a sequence\n", nxts_serve, get_packet_sequence(resposta));
          continue;
      }

      case(resposta)
        NACK:
          seq = resposta(dados)
          i -= seq
          break

        ACK:
          seq = resposta(dados)
          break
      }
    }
    print deu ruim se lost_con == 3
*/

  // for(int i = 0; i < len_arr; i++)
  //   free(packet_arr[i]);
  // free(packet_arr);
}


// retorna array de pacotes (char *), todos tem tamanho cte (67 bytes)
unsigned char **chunck_file(unsigned int start_seq, char *file, int *arr_size)
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
                free(data);
                free(array);
                return NULL;
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
