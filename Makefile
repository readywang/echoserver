all:
	g++ echocli1.cpp -g -o echocli1.o
	g++ echosrv1.cpp -g -o echosrv1.o

clean:
	rm -f echocli1.o echosrv1.o
