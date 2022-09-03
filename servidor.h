#ifndef SERVIDOR_H
#define SERVIDOR_H

#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>

void server_switch(unsigned char * buffer);
void cdc(unsigned char * buffer);
void mkdirc(unsigned char * buffer);
void get(unsigned char*buffer);
int put(unsigned char *buffer);
int ls(u_char *buffer);



unsigned int sequencia(void);

#endif /* __SERVIDOR__*/ 
