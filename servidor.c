#include "servidor.h"
#include "Packet.h"
#include "ConexaoRawSocket.h"
#include <sys/stat.h>

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

void print(char *buf){
    for(int i = 0; i < 63; i++)
        printf("%d,", buf[i]);
    printf("fim\n");
}




/* sniff sniff */
int main()
{
    // int bytes;
    // int check;
    // char*resposta, buffer[TAM_PACOTE];       // mensagem de tamanho constante
    // charbuffer[TAM_PACOTE];
     char* pacote;
    // soquete = ConexaoRawSocket("lo");
    // soquete = ConexaoRawSocket("enp2s0f1"); // abre o socket -> lo vira ifconfig to pc que recebe
    // soquete = ConexaoRawSocket("eno1");
    soquete = ConexaoRawSocket("enp3s0");

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    setsockopt(soquete, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

/* funciona 
    // recv
    char buf[63];
    memset(buf, 0, 63);
    recv(soquete, buf, 63, 0);
    printf("tam buf: %ld\n", strlen(buf));
    print(buf);
    // recv
    print(buf);
    memset(buf, 0, 63);
    recv(soquete, buf, 63, 0);
    printf("tam buf: %ld\n", strlen(buf));
    print(buf);
*/




/* da problema
*/
    // recv
    // char *dado = recebe_msg(soquete);
    char dado[TAM_PACOTE];
    memset(dado, 255, TAM_PACOTE);
    if(recv(soquete, dado, TAM_PACOTE, 0) > 0)
        print_bytes("recv recebeu", dado+TAM_HEADER, TAM_PACOTE);
    // free(dado);
    // sleep(1);
    printf("\n\n");
    // recv
    // char *dado2 = recebe_msg(soquete);
    char dado2[TAM_PACOTE];
    memset(dado2, 255, TAM_PACOTE);
    if(recv(soquete, dado, TAM_PACOTE, 0) > 0)
        print_bytes("recv recebeu", dado2+TAM_HEADER, TAM_PACOTE);
    // free(dado2);


    while(0){
        pacote = recebe(soquete, &serv_seq, &nxts_cli);
        if(!pacote) 
            continue;
/*
        if(get_packet_type(pacote) != CD){
            while(recv(soquete, buffer, TAM_PACOTE, MSG_PEEK) == 67){
                recv(soquete, buffer, TAM_PACOTE, 0);
                nxts_cli++;
                printf("DISCARTED:\n");
                read_packet(buffer);
            }
            send(soquete, make_packet(sequencia(), NACK, "3", 1), TAM_PACOTE, 0);
            continue;
        }
*/
        read_packet(pacote);
        server_switch(pacote);
        free(pacote);
    }
    
    close(soquete);
    printf("servidor terminado\n");
}

void server_switch( char* buffer)
{
    int tipo_lido = get_packet_type(buffer);

	switch (tipo_lido)
	{
        case OK:
            // funcao q redireciona
            break;
        case NACK:
            // funcao q redireciona
            break;
        case ACK:
            // funcao q redireciona
            break;
        case CD:
            cdc(buffer);
            break;
        case LS:
            // funcao q redireciona
            break;
        case MKDIR:
            mkdirc(buffer);
            break;
        case GET:
            get(buffer);
            break;
        case PUT:
            // funcao q redireciona
            break;
        case ERRO:
            // funcao q redireciona
            break;
        case DESC_ARQ:
            // funcao q redireciona
            break;
        case DADOS:
            // funcao q redireciona
            break;
        case FIM:
            // funcao q redireciona
            break;
        case SHOW_NA_TELA:
            // funcao q redireciona
            break;

		default:
			break;
	}
}


void cdc( char* buffer){
    int resultado, bytes, ret;
     char *resposta;
    char *cd, flag[1];
    cd = (char *) get_packet_data(buffer);
    char *d = calloc(strcspn(cd, " "), sizeof(char));    // remove espaco no final da mensagem, se tem espaco da ruim 
    char pwd[PATH_MAX];                                 // "cd ..     " -> "..    " nn existe 
    strncpy(d, cd, strcspn(cd, " "));

    if(getcwd(pwd, sizeof(pwd)))printf("before: %s\n", pwd);
    ret = chdir(d);
    if(getcwd(pwd, sizeof(pwd)))printf("after: %s\n", pwd);

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
        printf("erro %d foi : %s ; flag (%c)\n",errno, strerror(errno), flag[0]);
    }
    else{
        resultado = OK;
    }
    resposta = make_packet(sequencia(), resultado, flag, resultado == ERRO);
    if(!resposta) return;   // se pacote deu errado

    bytes = send(soquete, resposta, TAM_PACOTE, 0);                 // envia packet para o socket
    if(bytes<0)                                                     // pega erros, se algum
        printf("error: %s\n", strerror(errno));                     // print detalhes do erro

    free(resposta);
    free(cd);
    free(d);
}

