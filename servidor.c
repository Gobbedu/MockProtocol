#include "servidor.h"
#include "Packet.h"
#include "ConexaoRawSocket.h"

// - [] CHECAR FUNCAO check_packet sem loopback, MUDA CALCULO DA PARIDADE int paradis = 1;
// - [] (TODO) VERIFICAR SE SEQUENCIA CORRETA, ENVIA DE VOLTA SEQUENCIA NACK 
// - [] tratar da sequencia de mkdirc e cdc, caso algo de errado, senao sequencia nunca emparelha dnv
// - [] tratar next_seq do cliente e do servidor, update quando aceita
// - [] sequencia do GET ta quebrada
// - [] responder NACK se erro na paridade da msg recebida do cliente
// - [] get: se nack responde e espera outra msg


// se respode NACK, e mensagem nao chega ao destino, sequencia quebra (conforme esperado)


// global para servidor
int soquete;
unsigned int serv_seq = 10;   // current server sequence
unsigned int nxts_cli = 0;   // expected next client sequence

/* sniff sniff */
int main()
{
    unsigned char *pacote;
    // soquete = ConexaoRawSocket("lo");
    // soquete = ConexaoRawSocket("enp2s0f1"); // abre o socket -> lo vira ifconfig to pc que recebe
    // soquete = ConexaoRawSocket("enp1s0f1"); // abre o socket -> lo vira ifconfig to pc que recebe
    // soquete = ConexaoRawSocket("eno1");
    soquete = ConexaoRawSocket("enp3s0");

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    setsockopt(soquete, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

    while(1){
        // pacote = recebe(soquete, &serv_seq, &nxts_cli);
        pacote = recebe_msg(soquete);
        if(!pacote) 
            continue;
        if(!check_sequence(pacote, nxts_cli)){
            printf("servidor esperava SEQUENCIA (%d) mas recebeu (%d)\n",
            nxts_cli, get_packet_sequence(pacote));
            free(pacote);
            continue;
        }
        if(!check_parity(pacote)){
            printf("servidor recebeu mensagem com ERRO NA PARIDADE\n");
            free(pacote);
            continue;
        }
        next(&nxts_cli);

        server_switch(pacote);
        free(pacote);
    }
    
    close(soquete);
    printf("servidor terminado\n");
}

void server_switch(unsigned char* buffer)
{
    int tipo_lido = get_packet_type(buffer);
    system("echo -n \"\nServer:\t \" && pwd");

	switch (tipo_lido)
	{
        case CD:
            cdc(buffer);
            break;
        case LS:
            ls(buffer);
            break;
        case MKDIR:
            mkdirc(buffer);
            break;
        case GET:
            get(buffer);
            break;
        case PUT:
            put(buffer);
            break;
		default:
            printf("server_switch recebeu tipo nao definido\n");
            read_packet(buffer);
			break;
	}
}


void cdc(unsigned char* buffer){
    int resultado, ret;
    // unsigned char *resposta;
    unsigned char *cd, flag[1];

    printf("CD ");
    cd = get_packet_data(buffer);
    unsigned char *d = calloc(strcspn((char*)cd, " "), sizeof(unsigned char));    // remove espaco no final da mensagem, se tem espaco da ruim 
    char pwd[PATH_MAX];                                 // "cd ..     " -> "..    " nn existe 
    strncpy((char*)d, (char*)cd, strcspn((char*)cd, " "));

    // if(getcwd(pwd, sizeof(pwd)))printf("before: %s\n", pwd);
    ret = chdir((char*)d);

    if(ret == -1){
        resultado = ERRO;
        switch (errno){
            case 2:
                flag[0] = dir_nn_E;
                break;
            case 13:
                flag[0] = sem_permissao;
                break;
            default:
                flag[0] = '?';
                break;
        };
        printf("erro %d foi : %s (%s) ; flag (%c)\n",errno, strerror(errno), d, flag[0]);
    }
    else{
        resultado = OK;
        if(getcwd(pwd, sizeof(pwd)))    
            printf("New:\t %s\n", pwd);
    }


    // respota = recebe_msg(soquete);
    if(!envia_msg(soquete, &serv_seq, resultado, flag, resultado == ERRO))
        return;     // nao foi possivel enviar o pacote

    free(cd);
    free(d);
}

void mkdirc(unsigned char* buffer){
    int ret, resultado;
    // unsigned char *resposta;
    unsigned char *mkdir, flag[1];

    printf("MKDIR "); fflush(stdout);
    mkdir = get_packet_data(buffer);
    ret = system((char*)mkdir);

    if(ret != 0){        
        resultado = ERRO;
        switch (ret){       // errno da 11, que nao eh erro esperado
            case 256:       // ret devolve ($?)*256 de mkdir em system(mkdir)
                flag[0] = dir_ja_E;
                break;
            case 13*256:    // nunca acontece 
                flag[0] = sem_permissao;
                break;
            default:
                flag[0] = '?';
                break;
        };
        printf("erro %d foi : %s ; flag (%c)\n",errno, strerror(errno), flag[0]);
    }
    else{
        resultado = OK;
        printf("'%s' criado com sucesso!\n", mkdir+7);
    }
    if(!envia_msg(soquete, &serv_seq, resultado, flag, resultado == ERRO))
        return;     // nao foi possivel enviar o pacote

    free(mkdir);
}

void get(unsigned char *buffer){
    unsigned char *resposta;
    unsigned char *get, *mem, flag;

    printf("GET ");
    get = get_packet_data(buffer);  // arquivo a abrir
    // nxts_cli
    FILE *arquivo;

    arquivo = fopen((char*)get, "r");

    // ERRO AO LER ARQUIVO, retorna //
    if(!arquivo){        
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

        if(!envia_msg(soquete, &serv_seq, ERRO, &flag, 1))
            printf("get nao respondeu erro para cliente\n");

        return;         
        // fim da funcao get, se ERRO
    }

    // ARQUIVO ABERTO //
    stat((char*)get, &st);                     // devolve atributos do arquivo
    mem = calloc(16, sizeof(char));     // 16 digitos c/ bytes cabe ate 999Tb
    sprintf((char*)mem, "%ld", st.st_size);    // salva tamanho do arquivo em bytes
    long tamanho_bytes = atoi((char*)mem);

    if(!envia_msg(soquete, &serv_seq, DESC_ARQ, mem, 16))
        printf("NAO FOI POSSVIEL ENVIAR DESC_ARQ PARA CLIENTE\n");
    free(mem);

    resposta = recebe_msg(soquete);
    if(!resposta){
        printf("NAO RECEBEU RESPOSTA (OK;ERRO;NACK) DO CLIENTE\n");
        return;
    }
 
    next(&nxts_cli);
    read_packet(resposta);
    // define comportamento com base na resposta do cliente
    switch (get_packet_type(resposta))
    {
    case ERRO:                  // arquivo nao cabe
        return;                 // termina funcao get
    
    case OK:                    // envia arquivo
        if(envia_sequencial(soquete, arquivo, &serv_seq, &nxts_cli, tamanho_bytes))
            printf("arquivo transferido com sucesso\n");
        else
            printf("nao foi possivel tranferiri arquivo\n");
        return;
    }

    return;
}

int put(unsigned char *buffer)
{
    // TENTA CRIAR ARQUIVO E RESPONDE OK;NACK;ERRO
    // RECEBE DESC_ARQ DE OK E RESPODE OK;NACK;ERRO
    // SE OK CHAMA RECEBE SEQUENCIAL
    unsigned char *resposta;
    unsigned char flag;

    printf("PUT ");
    u_char *put = get_packet_data(buffer);  // arquivo a abrir
    // nxts_cli
    FILE *arquivo;

    char *dest = calloc(6+strlen((char*)put), sizeof(char));
    sprintf(dest, "(copy)%s", put);
    arquivo = fopen((char*)dest, "w");
    free(dest);
    
    // ERRO AO LER ARQUIVO, retorna //
    // PESQUISAR ERROS DO FOPEN PARA ESCRITA
    if(!arquivo){        
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

        if(!envia_msg(soquete, &serv_seq, ERRO, &flag, 1))
            printf("get nao respondeu erro para cliente\n");

        return false;         
        // fim da funcao get, se ERRO
    }
    // ARQUIVO ABERTO //

    // CALCULA MEMORIA DISPONIVEL // (em bytes)
    long int mem_livre;
    char *bashdf = "df --output=avail --block-size=1 / | tail -n +2 > /tmp/tamanho_gb.txt";
    char *bashrm = "rm /tmp/tamanho_gb.txt";
    system(bashdf);
    FILE *memfree = fopen("/tmp/tamanho_gb.txt", "r");
    fscanf(memfree, "%ld", &mem_livre);
    fclose(memfree);
    system(bashrm);

    if(!envia_msg(soquete, &serv_seq, OK, NULL, 0))
        printf("NAO FOI POSSVIEL ENVIAR OK PARA CLIENTE\n");

    resposta = recebe_msg(soquete);
    if(!resposta){
        printf("NAO RECEBEU RESPOSTA (OK;ERRO;NACK) DO CLIENTE\n");
        return false;
    }

    // IF NACK (TRATAR)
 
    next(&nxts_cli);
    // read_packet(resposta);
    long tamanho = atoi((char*)get_packet_data(resposta));
    // ARQUIVO NAO CABE, retorna //
    if(mem_livre < tamanho){
        printf("Espaço insuficiente!\n");
        if(!envia_msg(soquete, &serv_seq, ERRO, NULL, 0))
            printf("Não foi possivel responder o erro!\n");
        return false;
    }

    // ARQUIVO CABE //
    if(!envia_msg(soquete, &serv_seq, OK, NULL, 0)){
        printf("nao foi possivel responder ok para o servidor\n");
        return false;   
    }

    // CASO resultado == OK, FAZER A LOGICA DAS JANELAS DESLIZANTES AQUI
    if(recebe_sequencial(soquete, (u_char*)put, &serv_seq, &nxts_cli, tamanho)){
        printf("\narquivo (%s) transferido com sucesso!\n", put);
        return true;
    }


    // return;
    return false;
}

int ls(u_char *buffer){
    unsigned char *resposta;
    unsigned char *mem, flag;

    printf("LS ");

    // RECEBE COMANDO LS EM DADOS DO PACOTE
    u_char * buffer_aux = calloc(6 + strlen(" > /tmp/ls.txt"), sizeof(char));
    buffer_aux = get_packet_data(buffer);
    strcat((char *)buffer_aux, " > /tmp/ls.txt");
    // EXECUTA COMANDO NO SERVER E SALVA EM TMP FILE
    system((char*)buffer_aux);
    FILE *arquivo;

    arquivo = fopen("/tmp/ls.txt", "r");

    // RESPONDE NACK;ERRO;MOSTRA_TELA
    // ERRO AO LER ARQUIVO, retorna //
    if(!arquivo){        
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

        if(!envia_msg(soquete, &serv_seq, ERRO, &flag, 1))
            printf("ls nao respondeu erro para cliente\n");

        return 0;         
        // fim da funcao ls, se ERRO
    }

    // ARQUIVO ABERTO //
    stat((char*)"/tmp/ls.txt", &st);                     // devolve atributos do arquivo
    mem = calloc(16, sizeof(char));     // 16 digitos c/ bytes cabe ate 999Tb
    sprintf((char*)mem, "%ld", st.st_size);    // salva tamanho do arquivo em bytes
    long tamanho_bytes = atoi((char*)mem);
    free(mem);

    if(!envia_msg(soquete, &serv_seq, MOSTRA_TELA, (unsigned char *)"ls", 2))
        printf("NAO FOI POSSVIEL ENVIAR MOSTRA_TELA PARA CLIENTE\n");

    resposta = recebe_msg(soquete);
    if(!resposta){
        printf("NAO RECEBEU RESPOSTA (OK;ERRO;NACK) DO CLIENTE\n");
        return 0;
    }

    // ENVIA_SEQUENCIAL TMP FILE
    next(&nxts_cli);
    // read_packet(resposta);
    // define comportamento com base na resposta do cliente
    switch (get_packet_type(resposta))
    {
    case ERRO:                  // arquivo nao cabe
        return 0;               // termina funcao ls
    
    case OK:                    // envia arquivo
        if(envia_sequencial(soquete, arquivo, &serv_seq, &nxts_cli, tamanho_bytes))
            printf("arquivo transferido com sucesso\n");
        else
            printf("nao foi possivel tranferiri arquivo\n");
        return 0;
    }

    // SUCESSO REMOVE TMP FILE
    system("rm /tmp/ls.txt");
    free(buffer_aux);

    return 1;
}
