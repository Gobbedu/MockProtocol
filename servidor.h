#ifndef SERVIDOR_H
#define SERVIDOR_H

#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>


void server_switch(unsigned char* buffer);
void cdc(unsigned char* buffer);
void mkdirc(unsigned char* buffer);

#endif /* __SERVIDOR__*/ 
