// some things dont work quite the same with mpich2 (RMA)
//     see notes below
// but for some testing with mpich2, you can do:
//     mpicxx -o tester tester.c

#include "mpi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#define BALL_VEC_SZ 100000

int Wtime();
int ProbeEtc();
int NonBlock();
int Gather();
int DupOnly();
int ANY();
int Bcast();
int Ball();
int LargeMessage();
int Barrier();
int Callhome();
int Multi();
int Locks();
int RMA();

int callhome_version = 0;

int main(int argc, char** argv)
{
    setbuf(stdout,NULL);

    if (argc < 2)
    {
        printf("USAGE: %s test_to_run\n");
        exit(-1);
    }
    if (strcmp(argv[1],"wtime") == 0)
        Wtime();  // nranks 8+
    else if (strcmp(argv[1],"probeetc") == 0)
        ProbeEtc();      // nranks 2
    else if (strcmp(argv[1],"nonblock") == 0)
        NonBlock();      // nranks 2
    else if (strcmp(argv[1],"gather") == 0)
        Gather();        // nranks 2, 8
    else if (strcmp(argv[1],"duponly") == 0)
        DupOnly();    // nranks 2, 8  // hangs; requires exactly 2 ranks
    else if (strcmp(argv[1],"any") == 0)
        ANY();           // nranks 2, 8
    else if (strcmp(argv[1],"bcast") == 0)
        Bcast();         // nranks 2, 8
    else if (strcmp(argv[1],"ball") == 0)
        Ball();          // nranks 2, 8
    else if (strcmp(argv[1],"largemessage") == 0)
        LargeMessage();  // nranks 2, 8
    else if (strcmp(argv[1],"barrier") == 0)
        Barrier();       // nranks 2, 8
    else if (strcmp(argv[1],"callhome1") == 0)
    {
        callhome_version = 1;
        Callhome();      // nranks 2, 8
    }
    else if (strcmp(argv[1],"callhome2") == 0)
    {
        callhome_version = 2;
        Callhome();      // nranks 2, 8
    }
    else if (strcmp(argv[1],"multi") == 0)
        Multi();         // nranks 2, 8
    else if (strcmp(argv[1],"locks") == 0)
        Locks();         // nranks 2, 8
    else if (strcmp(argv[1],"rma") == 0)
        RMA();           // nranks 2, 8  // for more than 2, see comments on next lines
        // for RMA above:
        //     we use strict locking and can run with more than 2
        //     but should only use 2 if testing with mpich2
    else
    {
        printf("INVALID ARG TO PGM\n");
        exit(-1);
    }
}

int ProbeEtc()  //probe
{
    MPI_Init(NULL, NULL);
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int home = 0;
    
    if (rank == 0)
    {
        int out;
        out = 5556;
        MPI_Send(&out, 1 , MPI_INT, 1, 60, MPI_COMM_WORLD);
    } else if (rank == 1)
    {
        sleep(1);
        MPI_Status st;
        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &st);
        fprintf(stderr, "st source %d\n", st.MPI_SOURCE);
        int buf;
        MPI_Recv(&buf, 1 , MPI_INT, st.MPI_SOURCE, st.MPI_TAG, MPI_COMM_WORLD, &st);
        fprintf(stderr, "it was %d\n", buf);
    }

    MPI_Finalize();

    printf("SUCCESS\n");
    return 0;
}

