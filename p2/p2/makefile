all: libpp.a ppexec

ppexec: ppexec.cpp
	g++ -std=c++11 -o ppexec ppexec.cpp -lpp -L.
	gcc -o t1 t1.c -lpp -L.
	gcc -o t2 t2.c -lpp -L.
	gcc -o t3 t3.c -lpp -L.
	gcc -o t4 t4.c -lpp -L.
	gcc -o t5 t5.c -lpp -L.
	gcc -o t6 t6.c -lpp -L. 
	
libpp.a: mpi.c 
	gcc -c -o mpi.o mpi.c
	ar rvf libpp.a mpi.o 

clean:
	rm -f ppexec libpp.a t? *.o
