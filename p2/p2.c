// Name:        Ben Belden
// Class ID#:   bpb2v
// Section:     CSCI 6430-001
// Assignment:  Lab #1
// Due:         18:00:00, February 2, 2017
// Purpose:     Write a C/C++ version of mpiexec and any supporting programs.        
// Input:       From terminal.  
// Outut:       To terminal.
// 
 // File:        p1.c

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"

int main (int argc, char * argv[])
{
	int i=0,j=0,rc=0,sz=0,firstArg=1,sshArgLen=0;
	int hasF=0,hostFileExists=0,hasN=0,n=0,hostRanks=0,status;
	FILE *f;
	char *rank="PP_MPI_RANK=",*size = "PP_MPI_SIZE=";
	char *port1="PP_MPI_HOST_PORT=",*port2 = ":9999";
	char *ssh="ssh",*arg1="bash -c '",*end="'",*space=" ";
	char *hostArr[99],*hostFile,*sizeStr,*ppSize,*rankStr,*ppRank;
	char *ppPort,*sshHost,*sshArg,bashCmd[100],hostName[1024];
	bashCmd[100]='\0';
	hostName[1024]='\0';
	gethostname(hostName,1024);
	pid_t p;

	while (i < argc) // determine whether -n &/or -f flags set
	{
		if (strcmp(argv[i],"-n")==0)
		{
			hasN = 1;
			i++;
			n = atoi(argv[i]);
		}
		if (strcmp(argv[i],"-f")==0)
		{
			hasF = 1;
			i++;
			hostFile = malloc(strlen(argv[i]));
			strcpy(hostFile,argv[i]);
		}
		i++;
	}

	if (!hasN) n=1; // assume 1 rank if -n not set
	else firstArg+=2; // advance past -n flag info

	if(!hasF)
	{
		hostFile = malloc(9); // assume hostFile is "hostnames"
		strcpy(hostFile,"hostnames");
	}
	else firstArg+=2; // advance past -f flag info
	
	// check to see if hostFile is in local directory
	if (access(hostFile,F_OK)!=-1) hostFileExists=1;

	if (hostFileExists) // open file, malloc space for each host, read in hosts 
	{
		f = fopen(hostFile,"r");
		sz=0,j=1;
		for (i=0;i<99 && j!=-1;i++)
		{
			j = fscanf(f,"%*s%n",&sz);
			hostArr[i] = malloc((sz+1)*sizeof(*hostArr[i]));
			hostRanks++;
		}
		fclose(f);
		hostRanks--; // had to try to read one past the number assigned

		f = fopen(hostFile,"r");
		for (i=0;i<hostRanks;i++) fscanf(f,"%s",hostArr[i]);
		fclose(f);
	}


	// get bash cmd and any args
	for (i=firstArg;i<argc;i++)
	{
		if (firstArg+1 == argc)
		{
			strcpy(bashCmd,argv[i]);
		}
		else if (i == firstArg && firstArg+1 != argc)
		{
			strcpy(bashCmd,argv[i]);
			strcat(bashCmd,space);
		}
		else
		{
			strcat(bashCmd,argv[i]);
			if (i != argc-1) strcat(bashCmd,space);
		}
	}

	// malloc and assign any vars that don't need to be in loop
	sshHost = malloc(3*sizeof(char)); // assume c00-c32 hosts
	sizeStr = malloc(snprintf(NULL,0,"%d",n));
	snprintf(sizeStr,snprintf(NULL,0,"%02d",n),"%d",n);
	ppSize = malloc((strlen(size)+strlen(sizeStr)+1)*sizeof(char));
	strcpy(ppSize,size); strcat(ppSize,sizeStr); strcat(ppSize,space);

	// loop through the ranks in the host file
	i=j=0;
	while(j<n)
	{
		if (!hostFileExists) strcpy(sshHost,hostName);
		else strcpy(sshHost,hostArr[i]);
		rankStr = malloc(snprintf(NULL,0,"%d",i));
		snprintf(rankStr,snprintf(NULL,0,"%02d",i),"%d",i);
		ppRank = malloc((strlen(rank)+strlen(rankStr)+1)*sizeof(char));
		strcpy(ppRank,rank); strcat(ppRank,rankStr); strcat(ppRank,space);
		ppPort = malloc((strlen(port1)+strlen(hostName)+strlen(port2)+1)*sizeof(char));
		strcpy(ppPort,port1); strcat(ppPort,hostName);
		strcat(ppPort,port2); strcat(ppPort,space);
		sshArgLen = strlen(arg1)+strlen(ppRank)+strlen(ppPort)+
					strlen(ppSize)+strlen(bashCmd)+strlen(end);
		sshArg = malloc((sshArgLen)*sizeof(char));
		strcpy(sshArg,arg1); strcat(sshArg,ppRank); strcat(sshArg,ppSize);
		strcat(sshArg,ppPort); strcat(sshArg,bashCmd); strcat(sshArg,end);
		printf("Command string: %s %s %s\n",ssh,sshHost,sshArg);

		if (rc=fork() == 0) execlp(ssh,ssh,sshHost,sshArg,NULL);

		i++;j++;
		if (i==hostRanks) i=0; // if n > hostRanks
		free(ppRank); free(ppPort); free(sshArg); free(rankStr);
	}
	p = wait(&status);  // TODO how to fix advancing to next prompt before all ranks come back
	free(ppSize); free(sshHost); free(sizeStr); free(hostFile);

	return 0;
}

	// ssh c01 "bash -c 'FOO=BAR env'"
	// mpi_init(&argc,&argv);  // strip away the unneeded args

	// env variables in ranks
	// PP_MPI_RANK		# rank of process  === order started
	// PP_MPI_SIZE		# number of ranks started === n
	// PP_MPI_HOST_PORT	# === hostname:9999 ; hostname is host where p1 is run

