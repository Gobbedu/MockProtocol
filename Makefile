CC = @gcc 								# compilador que queremos (@ esconde comando)
CFLAGS = -Wall -g -O0					# flags para compilar .c

objs = 	\
		ConexaoRawSocket.o \
		Packet.o \
		PacketComms.o

all: servidor cliente

# condicoes implicitas compilam com CC e CFLAGS
cliente:  $(objs) cliente.o 
servidor: $(objs) servidor.o

clean:
	@rm -f $(objs) servidor.o cliente.o

purge: clean
	@rm -f servidor cliente

run: clean purge all