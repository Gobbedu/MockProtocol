#ifndef SERVIDOR_H
#define SERVIDOR_H

#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>

struct stat st;

void server_switch(char * buffer);
void cdc(char * buffer);
void mkdirc(char * buffer);
void get(char*buffer);

unsigned int sequencia(void);

#endif /* __SERVIDOR__*/ 
