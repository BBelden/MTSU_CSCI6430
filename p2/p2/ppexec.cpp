// Name:        Ben Belden
// Class ID#:   bpb2v
// Section:     CSCI 6430-001
// Assignment:  Lab #2
// Due:         16:20:00,February 28,2017
// Purpose:     Write a C/C++ version of mpiexec and any supporting programs.        
// Input:       From terminal.  
// Outut:       To terminal.
// 
// File:        ppexec.cpp

#include <stdio.h>  
#include <stdlib.h> 
#include <unistd.h> 
#include <fstream>
#include <string> 
#include <string.h>
#include <vector> 
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <errno.h>
#include "mpi.h"

using namespace std;

struct scktInfo
{
    int port;
    string name;
};

void error_check(int val,const char *str)    
{
    if (val<0)
    {
        printf("%s :%d: %s\n",str,val,strerror(errno));
        exit(1);
    }
}

vector<string> getRanks(string file,int nProcs)
{
	int i;
    string line;
    vector<string> hosts;
    ifstream inFile;
    inFile.open(file.c_str());

    if(inFile.good())
    {
        inFile >> line;
        hosts.push_back(line);
        for(i=1;i<nProcs;i++)
        {
            inFile >> line;
            if(!inFile)
            {
                inFile.close();
                inFile.open(file.c_str());
                inFile >> line;
            }
            hosts.push_back(line);
        }
    }
    
    else
    {
        char curHost[1000];
        gethostname(curHost,1000);
        line=curHost;
        for(i=0;i<nProcs;i++) hosts.push_back(line);
    }
    return hosts;
}

void setUpCmd(vector<string>& argvList,string &cmd,string &file,int &nProcs)
{
    int i=0;
    while(i<argvList.size())
    {
        if(argvList[i]=="-n" || argvList[i]=="-f")
        {
            if(argvList[i]=="-n") nProcs=atoi(argvList[i+1].c_str());
            else if(argvList[i]=="-f") file=argvList[i+1];
            argvList.erase(argvList.begin()+i,argvList.begin()+i+2);
            i=0;
        }
        else i++;
    }
    for(i=0;i<argvList.size();i++)
    {
        cmd+=argvList[i];
        if(i != argvList.size()-1) cmd+=" ";
    }
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
    int fromlen,client_socket,gotit,optval=1;

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

int sendStuff(int fd,int num,char * buf)
{
    uint32_t val=htonl(num);
    char * data=(char*)&val;
    int rc,rem=sizeof(val);
    do
    {
        rc=write(fd,data,rem);
        if (rc<0)
        {
            if ((errno==EAGAIN) || (errno==EWOULDBLOCK)) printf("wait for the socket to be writable\n");
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
            if ((errno==EAGAIN) || (errno==EWOULDBLOCK)) printf("wait for the socket to be writable\n");
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

int recvStuff(int fd,char * buf)
{
    uint32_t val;
    char *data=(char*)&val;
    int rc,sz,bytesRead=0,rem=sizeof(val);
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



int main(int argc,char * argv[])
{
    int i,scktLst[256],port=4401,selRC,cn=0,nProcs=1,rc,rcRank,status,counter=0,acceptSock,clientSock;
    bool doneFlag[256],hostname=false,env=false,done=false;
    char mainHost[1024],cwdPath[1024],mnBuf[1024];
    gethostname(mainHost,1024);
    string line,rankHost,rank,tblInfo,file="hostnames",cmd="",mainHostStr=mainHost,curPath;
    fd_set readSet;
    vector<string> hosts,argvList;
    ifstream inFile;
    scktInfo info[256];
    struct timeval tv;
    struct sockaddr_in sa;
    socklen_t saLen=sizeof(sa);

    if(argc>1) for(i=1;i<argc;i++) argvList.push_back(argv[i]);

    setUpCmd(argvList,cmd,file,nProcs);
    hosts=getRanks(file,nProcs);
    if(getcwd(cwdPath,sizeof(cwdPath)) != NULL) curPath=cwdPath;
    else perror("getcwd() error");

    for(i=0;i<nProcs;i++)
    {
        rc=fork();
        doneFlag[i]=false;
        if(rc==0)
        {
            rcRank=i;
            rank=to_string(i);
            rankHost=hosts[i];
            break;
        }
        else if (rc>0) doneFlag[i]=false;
    }

    if(rc>0) // parent
    {   
        doneFlag[nProcs]=false;
        acceptSock=setup_to_accept(port);
        FD_ZERO(&readSet);
        FD_SET(acceptSock,&readSet);

        while(!done)
        {
            rc=waitpid(-1,&status,WNOHANG);
            if (rc==0) // child still active
            {
                tv.tv_sec=3;
                tv.tv_usec=0;
                selRC=select(FD_SETSIZE,&readSet,NULL,NULL,&tv);
                if (selRC==0) continue;// timeout
                else if (selRC==-1 && errno==EINTR) continue;// interrupt
                else if (selRC<0)
                {
                    perror("select failed");
                    break;
                }
                if FD_ISSET(acceptSock,&readSet) // inc
                {
                    scktLst[cn]=accept_connection(acceptSock);
                    if(getsockname(scktLst[cn],(struct sockaddr*)&sa,&saLen)==-1)
                    {
                        perror("getsockname() failed");
                        break;
                    }

                    recvStuff(scktLst[cn],mnBuf);
                    string tmp=mnBuf;
                    if (tblInfo=="") tblInfo+=tmp;
                    else tblInfo+=","+tmp;
                    cn++;
                    memset(mnBuf,'\0',strlen(mnBuf));

                    if (cn==nProcs)
                    {
                        tblInfo.copy(mnBuf,sizeof(mnBuf));
                        for(i=0;i<cn;i++) sendStuff(scktLst[i],sizeof(mnBuf),mnBuf);
                    }
                }
            }
            else if (rc==-1) printf("RC -1 error on waitpid\n");// error
            else // child done
            {
                counter++;
                if(counter==nProcs) done=true;
            }
        } 
    }

    else if(rc==0) // child
    {
        char * args[]={(char*)"ssh",(char*)"c00",(char*)"bash -c",'\0'};
        args[1]=(char*)rankHost.c_str();
        line="bash -c \'PWD="+curPath+"; cd $PWD;PP_MPI_RANK="+rank+
			" PP_MPI_SIZE="+to_string(nProcs)+" PP_MPI_HOST_PORT="+mainHostStr+
			":4401 "+cmd +'\'';
        args[2]=(char*)line.c_str();
        execvp((char*)"ssh",args);
        perror("execvp");
        exit(0);
    }
    else printf("Fork error\n");
    return 0;
}

