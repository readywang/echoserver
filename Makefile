all:
	g++ echocli.cpp -g -o echocli.o
	g++ echosrv.cpp -g -o echosrv.o
	g++ echocli1.cpp -g -o echocli1.o
	g++ echosrv1.cpp -g -o echosrv1.o
	g++ echocli2.cpp -g -o echocli2.o
	g++ echosrv2.cpp -g -o echosrv2.o
	g++ echocli3.cpp -g -o echocli3.o
	g++ echosrv4.cpp -g -o echosrv3.o
clean:
	rm -f echocli.o echosrv.o echocli1.o echosrv1.o echocli2.o echosrv2.o echocli3.o echosrv3.o
