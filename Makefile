CC = @gcc 								# compilador que queremos (@ esconde comando)
CFLAGS = -Wall -g						# flags para compilar .c

objs = 	ConexaoRawSocket.o \
		Packet.o \
		servidor.o \
		cliente.o 

all: servidor cliente

# condicoes implicitas compilam com CC e CFLAGS
cliente:  ConexaoRawSocket.o Packet.o cliente.o
servidor: ConexaoRawSocket.o Packet.o servidor.o

clean:
	@rm -f $(objs)

purge: clean
	@rm -f servidor cliente
	@rm -f recv-tut send-tut

# # =======================tutorial=======================

tut: recv-tut send-tut

recv-tut:
	@gcc packet_sniff_raw_subodh.c -o recv-tut

send-tut:
	@gcc send_packet_raw_subodh.c -o send-tut