int NonBlock() //nonblocking send/recv
{
    MPI_Init(NULL, NULL);
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int home = 0;
    MPI_Status status;

    printf("Rank: %d\tSize:%d\n", rank, size);

    if (rank == 0)
    {
        int flag;

        MPI_Request req1, req2, req3;
        MPI_Status st1, st2, st3;

        int buf1 = 1111;
        int buf2 = 2222;
        int buf3 = 3333;

        MPI_Isend(&buf1, 1, MPI_INT, 1, 50, MPI_COMM_WORLD, &req1);
        MPI_Isend(&buf2, 1, MPI_INT, 1, 60, MPI_COMM_WORLD, &req2);
        MPI_Isend(&buf3, 1, MPI_INT, 1, 50, MPI_COMM_WORLD, &req3);

        do{
            MPI_Test(&req1, &flag, &st1);
        }
        while(!flag);

        //MPI_Wait(&req1, &st1);
        MPI_Wait(&req2, &st2);
        MPI_Wait(&req3, &st3);

        fprintf(stderr,"I'm done with the first stage\n");
    }
    if (rank == 1)
    {
        int flag;

        MPI_Request req1, req2, req3;
        MPI_Status st1, st2, st3;

        int buf1, buf2, buf3;

        MPI_Irecv(&buf2, 1, MPI_INT, 0, 60, MPI_COMM_WORLD, &req2); //notice how 60 is out of order
        MPI_Irecv(&buf1, 1, MPI_INT, 0, 50, MPI_COMM_WORLD, &req1);
        MPI_Irecv(&buf3, 1, MPI_INT, 0, 50, MPI_COMM_WORLD, &req3);


        MPI_Wait(&req1, &st1);
        MPI_Wait(&req2, &st2);
        MPI_Wait(&req3, &st3);

        fprintf(stderr,"Stage 1 Recevied: %d %d %d\n", buf1, buf2, buf3);
    }
    sleep(1);

    //stage 2
    if (rank == 0)
    {
        int flag;

        MPI_Request req1, req2, req3;
        MPI_Status st1, st2, st3;

        int buf1 = 1111;
        int buf2 = 2222;
        int buf3 = 3333;

        MPI_Isend(&buf1, 1, MPI_INT, 1, 10, MPI_COMM_WORLD, &req1);
        MPI_Isend(&buf2, 1, MPI_INT, 1, 20, MPI_COMM_WORLD, &req2);
        MPI_Isend(&buf3, 1, MPI_INT, 1, 30, MPI_COMM_WORLD, &req3);

        MPI_Wait(&req1, &st1);
        MPI_Wait(&req2, &st2);
        MPI_Wait(&req3, &st3);

        fprintf(stderr,"I'm done with the second stage\n");
    }
    if (rank == 1)
    {
        int flag;

        MPI_Request req1, req2, req3;
        MPI_Status st1, st2, st3;

        int buf1, buf2, buf3;

        MPI_Irecv(&buf1, 1, MPI_INT, 0, 20, MPI_COMM_WORLD, &req1); //2222
        MPI_Irecv(&buf2, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &req2); // 1111
        MPI_Irecv(&buf3, 1, MPI_INT, 0, 30, MPI_COMM_WORLD, &req3); // 3333

        MPI_Wait(&req1, &st1);
        MPI_Wait(&req2, &st2);
        MPI_Wait(&req3, &st3);

        fprintf(stderr,"Stage 2 Recevied: %d %d %d\n", buf1, buf2, buf3); //2222 1111 3333
    }
    sleep(1);

    //stage 3
    if (rank == 0)
    {
        int flag;

        MPI_Request req1, req2, req3;
        MPI_Status st1, st2, st3;

        int buf1 = 1111;
        int buf2 = 2222;
        int buf3 = 3333;

        MPI_Isend(&buf1, 1, MPI_INT, 1, 10, MPI_COMM_WORLD, &req1);
        MPI_Isend(&buf2, 1, MPI_INT, 1, 20, MPI_COMM_WORLD, &req2);
        MPI_Send(&buf3, 1, MPI_INT, 1, 30, MPI_COMM_WORLD);

        MPI_Wait(&req1, &st1);
        MPI_Wait(&req2, &st2);

        fprintf(stderr,"I'm done with the third stage\n");
    }
    if (rank == 1)
    {
        int flag;

        MPI_Request req1, req2, req3;
        MPI_Status st1, st2, st3;

        int buf1, buf2, buf3;

        MPI_Irecv(&buf1, 1, MPI_INT, 0, 20, MPI_COMM_WORLD, &req1); //2222
        MPI_Irecv(&buf2, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &req2); // 1111
        MPI_Irecv(&buf3, 1, MPI_INT, 0, 30, MPI_COMM_WORLD, &req3); // 3333

        MPI_Wait(&req1, &st1);
        MPI_Wait(&req2, &st2);
        MPI_Wait(&req3, &st3);

        fprintf(stderr,"Stage 3 Recevied: %d %d %d\n", buf1, buf2, buf3); //2222 1111 3333
    }
    sleep(1);

    //stage 4
    if (rank == 0)
    {
        int flag;

        MPI_Request req1, req2, req3;
        MPI_Status st1, st2, st3;

        int buf1 = 1111;
        int buf2 = 2222;
        int buf3 = 3333;

        MPI_Isend(&buf1, 1, MPI_INT, 1, 10, MPI_COMM_WORLD, &req1);
        MPI_Isend(&buf2, 1, MPI_INT, 1, 20, MPI_COMM_WORLD, &req2);
        MPI_Send(&buf3, 1, MPI_INT, 1, 30, MPI_COMM_WORLD);

        MPI_Wait(&req1, &st1);
        MPI_Wait(&req2, &st2);

        fprintf(stderr,"I'm done with the fourth stage\n");
    }
    else
    {
        int flag;

        MPI_Request req1, req2, req3;
        MPI_Status st1, st2, st3;

        int buf1, buf2, buf3;

        MPI_Irecv(&buf1, 1, MPI_INT, 0, 20, MPI_COMM_WORLD, &req1); //2222
        MPI_Irecv(&buf2, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &req2); // 1111
        MPI_Recv(&buf3, 1, MPI_INT, 0, 30, MPI_COMM_WORLD, &st3); // 3333

        MPI_Wait(&req1, &st1);
        MPI_Wait(&req2, &st2);

        fprintf(stderr,"Stage 4 Recevied: %d %d %d\n", buf1, buf2, buf3); //2222 1111 3333
    }
    sleep(1);

    MPI_Finalize();
    printf("SUCCESS\n");
    return 0;
}

