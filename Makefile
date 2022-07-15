
.PHONY: cliente servidor clean		# sempre vai estar desatualizado (make roda dnv)

all: cliente servidor

cliente: ConexaoRawSocket.o Packet.o
	@gcc ConexaoRawSocket.o Packet.o cliente.c -o cliente

servidor: ConexaoRawSocket.o Packet.o
	@gcc  ConexaoRawSocket.o Packet.o servidor.c -o servidor

ConexaoRawSocket.o: ConexaoRawSocket.c
	@gcc -c ConexaoRawSocket.c

Packet.o: Packet.c
	@gcc -c Packet.c

# =======================tutorial=======================

tut: recv-tut send-tut

recv-tut:
	@gcc packet_sniff_raw_subodh.c -o sniff-tut

send-tut:
	@gcc send_packet_raw_subodh.c -o dest-tut


clean:
	@rm -f cliente servidor ConexaoRawSocket.o Packet.o
	@rm -f dest-tut sniff-tut 
