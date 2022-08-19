#include "ConexaoRawSocket.h"
#include "cliente.h"
#include "Packet.h"

// - [] response_GET responde NACK, servidor deve tratar

// todos no cliente podem ver, mas servidor nao
int soquete;
int client_seq = 0;     // current client sequence
int nxts_serve = 0;     // expected next server sequence

int main(){
    char pwd[PATH_MAX];
    char comando[COMMAND_BUFF];

    soquete = ConexaoRawSocket("lo");            // abre o socket -> lo vira ifconfig to pc que manda
    // soquete = ConexaoRawSocket("enp1s0f1");   // abre o socket -> lo vira ifconfig to pc que manda

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    setsockopt(soquete, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

    while(1){
        if(getcwd(pwd, sizeof(pwd)))    // se pegou pwd, (!NULL)
            printf(GREEN "limbo@anywhere" RESET ":" BLUE "%s" RESET "$ ", pwd);

        fgets(comando, COMMAND_BUFF, stdin);
        client_switch(comando);
    } 

    close(soquete);                                        // fecha socket
    return 0;
}


/* final do comando c : client
 * final do comando s : server
 * comandos aceitos   : cd, mkdir, ls, get, put
 */
void client_switch(char* comando){
    int ret;
    comando[strcspn(comando, "\n")] = 0;                // remove new line
    char *parametro = comando;      // = comando pro make nn reclama, dpois tiro

    // LOCAL //
	if(strncmp(comando, "lsc", 3) == 0)
    {
        parametro[2] = ' ';                               // remove 'c' : lsc -> ls_
        ret = system(parametro);
        if(ret == -1)
            printf("ERRO: %s, errno: %d  parametro: (%s)\n", strerror(errno), errno, parametro);
    }
    else if(strncmp(comando, "cdc", 3) == 0)
    {
        parametro = comando+3;                              // remove "cdc"
        ret = chdir((parametro+strspn(parametro, " ")));    // remove espacos antes do parametro
        if(ret == -1)
            printf("ERRO: %s, errno: %d  parametro: (%s)\n", strerror(errno), errno, parametro);
    }
    else if(strncmp(comando, "mkdirc", 6) == 0)
    {
        parametro[5] = ' ';                               // remove 'c' : mkdirc_[]-> mkdir__[]
        ret = system(parametro);
        if(ret == -1)
            printf("ERRO: %s, errno: %d  parametro: (%s)\n", strerror(errno), errno, parametro);
    }

    // SERVIDOR //
    else if(strncmp(comando, "cds", 3) == 0)
    {   // chdir nao pode ter espacos errados
        parametro += 3;                         // remove "cds"
        parametro += strspn(parametro, " ");    // remove ' '  no inicio do comando
        if(cliente_sinaliza(parametro, CD))
            printf("moveu de diretorio no servidor com sucesso!\n");
        else
            printf("nao foi possivel mover de diretorio no servidor ...\n");
    }
    else if(strncmp(comando, "mkdirs", 6) == 0)
    {   // mkdir usa syscall, pode ter espacos
        parametro[5] = ' ';                   // "mkdirs_[...]" -> "mkdir__[...]" 

        if(cliente_sinaliza(parametro, MKDIR))
            printf("criou diretorio no servidor com sucesso!\n");
        else
            printf("nao foi possivel criar diretorio no servidor ...\n");
    }
    else if(strncmp(comando, "lss", 3) == 0)
    {   

    }
    else if (strncmp(comando, "get", 3) == 0)
    {
        parametro += 3;                       // remove "get"
        parametro += strspn(parametro, " ");    // remove ' '  no inicio do comando
        // get(comando, GET);
        cliente_sinaliza(parametro, GET);
    }
    else if (strncmp(comando, "put", 3) == 0)
    {
        
    }
    else
    {
        if(strncmp(comando, "exit", 4) == 0)
        {   // sair com estilo
            printf(RED "CLIENT TERMINATED\n" RESET);      
            exit(0);
        }
        if(comando[0] != 0)     // diferente de um enter
            printf("comando invalido: %s\n", comando);
    }
}


// talvez precise refatorar mais tarde (put & ls tb usam tipo desc_arq)
int response_GET(unsigned char* resposta, char *parametro){
    int bytes, resultado, e_livre;
    unsigned char *resposta_2;
    char pwd[PATH_MAX], flag[1];
    char* dado;
    int tamanho = 0;
    
    // MEXER AQUI !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    dado = get_packet_data(resposta);
    // pegando o tamanho do arquivo, enviado pelo servidor
    tamanho = atoi(dado);
    // verificando o diretorio atual
    getcwd(pwd, sizeof(pwd));

    char *bashdf = "df -h --output=avail / | tail -n +2 > /tmp/tamanho_gb.txt";
    char *bashrm = "rm /tmp/tamanho_gb.txt";
    system(bashdf);
    FILE *memfree = fopen("/tmp/tamanho_gb.txt", "r");
    fscanf(memfree, "%d", &e_livre);
    fclose(memfree);
    system(bashrm);


    printf("\nmeu pc tem %dG memoria livre \n",e_livre);
    // transformando o tamanho do arquivo de bytes para Gb
    tamanho /= 1024;
    tamanho /= 1024;
    tamanho /= 1024;

    // caso o tamanho do arquivo seja maior que espaço livre
    if(e_livre < tamanho){
        resultado = ERRO;
        flag[0] = sem_espaco;
        printf("Espaço insuficiente!\n");
    }
    else{
        resultado = OK;
    }
    resposta_2 = make_packet(sequencia(), resultado, flag, resultado == ERRO);
    if(!resposta_2) return;   // se pacote deu errado

    bytes = send(soquete, resposta_2, TAM_PACOTE, 0);                 // envia packet para o socket
    if(bytes<0)                                                     // pega erros, se algum
        printf("error: %s\n", strerror(errno));                     // print detalhes do erro

    free(resposta_2);
    if(resultado == ERRO)
        return false;

    // CASO resultado == OK, FAZER A LOGICA DAS JANELAS DESLIZANTES AQUI
    char *dest;
    sprintf(dest, "(copy)%s", parametro);
    FILE *escreve_file;
    escreve_file = fopen(dest, "w");
    janela_recebe4(soquete, escreve_file, client_seq, nxts_serve);
}


/* errno: 
 *  A - No such file or directory   : 2
 *  B - Permission denied           : 13
 *  C - File already exists         : 11 (?)
 */
int cliente_sinaliza(char *parametro, int tipo)
{

    int bytes, timeout, lost_conn, resend;
    unsigned char resposta[TAM_PACOTE];
    char* data;

    /* cria pacote com parametro para cd no server */
    unsigned char *packet = make_packet(sequencia(), tipo, parametro, strlen(parametro));
    if(!packet)
        fprintf(stderr, "ERRO NA CRIACAO DO PACOTE\n");

    // read_packet(packet);

    timeout = lost_conn = 0;
    // exit if received 3 timeouts, or return if OK
    while(lost_conn<3){ 
        resend = 0;

        bytes = send(soquete, packet, TAM_PACOTE, 0);       // envia packet para o socket
        if(bytes < 0){                                      // pega erros, se algum
            printf("error: %s\n", strerror(errno));  
        }

        // said do loop soh se server responde nack, (ok e erro retorna da funcao)
        timeout = 0;
        while(!resend)
        {   
            if(timeout == 3){
                lost_conn++;
                break;
            }

            bytes = recv(soquete, resposta, TAM_PACOTE, 0);
            if(errno == EAGAIN || errno == EWOULDBLOCK){    // pega timeout
                printf("recv error : (%s); errno: (%d)\n", strerror(errno), errno);
                timeout++;
                continue;
            }

            // VERIFICA //
            if (bytes <= 0) continue;                                   // algum erro no recv
            if (!is_our_packet((unsigned char*) resposta)) continue;    // nao eh nosso pacote
            if (!check_sequence(resposta, nxts_serve)){                 // sequencia incorreta
                printf("client expected %d but got %d as a sequence\n", nxts_serve, get_packet_sequence(resposta));
                continue;
            }

            // PROCESSA //
            data = get_packet_data(resposta);
            // precisa checar paridade, servidor responde dnv (???????????) nao sei
            if (!check_parity ((unsigned char *)resposta)){             
                // Paridade diferente: NACK 
                // mensagem recebida mas nao compreendida, reenviar
                printf("resposta: (%s) ; mensagem: (%s)\n", get_type_packet(resposta), data);
                lost_conn = timeout = 0;
                resend = true;
                free(data);
                break;  // exit response loop & re-send 
            }

            // nxts_serve = (nxts_serve+1)%MAX_SEQUENCE;
            switch (get_packet_type(resposta))
            {
                case OK:
                    nxts_serve = (nxts_serve+1)%MAX_SEQUENCE;
                    printf("resposta: (%s) ; mensagem: (%s)\n", get_type_packet(resposta), data);
                    free_packet(packet);
                    free(data);
                    return true;

                case DESC_ARQ:
                    nxts_serve = (nxts_serve+1)%MAX_SEQUENCE;
                    read_packet(resposta);
                    response_GET(resposta, parametro);
                    free_packet(packet);
                    free(data);
                    return true;

                case NACK:
                    // nxts_serve = (nxts_serve+1)%MAX_SEQUENCE;
                    // mensagem recebida mas nao compreendida, reenviar
                    printf("resposta: (%s) ; mensagem: (%s)\n", get_type_packet(resposta), data);
                    lost_conn = timeout = 0;
                    resend = true;
                    break;  // exit response loop & re-send 

                case ERRO:
                    nxts_serve = (nxts_serve+1)%MAX_SEQUENCE;
                    printf("resposta: (%s) ; mensagem: (%s)\n", get_type_packet(resposta), data);
                    free_packet(packet);
                    free(data);
                    return false;
            }
            free(data);
        }
    }

    printf("Erro de comunicacao, servidor nao responde :(\n");

    free_packet(packet);
    return false;
}

// atualiza e retorna proxima sequencia
unsigned int sequencia(void)
{
    int now = client_seq;
    client_seq = (client_seq+1)%MAX_SEQUENCE;
    return now;
}
// // retorna sequencia atual
// unsigned int get_seq(void){
//     return now_sequence;
// }
// // retorna sequencia passada
// unsigned int get_lastseq(void){
//     return last_sequence;
// }
// // simula qual seria a proxima sequencia
// unsigned int next_seq(void){
//     return (now_sequence+1)%MAX_SEQUENCE;
// }

