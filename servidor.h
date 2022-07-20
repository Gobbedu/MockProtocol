#ifndef SERVIDOR_H
#define SERVIDOR_H

#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>


void type_process_server(unsigned char* buffer,int buflen);

#endif /* __SERVIDOR__*/ 
