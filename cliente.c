#include "ConexaoRawSocket.h"
#include "cliente.h"
#include "Packet.h"

// - [] response_GET responde NACK, servidor deve tratar

// todos no cliente podem ver, mas servidor nao
int soquete;
unsigned int client_seq = 0;     // current client sequence
unsigned int nxts_serve = 10;     // expected next server sequence

void print(char *buf){
    for(int i = 0; i < 63; i++)
        printf("%d,", buf[i]);
    printf("fim\n");
}


int main(){
    char pwd[PATH_MAX];
    char comando[COMMAND_BUFF];

    // soquete = ConexaoRawSocket("lo");            // abre o socket -> lo vira ifconfig to pc que manda
    soquete = ConexaoRawSocket("enp1s0f1");   // abre o socket -> lo vira ifconfig to pc que manda

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    setsockopt(soquete, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

   /* char buffer[TAM_PACOTE];
    char*buff;
    send(soquete, make_packet(sequencia(), MKDIR, "1o mkdir", 8), TAM_PACOTE, 0);
    send(soquete, make_packet(sequencia(), MKDIR, "2o mkdir", 8), TAM_PACOTE, 0);
    send(soquete, make_packet(sequencia(), MKDIR, "3o mkdir", 8), TAM_PACOTE, 0);
    send(soquete, make_packet(sequencia(), MKDIR, "4o mkdir", 8), TAM_PACOTE, 0);
    sleep(1);
    recv(soquete, buffer, TAM_PACOTE, 0);
    // buff = recebe(soquete, &client_seq, &nxts_serve);
    perror("ERRO ");
    read_packet(buffer);
    send(soquete, make_packet(sequencia(), CD,    "emfim cd", 8), TAM_PACOTE, 0);
*/

    char buf[63];
    memset(buf, 0, 63);

    // send
    // char ok[63] = {-43,86,78,91,64,56,32,-127,-103,33,4,81,76,67,-34,110,38,97,41,75,-23,-9,32,119,-24,69,42,-70,76,-60,-100,105,26,-45,48,78,-127,-68,84,-99,75,-77,32,13,49,-20,10,69,-34,-125,107,102,70,12,39,-69,-51,-86,96,71,-6,64,66};
    char ok[63] = {117,149,105,188,100,163,49,20,119,129,0,30,173,248,68,200,143,194,140,16,214,226,183,238,8,187,187,252,20,150,226,177,88,173,222,239,190,240,80,22,4,189,221,34,102,248,140,192,250,48,124,8,248,60,144,138,181,194,33,66,73,157,170};
    printf("tam ok: %ld\n", strlen(ok));
    send(soquete, ok, 63, 0);
    // recv
    // recv(soquete, buf, 63, 0);
    // printf("tam buf: %d\n", strlen(buf));
    // print(buf);
    // send
    // char quebra[63] = {48,102,78,25,-124,-111,49,-120,0,-127,0,80,82,114,8,52,4,-60,-108,-14,56,12,64,-79,-52,-117,87,39,-90,60,100,98,10,91,83,80,110,-43,93,67,-63,-101,-125,25,2,-12,-54,-99,-101,-85,87,69,101,102,-92,62,-27,-100,-104,-13,48,-90,82};
    char no[63] = {83,100,0,147,115,57,0,147,216,129,0,147,246,209,0,148,22,129,0,148,53,131,0,148,85,29,0,148,116,34,0,148,152,244,0,149,19,22,0,149,71,225,0,149,101,218,0,149,134,6,0,149,179,125,0,149,226,186,0,150,15,184,0};
    printf("tam no: %ld\n", strlen(no));
    send(soquete, no, 63, 0);
    // recv
    // memset(buf, 0, 63);
    // recv(soquete, buf, 63, 0);
    // printf("tam buf: %d\n", strlen(buf));
    // print(buf);

/* da problema
    char ok[63] = {-43,86,78,91,64,56,32,-127,-103,33,4,81,76,67,-34,110,38,97,41,75,-23,-9,32,119,-24,69,42,-70,76,-60,-100,105,26,-45,48,78,-127,-68,84,-99,75,-77,32,13,49,-20,10,69,-34,-125,107,102,70,12,39,-69,-51,-86,96,71,-6,64,66};
    envia_msg(soquete, &client_seq, DADOS, ok, 63);

    char *resp = recebe_msg(soquete);
    read_packet(buf);

    char quebra[63] = {48,102,78,25,-124,-111,49,-120,0,-127,0,80,82,114,8,52,4,-60,-108,-14,56,12,64,-79,-52,-117,87,39,-90,60,100,98,10,91,83,80,110,-43,93,67,-63,-101,-125,25,2,-12,-54,-99,-101,-85,87,69,101,102,-92,62,-27,-100,-104,-13,48,-90,82};
    envia_msg(soquete, &client_seq, DADOS, quebra, 63);

    resp = recebe_msg(soquete);
    read_packet(resp);
    free(resp);
*/
    

    while(0){
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

    //======================== LOCAL ========================//
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

    //======================== SERVIDOR ========================//
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
        if(strncmp(comando, "print", 5) == 0){
            printf("seq servidor esperada: %d\nsequencia cliente: %d\n ", nxts_serve, client_seq);
        }
    }
}


// talvez precise refatorar mais tarde (put & ls tb usam tipo desc_arq)
int response_GET(char * resposta_srv, char *file){
    // int bytes, resultado, mem_livre;
    char*resposta_cli;
    int bytes, mem_livre;
    char pwd[PATH_MAX];
    int tamanho = 0;
    char* dado;
    
    dado = get_packet_data(resposta_srv);
    tamanho = atoi(dado);               // pegando o tamanho do arquivo em bytes, enviado pelo servidor
    free(dado);
    getcwd(pwd, sizeof(pwd));           // verificando o diretorio atual

    // CALCULA MEMORIA DISPONIVEL // (em bytes)
    char *bashdf = "df --output=avail / | tail -n +2 > /tmp/tamanho_gb.txt";
    char *bashrm = "rm /tmp/tamanho_gb.txt";
    system(bashdf);
    FILE *memfree = fopen("/tmp/tamanho_gb.txt", "r");
    fscanf(memfree, "%d", &mem_livre);
    fclose(memfree);
    system(bashrm);

    // caso o tamanho do arquivo seja maior que espaço livre
    printf("\nmeu pc tem %d bytes de memoria livre \n",mem_livre);

    // ARQUIVO NAO CABE, retorna //
    if(mem_livre < tamanho){
        printf("Espaço insuficiente!\n");
        resposta_cli = make_packet(sequencia(), ERRO, (char*) &sem_espaco, 1);
        bytes = send(soquete, resposta_cli, TAM_PACOTE, 0);     // envia packet para o socket
        if(bytes<0)                                             // pega erros, se algum
            printf("falha ao enviar pacote de resposta do get, erro: %s\n", strerror(errno));         // print detalhes do erro
        free(resposta_cli);
        return false;
    }

    // ARQUIVO CABE //
    resposta_cli = make_packet(sequencia(), OK, NULL, 0);
    if(!resposta_cli){  // se pacote deu errado
        printf("falha ao criar pacote de resposta do get (cliente), terminando\n");
        return false;   
    } 

    bytes = send(soquete, resposta_cli, TAM_PACOTE, 0);     // envia packet para o socket
    if(bytes<0)                                             // pega erros, se algum
        printf("falha ao enviar pacote de resposta do get (cliente), erro: %s\n", strerror(errno));         // print detalhes do erro
    free(resposta_cli);

    // CASO resultado == OK, FAZER A LOGICA DAS JANELAS DESLIZANTES AQUI
    if(recebe_sequencial(soquete, file, &client_seq, &nxts_serve))
        printf("arquivo (%s) transferido com sucesso!\n", file);
    else
        printf("nao foi possivel transferir o arquivo (%s)\n", file);

    return true;
}


/* errno: 
 *  A - No such file or directory   : 2
 *  B - Permission denied           : 13
 *  C - File already exists         : 11 (?)
 */
int cliente_sinaliza(char *parametro, int tipo)
{
    // int bytes, timeout, lost_conn, resend;
    // charresposta[TAM_PACOTE];
    char*resposta;
    char* data;

    /* cria pacote com parametro para cd no server */
    char*packet = make_packet(sequencia(), tipo, parametro, strlen(parametro));
    if(!packet)
        fprintf(stderr, "ERRO NA CRIACAO DO PACOTE\n");

    // envia pacote pro servidor e aguarda uma resposta
    resposta = envia(soquete, packet, &nxts_serve); // se enviou atualiza sequencia nxts_serve
    if(!resposta) return false;

    data = get_packet_data(resposta);
    switch (get_packet_type(resposta))
    {
        case OK:        // resposta de (cd, mkdir, put)
            printf("resposta: (%s) ; mensagem: (%s)\n", get_type_packet(resposta), data);
            free_packet(packet);
            free(resposta);
            free(data);
            return true;

        case DESC_ARQ:  // resposta de (get)
            read_packet(resposta);
            response_GET(resposta, parametro);  // parametro eh file
            free_packet(packet);
            free(resposta);
            free(data);
            return true;

        case ERRO:      // resposta de (ls, cd, mkdir, put, get)
            printf("resposta: (%s) ; mensagem: (%s)\n", get_type_packet(resposta), data);
            free_packet(packet);
            free(resposta);
            free(data);
            return false;

        case SHOW_NA_TELA:  // resposta do ls
            return true;
    }

    printf("nao caiu em nenhum caso acima\n");
    read_packet(resposta);

    free(resposta);
    free(packet);
    return false;
}

// atualiza e retorna proxima sequencia
unsigned int sequencia(void)
{
    int now = client_seq;
    client_seq = (client_seq+1)%MAX_SEQUENCE;
    return now;
}

