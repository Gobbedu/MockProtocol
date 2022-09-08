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

    // soquete = ConexaoRawSocket("lo");         // abre o socket -> lo vira ifconfig to pc que manda
    soquete = ConexaoRawSocket("enp1s0f1");   // abre o socket -> lo vira ifconfig to pc que manda
    // // soquete = ConexaoRawSocket("eno1");
    // soquete = ConexaoRawSocket("enp3s0");   // pc gobbo

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
        parametro += 3;                         // remove "get"
        parametro += strspn(parametro, " ");    // remove ' '  no inicio do comando
        // get(comando, GET);
        cliente_sinaliza((unsigned char*)parametro, GET);
    }
    else if (strncmp(comando, "put", 3) == 0)
    {
        parametro += 3;                         // remove "put"
        parametro += strspn(parametro, " ");    // remove ' '  no inicio do comando
        if(cliente_sinaliza((u_char*) parametro, PUT))
            response_PUT((u_char*) parametro);        
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

    // DESTINO //
    // char *dest = calloc(6+strlen((char*)file), sizeof(char));
    // sprintf(dest, "(copy)%s", file);
    
    // FILE *dst = fopen(dest, "w");
    // free(dest);
    // // FILE *dst = fopen("teste_dst.txt", "w");
    // if(!dst){
    //     printf("could not open destine file, terminate\n");
    //     return false; 
    // }

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

int response_LS(u_char *resposta, u_char *parametro)
{
    // RECEBEU MOSTRA_TELA
    // RECEBE_SEQUENCIAL DADOS
    // LE TEMP FILE E MOSTRA RESULTADO NA TELA
    // REMOVE TEMP FILE
    return false;
}

int response_PUT(u_char *parametro)
{
    unsigned char *resposta;
    unsigned char *mem, flag;

    FILE *put_file = fopen((char*)parametro, "r");

    // ERRO AO LER ARQUIVO, retorna //
    if(!put_file){        
        switch (errno){             // errno da 11, que nao eh erro esperado
            case 2:                 // ret devolve ($?)*256 de mkdir em system(mkdir)
                flag = arq_nn_E;
                break;
            case 13*256:            // nunca acontece (erro de permissao)
                flag = sem_permissao;
                break;
            default:
                flag = '?';
                break;
        };
        printf("erro %d foi : %s ; flag (%c)\n",errno, strerror(errno), flag);

        if(!envia_msg(soquete, &client_seq, ERRO, &flag, 1))
            printf("put nao enviou erro para servidor\n");

        return false;         
        // fim da funcao get, se ERRO
    }

    // ARQUIVO ABERTO //
    stat((char*)put_file, &st);                      // devolve atributos do arquivo
    mem = calloc(16, sizeof(char));             // 16 digitos c/ bytes cabe ate 999Tb
    sprintf((char*)mem, "%ld", st.st_size);     // salva tamanho do arquivo em bytes

    if(!envia_msg(soquete, &client_seq, DESC_ARQ, mem, 16))
        printf("NAO FOI POSSVIEL ENVIAR DESC_ARQ PARA SERVIDOR\n");

    resposta = recebe_msg(soquete);
    if(!resposta){
        printf("NAO RECEBEU RESPOSTA (OK;ERRO;NACK) DO SERVIDOR\n");
        return false;
    }
 
    next(&nxts_serve);
    read_packet(resposta);
    // define comportamento com base na resposta do servidor
    switch (get_packet_type(resposta))
    {
    case ERRO:                  // arquivo nao cabe
        return false;                 // termina funcao get
    
    case OK:                    // envia arquivo
        if(envia_sequencial(soquete, put_file, &client_seq, &nxts_serve))
            printf("arquivo transferido com sucesso\n");
        else
            printf("nao foi possivel tranferiri arquivo\n");
    }

    return true;
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
    if(!resposta)   
        return false;

    data = get_packet_data(resposta);
    switch (get_packet_type(resposta))
    {
        case OK:            // resposta de (cd, mkdir, put)
            printf("cliente_sinaliza resposta: (%s) ; mensagem: (%s)\n", get_type_packet(resposta), data);
            break;

        case DESC_ARQ:      // resposta do get
            read_packet(resposta);
            response_GET(resposta, parametro);  // parametro eh file
            break;

        case MOSTRA_TELA:  // resposta do ls
            response_LS(resposta, parametro);
            break;

        case ERRO:          // resposta de (ls, cd, mkdir, put, get)
            printf("cliente_sinaliza resposta: (%s) ; mensagem: (%s)\n", get_type_packet(resposta), data);
            free(resposta);
            free(data);
            return false;

        default:            // fedeu
            printf("cliente_sinaliza recebeu tipo nao definido\n");
            read_packet(resposta);
            break;
    }
    free(resposta);
    free(data);

    return true;
}

// atualiza e retorna proxima sequencia
unsigned int sequencia(void)
{
    int now = client_seq;
    client_seq = (client_seq+1)%MAX_SEQUENCE;
    return now;
}

