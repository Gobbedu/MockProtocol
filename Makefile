
teste: ConexaoRawSocket.o
	@gcc ConexaoRawSocket.o teste.c -o teste

ConexaoRawSocket.o:
	@gcc -c ConexaoRawSocket.c

clean:
	@rm -f teste
	@rm -f ConexaoRawSocket.o