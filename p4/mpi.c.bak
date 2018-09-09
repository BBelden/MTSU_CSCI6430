// Name:        Ben Belden
// Class ID#:   bpb2v
// Section:     CSCI 6430-001
// Assignment:  Lab #3
// Due:         16:20:00,March 21,2017
// Purpose:     Write a C/C++ version of mpiexec and any supporting programs.        
// Input:       From terminal.  
// Outut:       To terminal.
// 
// File:        mpi.c
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <errno.h>
#include "mpi.h"

int size,rank,accptSckt,clntSckt,scktSz=0,myPort,servPort,listInd=0;
int serv2clnt[256],clnt2serv[256],portList[256],rankList[256],readBools[256];
int commSize,commList[256];
char hostList[256][1024],tableInfo[1024],tableInfoRows[256][1024];
struct timeval tv;
fd_set readSet;

void error_check(int val,const char *str){
    if (val<0){ printf("%s :%d: %s\n",str,val,strerror(errno)); exit(1); }
}

int sendStuff(int fd,int num,char * buf){
    uint32_t val=htonl(num);
    char *data=(char*)&val;
    int remaining=sizeof(val),rc;

    do{
        rc=write(fd,data,remaining);
        if (rc<0){
            if ((errno==EAGAIN) || (errno==EWOULDBLOCK)) printf("wait for socket to be writable\n");
            else if (errno != EINTR){ printf("EINTR: MPI send interrupted\n"); return -1; }
        }
        else{ data+=rc;  remaining-=rc; }
    }
    while (remaining>0);
    
    remaining=num;
    do{
        rc=write(fd,buf,remaining);
        if (rc<0){
            if ((errno==EAGAIN) || (errno==EWOULDBLOCK)) printf("wait for socket to be writable\n");
            else if (errno != EINTR){ printf("EINTR: MPI send interrupted with\n"); return -1; }
        }
        else{ buf+=rc;  remaining-=rc; }
    }
    while (remaining > 0);
    return 0;
}

int recvStuff(int fd,char * buf){
    uint32_t val;
    int rc,bytesRead=0,size,remaining=sizeof(val);
    char *data=(char*)&val;

    do{
        rc=read(fd,data,remaining);
        if (val <= 0){
            if ((errno==EAGAIN) || (errno==EWOULDBLOCK))
                printf("Use select or epoll to wait for the socket to be readable again\n");
            else if (errno != EINTR){ printf("EINTR occurred\n"); return -1; }
        }
        else{ data+=rc;  remaining-=rc; }
    }
    while (remaining > 0);
    size=ntohl(val);

    while(bytesRead<size){
        rc=read(fd,buf + bytesRead,size - bytesRead);
        if (rc<1){ printf("Error during 2nd phase in recv\n"); return -1; }
        bytesRead+=rc;
    }
    return 0;
}

