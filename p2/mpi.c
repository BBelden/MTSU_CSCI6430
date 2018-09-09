// Name:        Ben Belden
// Class ID#:   bpb2v
// Section:     CSCI 6430-001
// Assignment:  Lab #2
// Due:         16:20:00,February 28,2017
// Purpose:     Write a C/C++ version of mpiexec and any supporting programs.        
// Input:       From terminal.  
// Outut:       To terminal.
// 
// File:        mpi.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <errno.h>
#include "mpi.h"

int size,rank,accptSckt,clntSckt,scktSz=0,myPort,servPort,listInd=0;
int serv2clnt[256],clnt2serv[256],portList[256],rankList[256],readBools[256];
char hostList[256][1024],tableInfo[1024],tableInfoRows[256][1024];
struct timeval tv;
fd_set readSet;  

void error_check(int val,const char *str)    
{
    if (val<0)
    {
        printf("%s :%d: %s\n",str,val,strerror(errno));
        exit(1);
    }
}

int sendStuff(int fd,int num,char * buf)
{
    uint32_t val=htonl(num);
    int rc,rem=sizeof(val);
    char *data=(char*)&val;
    do
    {
        rc=write(fd,data,rem);
        if (rc<0)
        {
            if ((errno==EAGAIN) || (errno==EWOULDBLOCK))
                printf("wait for the socket to be writable\n");
            else if (errno != EINTR)
            {
                printf("EINTR: MPI send interrupted\n");
                return -1;
            }
        }
        else
        {
            data+=rc;
            rem-=rc;
        }
    }
    while (rem>0);
    
    rem=num;
    do
    {
        rc=write(fd,buf,rem);
        if (rc<0)
        {
            if ((errno==EAGAIN) || (errno==EWOULDBLOCK))
                printf("wait for the socket to be writable\n");
            else if (errno != EINTR)
            {
                printf("EINTR: MPI send interrupted\n");
                return -1;
            }
        }
        else
        {
            buf+=rc;
            rem-=rc;
        }
    }
    while (rem>0);
    return 0;
}

int recvStuff(int fd,char *buf)
{
    uint32_t val;
    int rc,bytesRead=0,sz,rem=sizeof(val);
    char *data=(char*)&val;
    do
    {
        rc=read(fd,data,rem);
        if (val <= 0)
        {
            if ((errno==EAGAIN) || (errno==EWOULDBLOCK))
                printf("Use select or epoll to wait for the socket to be readable again\n");
            else if (errno != EINTR)
            {
                printf("EINTR occurred\n");
                return -1;
            }
        }
        else
        {
            data+=rc;
            rem-=rc;
        }
    }
    while (rem>0);
    sz=ntohl(val);

    while(bytesRead<sz)
    {
        rc=read(fd,buf+bytesRead,sz-bytesRead);
        if (rc<1)
        {
            printf("Error during 2nd phase in recv\n");
            return -1;
        }
        bytesRead+=rc;
    }
    return 0;
}

int setup_to_accept(int port)
{
    int rc,accept_socket,optval=1;
    struct sockaddr_in sin;
    sin.sin_family=AF_INET;
    sin.sin_addr.s_addr=INADDR_ANY;
    sin.sin_port=htons(port);
    accept_socket=socket(AF_INET,SOCK_STREAM,0);
    error_check(accept_socket,"setup_to_accept socket");
    setsockopt(accept_socket,SOL_SOCKET,SO_REUSEADDR,(char *)&optval,sizeof(optval));
    rc=bind(accept_socket,(struct sockaddr *)&sin ,sizeof(sin));
    error_check(rc,"setup_to_accept bind");
    rc=listen(accept_socket,256);
    error_check(rc,"setup_to_accept listen");
    return(accept_socket);
}

int accept_connection(int accept_socket)    
{
    struct sockaddr_in from;
    int fromlen,client_socket,gotit;
    int optval=1;
    fromlen=sizeof(from);
    gotit=0;
    while (!gotit)
    {
        client_socket=accept(accept_socket,(struct sockaddr *)&from,(socklen_t *)&fromlen);
        if (client_socket==-1)
        {
            if (errno==EINTR) continue;
            else error_check(client_socket,"accept_connection accept");
        }
        else gotit=1;
    }
    setsockopt(client_socket,IPPROTO_TCP,TCP_NODELAY,(char *)&optval,sizeof(optval));
    return(client_socket);
}

