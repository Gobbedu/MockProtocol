#ifndef CLIENTE_H
#define CLIENTE_H

#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

void type_process_client(char* comando, char* parametro);

void gera_pedido(char * dados, int tipo);

void get();

#endif