int Gather() //gather
{
    int i, iter;
    MPI_Init(NULL, NULL);
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int home = 0;

    printf("Rank: %d\tSize:%d\n", rank, size);

    for (iter = 0; iter < size; iter++)
    {
        home = iter;

        int sbuf = (100 * iter) + rank;

        if (rank == home)
        {
            int rbuf[size * sizeof(int)];
            printf("iter %d\n", iter);

            MPI_Gather(&sbuf, 1, MPI_INT, &rbuf, 1, MPI_INT, home, MPI_COMM_WORLD);

            for (i = 0; i < size; i++)
            {
                    int test = rbuf[i];


                    //printf("**************************From %d got %d   iter %d\n", i, buf,iter);
                    if (test != (100 * iter) + i)
                    {
                            printf("EPIC FAIL EPIC FAIL EPIC FAIL EPIC FAIL EPIC FAIL!!!!!!\n");
                            printf("*****************From %d got %d   iter %d\n", i, test, iter);
                            exit(1);
                    }
            }
        } else {
            MPI_Gather(&sbuf, 1, MPI_INT, NULL, 0, MPI_INT, home, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();
    printf("SUCCESS\n");
    return 0;
}

int DupOnly()
{
    MPI_Init(NULL, NULL);
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int com1;
    int com2;
    MPI_Status st;
    int ball;

    alarm(4);  // DIE after 4 seconds

    if (size > 2)
    {
        printf("** RUN WITH exactly 2 ranks\n");
        exit(-1);
    }
    printf("prints two values by rank 1 and then hangs\n");
    MPI_Comm_dup(MPI_COMM_WORLD, &com2);
    if (rank == 0)
    {
        ball = 1;
        MPI_Send(&ball, 1, MPI_INT, 1, 999, MPI_COMM_WORLD);
        ball = 2;
        MPI_Send(&ball, 1, MPI_INT, 1, 999, com2);
        ball = 3;
        MPI_Send(&ball, 1, MPI_INT, 1, 999, MPI_COMM_WORLD);
    }
    else
    {
        MPI_Recv(&ball, 1, MPI_INT, MPI_ANY_SOURCE, 999, MPI_COMM_WORLD, &st);
        printf("RECVD BY 1:  %d\n",ball);
        MPI_Recv(&ball, 1, MPI_INT, MPI_ANY_SOURCE, 999, com2, &st);
        printf("RECVD BY 1:  %d\n",ball);
        MPI_Recv(&ball, 1, MPI_INT, MPI_ANY_SOURCE, 999, com2, &st);
        printf("RECVD BY 1:  %d\n",ball);
    }
    MPI_Finalize();
}

int Wtime()
{
    MPI_Init(NULL, NULL);
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int com1;
    int com2;
    double stime;

    stime = MPI_Wtime();
    while ((MPI_Wtime() - stime) < 4.567)
        ;
    printf("%d: %f\n",rank,MPI_Wtime()-stime);

    MPI_Finalize();
    return 0;
}

int ANY() //ANY source / ANY tag
{
    int iter, size, rank, ball;

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Status st;

    for (iter = 0; iter < 10; iter++)
    {

        ball = 0;

        if (rank == 0)
        {
            ball++;
            printf("Throwing the ball first Ball value: %d - iter %d\n", ball, iter);
            //usleep(200*1000);
            MPI_Send(&ball, 1, MPI_INT, rank + 1, 34, MPI_COMM_WORLD);

        } else {
            MPI_Recv(&ball, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &st);
            //printf("Got the ball! New value: %d\n", ++ball);
            if (rank != size - 1)
            {
                    //usleep(200*1000);
                    MPI_Send(&ball, 1, MPI_INT, rank + 1, 34, MPI_COMM_WORLD);
            } else {
                    //printf("last guy!\n");
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);

    }

    printf("all done\n");
    MPI_Finalize();
    printf("SUCCESS\n");
    return 0;
}

int Bcast() //bcast
{
    int size, rank, iter, ball;

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for (iter = 0; iter < size; iter++)
    {
        ball = (100 * iter) + rank;

        if (rank == iter)
        {
            printf("Broadcasting ball: %d - iter %d\n", ball, iter);
            MPI_Bcast(&ball, 1, MPI_INT, iter, MPI_COMM_WORLD);
        } else {
            MPI_Bcast(&ball, 1, MPI_INT, iter, MPI_COMM_WORLD);
            printf("Got the ball! Value: %d\n", ball);
        }
    }

    printf("all done\n");
    MPI_Finalize();
    printf("SUCCESS\n");
    return 0;
}

int Ball() //ball
{
    int size, rank, iter, ball;

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Status st;

    for (iter = 0; iter < 10; iter++)
    {

        ball = 0;

        if (rank == 0)
        {
            ball++;
            printf("Throwing the ball first Ball value: %d - iter %d\n", ball, iter);
            //usleep(200*1000);
            MPI_Send(&ball, 1, MPI_INT, rank + 1, 34, MPI_COMM_WORLD);

        } else {
            MPI_Recv(&ball, 1, MPI_INT, rank - 1, 34, MPI_COMM_WORLD, &st);
            //printf("Got the ball! New value: %d\n", ++ball);
            if (rank != size - 1)
            {
                //usleep(200*1000);
                MPI_Send(&ball, 1, MPI_INT, rank + 1, 34, MPI_COMM_WORLD);
            } else {
                //printf("last guy!\n");
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    printf("all done\n");
    MPI_Finalize();
    printf("SUCCESS\n");
    return 0;
}

int LargeMessage() //ball with large message
{
    int size, rank, iter, ball[BALL_VEC_SZ];

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Status st;

    ball[0] = 0;
    for (iter = 0; iter < 10; iter++)
    {
        if (rank == 0)
        {
            ball[0] = ball[BALL_VEC_SZ-1] = ball[0]++;
            printf("Throwing the ball first Ball value: %d - iter %d\n", ball[0], iter);
            //usleep(200*1000);
            MPI_Send(ball, BALL_VEC_SZ, MPI_INT, rank + 1, 34, MPI_COMM_WORLD);
        } else {
            MPI_Recv(ball, BALL_VEC_SZ, MPI_INT, rank - 1, 34, MPI_COMM_WORLD, &st);
            printf("Got the ball! value: %d %d\n", ball[0], ball[BALL_VEC_SZ-1]);
            if (rank != size - 1)
            {
                //usleep(200*1000);
                MPI_Send(ball, BALL_VEC_SZ, MPI_INT, rank + 1, 34, MPI_COMM_WORLD);
            } else {
                //printf("last guy!\n");
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    printf("all done\n");
    MPI_Finalize();
    printf("SUCCESS\n");
    return 0;
}

int Barrier() //barrier
{
    int size, rank, iter;

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for (iter = 0; iter < size; iter++)
    {

        if (rank == iter)
        {
            printf("Before barrier, sleeping iter %d\n", iter);
            usleep(100*1000);
            printf("Waking up! %d\n", iter);
            MPI_Barrier(MPI_COMM_WORLD);
            
        } else {
            //printf("waiting\n");
            MPI_Barrier(MPI_COMM_WORLD);
        }
        printf("barrier done iter %d\n", iter);
    }

    MPI_Finalize();
    printf("SUCCESS\n");
    return 0;
}

int Callhome() //callhome
{        
    int i, size, rank, data, home = 0;
    int buf, iter;

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Status status;

    printf("Rank: %d\tSize:%d\n", rank, size);

    for (iter = 0; iter < size; iter++)
    {
        home = iter;
        
        if (rank == home)
        {
            printf("iter %d\n", iter);
            for (i = 0; i < size; i++)
            {
                if (i == home)
                        continue;
                
                buf = 123;
                //printf("Waiting on %d\n", i);

                //pick only 1 of the next two lines to test different functionality
                if (callhome_version == 1)
                    MPI_Recv(&buf, 1, MPI_INT, i, 55, MPI_COMM_WORLD, &status);
                else
                    MPI_Recv(&buf, 1, MPI_INT, i, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                //notice: if you do MPI_ANY_SOURCE THEN YOU SHOULD EXPECT AN ERROR!
                //because this code will throw an EPIC FAIL if it determines that it received the
                //wrong message - obviously an MPI_ANY_SOURCE will receive msgs out of order and do an EPIC FAIL
                //just as it should

                //printf("***********************************************From %d got %d   iter %d\n", i, buf,iter);
                if (buf != (100 * iter) + i)
                {
                        printf("EPIC FAIL EPIC FAIL EPIC FAIL EPIC FAIL EPIC FAIL EPIC FAIL EPIC FAIL EPIC FAIL EPIC FAIL!!!!!!");
                        printf("                **************************************From %d got %d   iter %d\n", i, buf,iter);
                        exit(1);
                }
            }
        } else {
            data = (100 * iter) + rank;
            //printf("Going\n");
            MPI_Send(&data, 1, MPI_INT, home, 55, MPI_COMM_WORLD);
            //printf("Sent\n");
            //printf("Sent for iter %d\n", iter);
        }
    }

    //printf("GONNA FINISH UP!!!\n");

    MPI_Finalize();
    printf("SUCCESS\n");
    return 0;
}

int Multi() //strings and multiple ints (and MPI_Abort) and MPI_Status
{
    int size, rank, buf[3];
    char sbuf[50];

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    printf("Rank: %d\tSize:%d\n", rank, size);

    if (size < 3)
    {
        fprintf(stderr, "This app requires at least 3 nodes to run!\n");

        //this next if statement is used to prove that MPI_Abort works
        //(if all of the processes called MPI_Abort together what would it prove? nothing!)
        if (rank==0)
            MPI_Abort(MPI_COMM_WORLD, 100);
    }

    if (rank==0)
    {
        buf[3];
        sbuf[50];
        MPI_Status status;

        MPI_Recv(&buf, 3, MPI_INT, 1, 60, MPI_COMM_WORLD, &status);
        printf("Recvd %d %d %d \n", buf[0], buf[1], buf[2]);

        MPI_Recv(&sbuf, 50, MPI_CHAR, 2, 70, MPI_COMM_WORLD, &status);

        printf("Recvd %s  \n", &sbuf);
    } else if (rank==1)
    {
        buf[0] = 14;
        buf[1] = 56;
        buf[2] = -1;
        MPI_Send(&buf, 3, MPI_INT, 0, 60, MPI_COMM_WORLD);
        printf("sent\n");
    } else if (rank==2)
    {
        strcpy(sbuf,"Hello world!");
        MPI_Send(&sbuf, 50, MPI_CHAR, 0, 70, MPI_COMM_WORLD);
        printf("sent\n");
    }

    MPI_Finalize();
    printf("SUCCESS\n");
    return 0;
}

int Locks() //RMA LOCK test
{
    int i, size, rank, home = 0;
    int buf[1024];
    int iters = 5;

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Win win;

    MPI_Win_create(&buf, sizeof(int) * 512,sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    for (i = 0; i < iters; i++)
    {
        printf("Trying to acquire\n");
        MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
        printf("%d Got it .. holding on for 2 seconds\n", rank);
        sleep(2);
        printf("Releasing.. done\n");
        MPI_Win_unlock(0, win);
    }

    MPI_Finalize();

    printf("SUCCESS\n");
    return 0;
}

int RMA() //remote memory WITH LOCKS -- counting test
{
    int i, size, rank, data;
    int home = 0;
    int iters = 5;
    int buf[1024];

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Win win;

    if (rank == 0)
        buf[0] = 0;

    MPI_Win_create(buf, sizeof(int) * 1024, sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &win);

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank != 0)
    {
        for (i = 0; i < iters; i++)
        {
            MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
            MPI_Get(&data, 1, MPI_INT, 0, 0, 1, MPI_INT, win);
            MPI_Win_unlock(0, win);
            data++;
            MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);
            MPI_Put(&data, 1, MPI_INT, 0, 0, 1, MPI_INT, win);
            MPI_Win_unlock(0, win);
            printf("%d DATA %d\n",rank,data);  // dont print until AFTER unlock
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0)
    {
        printf("Final value of buf[0]: %d\n", buf[0]);
    }

    MPI_Finalize();

    printf("SUCCESS\n");
    return 0;
}