int connect_to_server(char *hostname,int port)    
{
    int rc,client_socket,optval=1;
    struct sockaddr_in listener;
    struct hostent *hp;

    hp=gethostbyname(hostname);
    if (hp==NULL)
    {
        printf("connect_to_server: gethostbyname %s: %s -- exiting\n",hostname,strerror(errno));
        exit(-1);
    }

    bzero((void *)&listener,sizeof(listener));
    bcopy((void *)hp->h_addr,(void *)&listener.sin_addr,hp->h_length);
    listener.sin_family=hp->h_addrtype;
    listener.sin_port=htons(port);
    client_socket=socket(AF_INET,SOCK_STREAM,0);
    error_check(client_socket,"net_connect_to_server socket");
    rc=connect(client_socket,(struct sockaddr *) &listener,sizeof(listener));
    error_check(rc,"net_connect_to_server connect");
    setsockopt(client_socket,IPPROTO_TCP,TCP_NODELAY,(char *)&optval,sizeof(optval));
    return(client_socket);
}






int MPI_Finalize(void) { return MPI_SUCCESS;}

int MPI_Comm_size(MPI_Comm x,int *y)
{
    if (x==MPI_COMM_WORLD)
    {
        *y=size;
        return MPI_SUCCESS;
    }
    return MPI_ERR_COMM;
}

int MPI_Comm_rank(MPI_Comm x,int *y)
{
    if (x==MPI_COMM_WORLD)
    {
        *y=rank;
        return MPI_SUCCESS;
    }
    return MPI_ERR_COMM;
}

int MPI_Init(int *x,char ***y)
{
    int i=0,tmpPort,tmpRank;
    char buffer[1024],container[1024];
    char *hostInfo=getenv("PP_MPI_HOST_PORT"),*p=strtok(hostInfo,":");
    char *hostPort[2],curHost[1024],portStr[100],rankStr[100];

    setbuf(stdout,NULL);
    gethostname(curHost,1024);
    size=atoi(getenv("PP_MPI_SIZE"));
    rank=atoi(getenv("PP_MPI_RANK"));
    while (p != NULL)
    {
        hostPort[i++]=p;
        p=strtok (NULL,":");
    }

    servPort=atoi(hostPort[1]);
    myPort=servPort+rank+1;
    sprintf(portStr,"%d",myPort);
    sprintf(rankStr,"%d",rank);
    accptSckt=setup_to_accept(myPort);
    strcpy(buffer,curHost);
    buffer[strlen(curHost)]=':';
    buffer[strlen(curHost)+1]='\0';
    strcpy(buffer+strlen(buffer),portStr);
    buffer[strlen(buffer)]=':';
    buffer[strlen(buffer)+1]='\0';
    strcpy(buffer+strlen(buffer),rankStr);
    clntSckt=connect_to_server(hostPort[0],servPort);
    sendStuff(clntSckt,sizeof(buffer),buffer);
    recvStuff(clntSckt,buffer);
    strcpy(tableInfo,buffer);

    i=0;
    p=strtok (tableInfo,",");
    while (p != NULL)
    {
        strcpy(tableInfoRows[i],p);
        p=strtok (NULL,",");
        listInd++;
        i++;
        scktSz++;
    }

    for(listInd=0;listInd<i;listInd++)
    {
        p=strtok (tableInfoRows[listInd],":");
        strcpy(container,p);
        p=strtok (NULL,":");
        tmpPort=atoi(p);
        p=strtok (NULL,":");
        tmpRank=atoi(p);
        strcpy(hostList[tmpRank],container);
        portList[tmpRank]=tmpPort;
        rankList[tmpRank]=tmpRank;
    }
    return MPI_SUCCESS;
}

// TODO ... well, why isn't this working? 
int MPI_Barrier(MPI_Comm comm)
{
    int i,done=0;
    char buf[1024];

    if (comm != MPI_COMM_WORLD) return MPI_ERR_COMM;

    while (!done)
    {
        if (rank==0)
        {
            strcpy(buf,(char*)"STOP");
            for(i=1;i<size;i++)
            {
                if (serv2clnt[i]==0) serv2clnt[i]=accept_connection(accptSckt);
                sendStuff(serv2clnt[i],sizeof(buf),buf);
            }
            done=1;
        }
        else // not connected
        {
            if (clnt2serv[0]==0) clnt2serv[0]=connect_to_server(hostList[0],portList[0]);
            recvStuff(clnt2serv[0],buf);
            if (strcmp(buf,(char*)"STOP")==0) done=1;
        }
    }
    done=0;

    while (!done)
    {
        if (rank==0)
        {
            for(i=1;i<size;i++) sendStuff(serv2clnt[i],sizeof(buf),buf);
            done=1;
        }
        else
        {
            recvStuff(clnt2serv[0],buf);
            if (strcmp(buf,(char*)"STOP")==0) done=1;
        }
    }
    return MPI_SUCCESS;
}

