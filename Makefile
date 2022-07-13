
all: cliente servidor

cliente: ConexaoRawSocket.o
	@gcc ConexaoRawSocket.o cliente.c -o cliente

servidor: ConexaoRawSocket.o
	@gcc ConexaoRawSocket.o servidor.c -o servidor

ConexaoRawSocket.o: ConexaoRawSocket.c
	@gcc -c ConexaoRawSocket.c


tut: recv-tut send-tut

recv-tut:
	@gcc packet_sniff_raw_subodh.c -o sniff-tut

send-tut:
	@gcc send_packet_raw_subodh.c -o dest-tut


clean:
	@rm -f cliente servidor ConexaoRawSocket.o
	@rm -f dest-tut sniff-tut 
