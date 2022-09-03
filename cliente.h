#ifndef CLIENTE_H
#define CLIENTE_H

#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

int cliente_sinaliza(unsigned char *comando, int tipo);
void client_switch(char* comando);

int response_GET(unsigned char * resposta, unsigned char *parametro);
int response_PUT(u_char *parametro);
int response_LS(u_char *resposta, u_char *parametro);


unsigned int sequencia(void);

#endif