// trata_tipos: case switch p/ cada ENUM uma funcao
// Tipo == ls -> funcao_ls(dados){ dados = a ou l ... } .... 

void mkdirc( char* buffer){
    int bytes, ret, resultado;
     char *resposta;
    char *mkdir, flag[1];

    mkdir = get_packet_data(buffer);
    ret = system(mkdir);

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
    }

    resposta = make_packet(sequencia(), resultado, flag, resultado == ERRO);    // mostra flag somente se tem erro, bytes_dados = 1
    if(!resposta) return;   // se pacote deu errado

    bytes = send(soquete, resposta, TAM_PACOTE, 0);     // envia packet para o socket
    if(bytes<0)                                         // pega erros, se algum
        printf("error: %s\n", strerror(errno));         // print detalhes do erro

    free_packet(resposta);
    free(mkdir);
}

void get( char *buffer){
    // int bytes, resultado;
    int bytes;
    // char*resposta_srv, *resposta_cli;
     char *resposta_srv, resposta_cli[TAM_PACOTE];
    char *get, *mem, flag;

    get = get_packet_data(buffer);  // arquivo a abrir
    // nxts_cli
    FILE *arquivo;

    arquivo = fopen(get, "r");

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

        resposta_srv = make_packet(sequencia(), ERRO, &flag, 1);
        if(!resposta_srv){  // se pacote deu errado
            printf("falha ao criar pacote de resposta do get (servidor), terminando\n");
            return;   
        } 

        bytes = send(soquete, resposta_srv, TAM_PACOTE, 0);     // envia packet para o socket
        if(bytes<0)                                             // pega erros, se algum
            printf("falha ao enviar pacote de resposta do get (servidor), erro: %s\n", strerror(errno));         // print detalhes do erro
        free(resposta_srv);
        return;         
        // fim da funcao get, se ERRO
    }

    // ARQUIVO ABERTO //
    stat(get, &st);                     // devolve atributos do arquivo
    mem = calloc(16, sizeof(char));     // 16 digitos c/ bytes cabe ate 999Tb
    sprintf(mem, "%ld", st.st_size);    // salva tamanho do arquivo em bytes

    resposta_srv = make_packet(sequencia(), DESC_ARQ, mem, 16); // string de 16 digitos em bytes
    free(mem);
    if(!resposta_srv){  // se pacote deu errado
        printf("falha ao criar pacote de resposta do get, terminando\n");
        return;   
    }  
    
    send(soquete, resposta_srv, TAM_PACOTE, 0);
    bytes = recv(soquete, resposta_cli, TAM_PACOTE, 0);
    if(bytes<0){
        printf("nao recebeu pacote de resposta do cliente (OK;ERRO;NACK), terminando\n");
        return;
    }
    read_packet(resposta_cli);
    nxts_cli = (nxts_cli+1)%MAX_SEQUENCE;

    // define comportamento com base na resposta do cliente
    switch (get_packet_type(resposta_cli))
    {
    case ERRO:                  // arquivo nao cabe
        // free(resposta_cli);     
        free(resposta_srv);
        return;                 // termina funcao get
    
    case OK:                    // envia arquivo
        envia_sequencial(soquete, arquivo, &serv_seq, &nxts_cli);
        return;
    }

    return;
}

// atualiza e retorna proxima sequencia
unsigned int sequencia(void)
{
    int now = serv_seq;
    serv_seq = (serv_seq+1)%MAX_SEQUENCE;
    return now;
}