all:	libpp.a ppexec

ppexec:	ppexec.cpp 
	g++ -std=c++11 -o ppexec ppexec.cpp -lpp -L.
	cc -o P5TEST1 P5TEST1.c -lpp -L.
	cc -o P5TEST2 P5TEST2.c -lpp -L.
	cc -o P5TEST3 P5TEST3.c -lpp -L.

libpp.a:	mpi.c
	cc -c -o mpi.o mpi.c
	ar rvf libpp.a mpi.o

clean:
	rm -f ppexec libpp.a *.o P5TEST?
