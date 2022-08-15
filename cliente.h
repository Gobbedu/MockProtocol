#ifndef CLIENTE_H
#define CLIENTE_H

#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

int cliente_sinaliza(char *comando, int tipo);
void client_switch(char* comando);
void get(char *comando, int tipo);

#endif