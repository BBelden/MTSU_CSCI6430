// Name:        Ben Belden
// Class ID#:   bpb2v
// Section:     CSCI 6430-001
// Assignment:  Lab #2
// Due:         16:20:00, February 28, 2017
// Purpose:     Write a C/C++ version of mpiexec and any supporting programs.        
// Input:       From terminal.  
// Outut:       To terminal.
// 
// File:        mpiInit.c

int MPI_Init(int *x, char ***y)
{
    int i = 0, tmpPort, tmpRank;
    char * hostInfo = getenv("PP_MPI_HOST_PORT");
    char * p = strtok (hostInfo, ":");
    char * hostPort[2], curHost[1024], portStr[100], rankStr[100];
    char container[1024];
    char buffer[1024];

    setbuf(stdout, NULL);
    gethostname(curHost, 1024);
    size = atoi(getenv("PP_MPI_SIZE"));
    rank = atoi(getenv("PP_MPI_RANK"));
    while (p != NULL)
    {
        hostPort[i++] = p;
        p = strtok (NULL, ":");
    }

    servPort = atoi(hostPort[1]);
    myPort = servPort + rank + 1;
    sprintf(portStr, "%d", myPort);
    sprintf(rankStr, "%d", rank);

    acceptSock = setup_to_accept(myPort);
    strcpy(buffer, curHost);

    buffer[strlen(curHost)] = ':';
    buffer[strlen(curHost)+1] = '\0';
    strcpy(buffer+strlen(buffer), portStr);
    buffer[strlen(buffer)] = ':';
    buffer[strlen(buffer)+1] = '\0';
    strcpy(buffer+strlen(buffer), rankStr);

    clientSock = connect_to_server(hostPort[0], servPort);

    reliableSend(clientSock, sizeof(buffer), buffer);
    reliableRecv(clientSock, buffer);
    strcpy(tableInfo, buffer);

    i = 0;
    p = strtok (tableInfo, ",");
    while (p != NULL)
    {
        strcpy(tableInfoRows[i], p);
        p = strtok (NULL, ",");
        listIndex++;
        i++;
        sockSize++;
    }

    for(listIndex = 0; listIndex < i; listIndex++)
    {
        p = strtok (tableInfoRows[listIndex], ":");
        strcpy(container, p);
        p = strtok (NULL, ":");
        tmpPort = atoi(p);
        p = strtok (NULL, ":");
        tmpRank = atoi(p);
        strcpy(hostList[tmpRank], container);
        portList[tmpRank] = tmpPort;
        rankList[tmpRank] = tmpRank;
    }
    return MPI_SUCCESS;
}

