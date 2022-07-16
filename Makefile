
.PHONY: all clean		# sempre vai estar desatualizado (make roda dnv)

all: cliente servidor

cliente: cliente.o ConexaoRawSocket.o Packet.o
	cc -o cliente cliente.o ConexaoRawSocket.o Packet.o

servidor: servidor.o ConexaoRawSocket.o Packet.o
	cc -o servidor servidor.o ConexaoRawSocket.o Packet.o

# =======================tutorial=======================

tut: recv-tut send-tut

recv-tut:
	@gcc packet_sniff_raw_subodh.c -o sniff-tut

send-tut:
	@gcc send_packet_raw_subodh.c -o dest-tut

clean:
	@rm -f ConexaoRawSocket.o Packet.o servidor.o cliente.o
	@rm -f dest-tut sniff-tut 

purge: clean
	@rm -f cliente servidor