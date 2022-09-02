#include "ConexaoRawSocket.h"
#include "cliente.h"
#include "Packet.h"

// - [] response_GET responde NACK, servidor deve tratar

// todos no cliente podem ver, mas servidor nao
int soquete;
unsigned int client_seq = 0;     // current client sequence
unsigned int nxts_serve = 10;     // expected next server sequence


int main(){
    char pwd[PATH_MAX];
    char comando[COMMAND_BUFF];

    // soquete = ConexaoRawSocket("lo");            // abre o socket -> lo vira ifconfig to pc que manda
    soquete = ConexaoRawSocket("enp1s0f1");   // abre o socket -> lo vira ifconfig to pc que manda
    // soquete = ConexaoRawSocket("eno1");

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
        if(cliente_sinaliza((unsigned char*)parametro, CD))
            printf("moveu de diretorio no servidor com sucesso!\n");
        else
            printf("nao foi possivel mover de diretorio no servidor ...\n");
    }
    else if(strncmp(comando, "mkdirs", 6) == 0)
    {   // mkdir usa syscall, pode ter espacos
        parametro[5] = ' ';                   // "mkdirs_[...]" -> "mkdir__[...]" 

        if(cliente_sinaliza((unsigned char*)parametro, MKDIR))
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
        cliente_sinaliza((unsigned char*)parametro, GET);
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
int response_GET(unsigned char * resposta_srv, unsigned char *file){
    // int bytes, resultado, mem_livre;
    long mem_livre, tamanho = 0;
    char pwd[PATH_MAX];
    unsigned char* dado;
    
    dado = get_packet_data(resposta_srv);
    tamanho = atoi((char*)dado);               // pegando o tamanho do arquivo em bytes, enviado pelo servidor
    free(dado);
    getcwd(pwd, sizeof(pwd));           // verificando o diretorio atual

    // CALCULA MEMORIA DISPONIVEL // (em bytes)
    char *bashdf = "df --output=avail --block-size=1 / | tail -n +2 > /tmp/tamanho_gb.txt";
    char *bashrm = "rm /tmp/tamanho_gb.txt";
    system(bashdf);
    FILE *memfree = fopen("/tmp/tamanho_gb.txt", "r");
    fscanf(memfree, "%ld", &mem_livre);
    fclose(memfree);
    system(bashrm);

    // caso o tamanho do arquivo seja maior que espaço livre
    printf("\nmeu pc tem %ld bytes de memoria livre \n",mem_livre);

    // ARQUIVO NAO CABE, retorna //
    if(mem_livre < tamanho){
        printf("Espaço insuficiente!\n");
        if(!envia_msg(soquete, &client_seq, ERRO, NULL, 0))
            printf("Não foi possivel responder o erro!\n");
        // resposta_cli = make_packet(sequencia(), ERRO, &sem_espaco, 1);
        // bytes = send(soquete, resposta_cli, TAM_PACOTE, 0);     // envia packet para o socket
        // if(bytes<0)                                             // pega erros, se algum
        //     printf("falha ao enviar pacote de resposta do get, erro: %s\n", strerror(errno));         // print detalhes do erro
        // free(resposta_cli);
        return false;
    }

    // ARQUIVO CABE //
    if(!envia_msg(soquete, &client_seq, OK, NULL, 0)){
        printf("nao foi possivel responder ok para o servidor\n");
        return false;
    }

    // CASO resultado == OK, FAZER A LOGICA DAS JANELAS DESLIZANTES AQUI
    if(recebe_sequencial(soquete, file, &client_seq, &nxts_serve)){
        printf("arquivo (%s) transferido com sucesso!\n", file);
        return true;
    }

    printf("nao foi possivel transferir o arquivo (%s)\n", file);
    // moven(&nxts_serve, -1);
    // next(&nxts_serve);
    return false;
}


/* errno: 
 *  A - No such file or directory   : 2
 *  B - Permission denied           : 13
 *  C - File already exists         : 11 (?)
 */
int cliente_sinaliza(unsigned char *parametro, int tipo)
{
    // int bytes, timeout, lost_conn, resend;
    // charresposta[TAM_PACOTE];
    unsigned char *resposta;
    unsigned char *data;

    // /* cria pacote com parametro para cd no server */
    // envia pacote pro servidor e aguarda uma resposta
    resposta = envia_recebe(soquete, &client_seq, &nxts_serve, parametro, tipo, strlen((char*)parametro));
    if(!resposta) return false;

    data = get_packet_data(resposta);
    switch (get_packet_type(resposta))
    {
        case OK:        // resposta de (cd, mkdir, put)
            printf("resposta: (%s) ; mensagem: (%s)\n", get_type_packet(resposta), data);
            // free_packet(packet);
            free(resposta);
            free(data);
            return true;

        case DESC_ARQ:  // resposta de (get)
            read_packet(resposta);
            response_GET(resposta, parametro);  // parametro eh file
            // free_packet(packet);
            free(resposta);
            free(data);
            return true;

        case ERRO:      // resposta de (ls, cd, mkdir, put, get)
            printf("resposta: (%s) ; mensagem: (%s)\n", get_type_packet(resposta), data);
            // free_packet(packet);
            free(resposta);
            free(data);
            return false;

        case SHOW_NA_TELA:  // resposta do ls
            return true;
    }

    printf("nao caiu em nenhum caso acima\n");
    read_packet(resposta);

    free(resposta);
    return false;
}

// atualiza e retorna proxima sequencia
unsigned int sequencia(void)
{
    int now = client_seq;
    client_seq = (client_seq+1)%MAX_SEQUENCE;
    return now;
}

