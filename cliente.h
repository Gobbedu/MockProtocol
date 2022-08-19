#ifndef CLIENTE_H
#define CLIENTE_H

#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

int cliente_sinaliza(char *comando, int tipo);
int response_GET(unsigned char* resposta, char *parametro);
void client_switch(char* comando);
// void get(char *comando, int tipo);

unsigned int sequencia(void);

#endif