#ifndef CLIENTE_H
#define CLIENTE_H

#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

void client_switch(char* comando);

void gera_pedido(char * dados, int tipo);
void cds(char *comando);
void get();

#endif