int MPI_Send(void *buffer,int count,MPI_Datatype x,int dest,int tag,MPI_Comm comm)
{
    int i,done=0,val[count];
    char tagBuf[100],rankBuf[100],ackBuf[1024],*buf=buffer;
    
    if (dest<0 || dest >= size) return MPI_ERR_RANK;
    if (tag<0) return MPI_ERR_TAG;
    if (comm != MPI_COMM_WORLD) return MPI_ERR_COMM;
    if (x != MPI_INT && x != MPI_CHAR) return MPI_ERR_TYPE;

    sprintf(tagBuf,"%d",tag);
    sprintf(rankBuf,"%d",rank);
    if (clnt2serv[dest]==0) // not connected
    {
        clnt2serv[dest]=connect_to_server(hostList[dest],portList[dest]);
        sendStuff(clnt2serv[dest],sizeof(rankBuf),rankBuf);
    }

    while (!done)
    {
        sendStuff(clnt2serv[dest],sizeof(tagBuf),tagBuf);
        recvStuff(clnt2serv[dest],ackBuf);
        if (strcmp(ackBuf,(char*)"GOOD")==0)
        {
            sendStuff(clnt2serv[dest],sizeof(buf),buf);
            done=1;
        }
        memset(ackBuf,'\0',strlen(ackBuf));
    }
    return MPI_SUCCESS;
}

int MPI_Ssend(void *buffer,int count,MPI_Datatype x,int dest,int tag,MPI_Comm comm)
{
    return MPI_Send(buffer,count,x,dest,tag,comm);
}

int MPI_Recv(void *buffer,int count,MPI_Datatype x,int source,int tag,MPI_Comm comm,MPI_Status *status)
{
    int i,n,rc,tmpSock,recvTag,done=0,realSource,realTag;
    char buf[1024],rankBuf[100];
    
    if (source != MPI_ANY_SOURCE)
    {
        if (source<0 || source >= size)
        {
            (*status).MPI_ERROR=MPI_ERR_RANK;
            return MPI_ERR_RANK;
        }
    }
    if (tag<0)
    {
        (*status).MPI_ERROR=MPI_ERR_TAG;
        return MPI_ERR_TAG;
    }
    if (comm != MPI_COMM_WORLD)
    {
        (*status).MPI_ERROR=MPI_ERR_COMM;
        return MPI_ERR_COMM;
    }
    if (x != MPI_INT && x != MPI_CHAR)
    {
        (*status).MPI_ERROR=MPI_ERR_TYPE;
        return MPI_ERR_TYPE;
    }
    FD_ZERO(&readSet);
    FD_SET(accptSckt,&readSet);

    while(!done)
    {
        tv.tv_sec=2;
        tv.tv_usec=0;
        rc=select(FD_SETSIZE,&readSet,NULL,NULL,&tv);

        if (rc==0) continue; // timeout
        else if (rc==-1 && errno==EINTR) continue; // interrupt
        else if (rc<0) // fail
        {
            perror("select failed");
            break;
        }
        if FD_ISSET(accptSckt,&readSet) // inc conn
		{
            tmpSock=accept_connection(accptSckt);
            recvStuff(tmpSock,rankBuf);
            serv2clnt[atoi(rankBuf)]=tmpSock;
            FD_SET(serv2clnt[atoi(rankBuf)],&readSet);
        }

        for (i=0;i<size;i++)
		{
            if (i==rank || serv2clnt[i]==0) continue;
            if (FD_ISSET(serv2clnt[i],&readSet))
            {
                n=recvStuff(serv2clnt[i],buf);
                if (n!=-1) // recvd
                {
                    recvTag=atoi(buf);
                    if (tag==recvTag || tag==MPI_ANY_TAG)
					{
                        memset(buf,'\0',strlen(buf));
                        strcpy(buf,(char*)"GOOD");
                        sendStuff(serv2clnt[i],sizeof(buf),buf);
                        memset(buf,'\0',strlen(buf));
                        recvStuff(serv2clnt[i],buf);
                        realTag=recvTag;
                        realSource=i;
                        done=1;
                        break;
                    }
                    else
                    {
                        memset(buf,'\0',strlen(buf));
                        strcpy(buf,(char*)"BAD");
                        sendStuff(serv2clnt[i],sizeof(buf),buf);
                        memset(buf,'\0',strlen(buf));
                        FD_CLR(serv2clnt[i],&readSet);
                    }
                }
            } 
        } 
    } 

    if (x==MPI_INT) *(int*)buffer=*(int *)buf;
    else if (x==MPI_CHAR) *(char*)buffer=*buf;
    (*status).MPI_SOURCE=realSource;
    (*status).MPI_TAG=realTag;
    return MPI_SUCCESS;
}