int setup_to_accept(int port){
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

int accept_connection(int accept_socket){
    struct sockaddr_in from;
    int fromlen,client_socket,gotit,optval=1;
    fromlen=sizeof(from);
    gotit=0;
    while (!gotit){
        client_socket=accept(accept_socket,(struct sockaddr *)&from,(socklen_t *)&fromlen);
        if (client_socket==-1){
            if (errno==EINTR)  continue;
            else  error_check(client_socket,"accept_connection accept");
        }
        else  gotit=1;
    }
    setsockopt(client_socket,IPPROTO_TCP,TCP_NODELAY,(char *)&optval,sizeof(optval));
    return(client_socket);
}

int connect_to_server(char *hostname,int port){
    int rc,optval=1,client_socket;
    struct sockaddr_in listener;
    struct hostent *hp;
    hp=gethostbyname(hostname);
    if (hp==NULL){
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

int checkComm(int comm){
    int i;
    for (i=0;i<commSize;i++)
        if (comm==commList[i])
            return 1;
    return 0;
}

int MPI_Finalize(void) { return MPI_SUCCESS;}

int MPI_Comm_size(MPI_Comm x,int *y){
    if (x==MPI_COMM_WORLD){ *y=size; return MPI_SUCCESS; }
    return MPI_ERR_COMM;
}

int MPI_Comm_rank(MPI_Comm x,int *y){
    if (x==MPI_COMM_WORLD){ *y=rank; return MPI_SUCCESS; }
    return MPI_ERR_COMM;
}

int MPI_Comm_dup(MPI_Comm comm,MPI_Comm *newcomm){
    if (commSize==0){ commList[0]=comm+1; *newcomm=commList[0]; commSize++; }
    else {
        commList[commSize]=commList[commSize-1]+1;
        *newcomm=commList[commSize]; commSize++;
    }
}

double MPI_Wtime(void) {
    struct timeval tv;
    gettimeofday(&tv,(struct timezone *)0);
    return (tv.tv_sec+(tv.tv_usec/1000000.0));
}

int MPI_Init(int *x,char ***y){
    int i=0,tmpPort,tmpRank;
    char *hostPort[2],curHost[1024],portStr[100],rankStr[100],container[1024],buffer[1024];
    char *hostInfo=getenv("PP_MPI_HOST_PORT");
    char *p=strtok(hostInfo,":");

    setbuf(stdout,NULL);
    gethostname(curHost,1024);
    size=atoi(getenv("PP_MPI_SIZE"));
    rank=atoi(getenv("PP_MPI_RANK"));
    while (p != NULL) { hostPort[i++]=p; p=strtok (NULL,":"); }
    
    commSize=0;
    servPort=atoi(hostPort[1]);
    myPort=servPort + rank + 1;
    sprintf(portStr,"%d",myPort);  sprintf(rankStr,"%d",rank);
    accptSckt=setup_to_accept(myPort);
    strcpy(buffer,curHost);  buffer[strlen(curHost)]=':';  buffer[strlen(curHost)+1]='\0';
    strcpy(buffer+strlen(buffer),portStr);  buffer[strlen(buffer)]=':';  buffer[strlen(buffer)+1]='\0';
    strcpy(buffer+strlen(buffer),rankStr);
    clntSckt=connect_to_server(hostPort[0],servPort);
    sendStuff(clntSckt,sizeof(buffer),buffer);
    recvStuff(clntSckt,buffer);
    strcpy(tableInfo,buffer);

    i=0;
    p=strtok(tableInfo,",");
    while (p != NULL) { strcpy(tableInfoRows[i],p); p=strtok(NULL,","); listInd++; i++; scktSz++; }

    for(listInd=0;listInd<i;listInd++){
        p=strtok(tableInfoRows[listInd],":");
        strcpy(container,p);
        p=strtok(NULL,":");  tmpPort=atoi(p);
        p=strtok(NULL,":");  tmpRank=atoi(p);
        strcpy(hostList[tmpRank],container);
        portList[tmpRank]=tmpPort;  rankList[tmpRank]=tmpRank;
    }
    return MPI_SUCCESS;
}

int MPI_Barrier(MPI_Comm comm){
    int i,done=0;
    char buffer[1024];

    if (comm != MPI_COMM_WORLD)  return MPI_ERR_COMM;
    while (!done){
        if (rank==0){
            strcpy(buffer,(char*)"STOP");
            for(i=1;i<size;i++){
                if (serv2clnt[i]==0)  serv2clnt[i]=accept_connection(accptSckt);
                sendStuff(serv2clnt[i],sizeof(buffer),buffer);
            } done=1;
        }
        else{
            if (clnt2serv[0]==0)  clnt2serv[0]=connect_to_server(hostList[0],portList[0]);
            recvStuff(clnt2serv[0],buffer);
            if (strcmp(buffer,(char*)"STOP")==0)  done=1;
        }
    } done=0;
    while (!done){
        if (rank==0) {
            for(i=1;i<size;i++)  sendStuff(serv2clnt[i],sizeof(buffer),buffer);
            done=1;
        }
        else{
            recvStuff(clnt2serv[0],buffer);
            if (strcmp(buffer,(char*)"STOP")==0)  done=1;
        }
    }
    return MPI_SUCCESS;
}

int MPI_Send(void *buffer,int count,MPI_Datatype x,int dest,int tag,MPI_Comm comm){
    int i,done=0,val[count];
    char commBuf[100],tagBuf[100],rankBuf[100],ackBuf[1024],*buf=buffer;
    
    if (dest<0 || dest>=size)  return MPI_ERR_RANK;
    if (tag<0)  return MPI_ERR_TAG;
    if (comm != MPI_COMM_WORLD){
        int good=checkComm(comm); // 0 = not found, 1 = found comm
        if (!good)  return MPI_ERR_COMM;
    }
    if (x != MPI_INT && x != MPI_CHAR)  return MPI_ERR_TYPE;

    sprintf(commBuf,"%d",comm);
    sprintf(tagBuf,"%d",tag);
    sprintf(rankBuf,"%d",rank);
    if (clnt2serv[dest]==0){ // if no conn, connect and send curr rank
        clnt2serv[dest]=connect_to_server(hostList[dest],portList[dest]);
        sendStuff(clnt2serv[dest],sizeof(rankBuf),rankBuf);
    }

    while (!done){
        sendStuff(clnt2serv[dest],sizeof(commBuf),commBuf); // send comm
        recvStuff(clnt2serv[dest],ackBuf);
        if (strcmp(ackBuf,(char*)"BAD")==0){ // if bad comm
            memset(ackBuf,'\0',strlen(ackBuf)); continue;
        }
        sendStuff(clnt2serv[dest],sizeof(tagBuf),tagBuf);
        recvStuff(clnt2serv[dest],ackBuf);
        if (strcmp(ackBuf,(char*)"GOOD")==0){
            sendStuff(clnt2serv[dest],sizeof(buf),buf); done=1;
        }
        memset(ackBuf,'\0',strlen(ackBuf));
    }
    return MPI_SUCCESS;
}

int MPI_Ssend(void *buffer,int count,MPI_Datatype x,int dest,int tag,MPI_Comm comm)
{  return MPI_Send(buffer,count,x,dest,tag,comm); }

int MPI_Recv(void *buffer,int count,MPI_Datatype x,int source,int tag,MPI_Comm comm,MPI_Status *status){
    int i,n,rc,tmpSock,recvTag,recvComm,done=0,realSource,realTag;
    char buf[1024],rankBuf[100];
    
    if (source != MPI_ANY_SOURCE){
        if (source<0 || source >= size){
            (*status).MPI_ERROR=MPI_ERR_RANK; return MPI_ERR_RANK;
        }
    }
    if (tag<0){
        (*status).MPI_ERROR=MPI_ERR_TAG; return MPI_ERR_TAG;
    }
    if (comm != MPI_COMM_WORLD){
        int good=checkComm(comm); // 0=not found,1=found comm
        if (!good){ (*status).MPI_ERROR=MPI_ERR_COMM; return MPI_ERR_COMM; }
    }
    if (x != MPI_INT && x != MPI_CHAR){
        (*status).MPI_ERROR=MPI_ERR_TYPE; return MPI_ERR_TYPE;
    }
    FD_ZERO( &readSet );
    FD_SET( accptSckt,&readSet );

    for (i=0;i<size;i++){ if (serv2clnt[i] != 0)  FD_SET( serv2clnt[atoi(rankBuf)],&readSet ); }
    
    while(!done){
        tv.tv_sec=2;  tv.tv_usec=0;
        rc=select(FD_SETSIZE,&readSet,NULL,NULL,&tv);

        if (rc==0)  continue; // timeout
        else if (rc==-1 && errno==EINTR)  continue; // interrupt
        else if (rc<0){ // fail
            perror("select failed"); break;
        }
        if FD_ISSET(accptSckt,&readSet) // incoming conn
        {   // assign fd to temp variable,recv rank,assign tmp sckt to main list,add to select read set
            tmpSock=accept_connection(accptSckt);
            recvStuff(tmpSock,rankBuf);
            serv2clnt[atoi(rankBuf)]=tmpSock;
            FD_SET(serv2clnt[atoi(rankBuf)],&readSet);
        }

        for (i=0;i<size;i++){ // check all sckts
            if (i==rank || serv2clnt[i]==0)  continue;// ignore my rank or unconnected sckts
            if ( FD_ISSET(serv2clnt[i],&readSet)){
                n=recvStuff(serv2clnt[i],buf);
                if (n==-1)  ; // close socket?
                else{
                    recvComm=atoi(buf);
                    memset(buf,'\0',strlen(buf));
                    if (MPI_COMM_WORLD==comm || checkComm(recvComm)==1){ // if valid comm
                        strcpy(buf,(char*)"GOOD");
                        sendStuff(serv2clnt[i],sizeof(buf),buf);
                        memset(buf,'\0',strlen(buf));
                    }
                    else{ // invalid comm
                        strcpy(buf,(char*)"BAD");
                        sendStuff(serv2clnt[i],sizeof(buf),buf);
                        memset(buf,'\0',strlen(buf)); continue;
                    }
                    recvStuff(serv2clnt[i],buf);
                    recvTag=atoi(buf);
                    if (tag==recvTag || tag==MPI_ANY_TAG){ // good tag
                        memset(buf,'\0',strlen(buf));
                        strcpy(buf,(char*)"GOOD");
                        sendStuff(serv2clnt[i],sizeof(buf),buf);
                        memset(buf,'\0',strlen(buf));
                        recvStuff(serv2clnt[i],buf);
                        realTag=recvTag;  realSource=i;  done=1;  break;
                    }
                    else{ // bad tag
                        memset(buf,'\0',strlen(buf));
                        strcpy(buf,(char*)"BAD");
                        sendStuff(serv2clnt[i],sizeof(buf),buf);
                        memset(buf,'\0',strlen(buf));
                        FD_CLR( serv2clnt[i],&readSet );
                    }
                } // end else
            } // end if
        } // end for
    } // end while
    if (x==MPI_INT)  *(int*)buffer=*(int *)buf;
    else if (x==MPI_CHAR)  *(char*)buffer=*buf;
    (*status).count=count;
    (*status).MPI_SOURCE=realSource;
    (*status).MPI_TAG=realTag;
    return MPI_SUCCESS;
}

int MPI_Bcast(void *buffer,int count,MPI_Datatype x,int root,MPI_Comm comm)
{
    int i;
    char *buf=buffer,cBuf[1024];

    if (comm != MPI_COMM_WORLD){
        int good=checkComm(comm);// 0=not found,1=found comm
        if (!good)  return MPI_ERR_COMM;
    }
    if (x != MPI_INT && x != MPI_CHAR)  return MPI_ERR_TYPE;
    if (rank==root){
        for(i=0;i<size;i++){ // are other ranks connected?
            if (i==rank)  continue;// skip if your rank
            // if no connection,accept connection
            if (serv2clnt[i]==0)  serv2clnt[i]=accept_connection(accptSckt);
            sendStuff(serv2clnt[i],sizeof(buf),buf);// send buffer
        }
    }
    else{
        // if not connected,make connection
        if (clnt2serv[root]==0)  clnt2serv[root]=connect_to_server(hostList[root],portList[root]);
        recvStuff(clnt2serv[root],cBuf);
        for (i=0;i<count;i++){
            if (x==MPI_INT)  ((int*)buffer)[i]=((int *)cBuf)[i];
            else if (x==MPI_CHAR)  ((char*)buffer)[i]=cBuf[i];
        }
    }
    MPI_Barrier(comm);
    return MPI_SUCCESS;
}

int MPI_Gather(void *sendbuf,int sendcnt,MPI_Datatype sendtype,void *recvbuf,int recvcnt,MPI_Datatype recvtype,int root,MPI_Comm comm){
    int i,j;
    char *sndPkt=sendbuf,recvPkt[1024];

    if (comm != MPI_COMM_WORLD){
        int good=checkComm(comm);// 0=not found,1=found comm
        if (!good)  return MPI_ERR_COMM;
    }
    if (recvtype != MPI_INT && recvtype != MPI_CHAR)  return MPI_ERR_TYPE;
    if (sendtype != MPI_INT && sendtype != MPI_CHAR)  return MPI_ERR_TYPE;
    if (rank==root){ // gather em up!
        for(i=0;i<size;i++){ // are other ranks connected?
            if (i==rank){ // skip if your rank
                for (j=0;j<recvcnt;j++){
                    if (recvtype==MPI_INT)  ((int*)recvbuf)[i*recvcnt+j]=((int *)sendbuf)[j];
                    else if (recvtype==MPI_CHAR)  ((char*)recvbuf)[i*recvcnt+j]=((char*)sendbuf)[j];
                }  continue;
            }
            // if not connected,make conn
            if (clnt2serv[i]==0)  clnt2serv[i]=connect_to_server(hostList[i],portList[i]);
            recvStuff(clnt2serv[i],recvPkt);
            for (j=0;j<recvcnt;j++){
                if (recvtype==MPI_INT)  ((int*)recvbuf)[i]=((int *)recvPkt)[j];
                else if (recvtype==MPI_CHAR)  ((char*)recvbuf)[i*recvcnt+j]=recvPkt[j];
            }
            memset(recvPkt,'\0',strlen(recvPkt));
        }
    }
    else{
        // if not connected,accept conn
        if (serv2clnt[root]==0)  serv2clnt[root]=accept_connection(accptSckt);
        sendStuff(serv2clnt[root],sizeof(sndPkt),sndPkt); // send buffer
    }
}

