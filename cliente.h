#ifndef CLIENTE_H
#define CLIENTE_H

#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

void client_switch(char* comando);
void cds(char *comando);
void get(char *comando);
void mkdirs(char *comando);
int cliente_sinaliza(char *comando, int tipo);

#endif