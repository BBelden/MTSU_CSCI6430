#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>

#define DEFAULT_BACKLOG	5

int setup_to_accept(int port); 
int accept_connection(int accept_socket); 
void send_msg(int fd, char *buf, int size);
int recv_msg(int fd, char *buf); 
void error_check(int val, char *str);

#define i_to_n(n) (int) htonl( (u_long) n)
#define n_to_i(n) (int) ntohl( (u_long) n)


int main(int argc, char *argv[])
{

	int i, j, rc, status;
	int numRanks = 1;
	int hostPort = 9999;
	int numHosts = 0;
	int hostToUse = 0; 
	char *hostFile = "hostnames";
	char hosts[100][256]; 
	char progToRun[256];
	char beginningStr[100];
	char endStr[100]; 
	char numRanksStr[15]; 
	char myRankStr[15];
	char myHostStr[15]; 
	FILE *fp;
	
	if (argc <= 1)
	{
		printf("No executable provided\n"); 
		exit(-1); 
	}

	for (i = 1; i < argc; i++)
	{
		if (strcmp(argv[i], "-f") == 0)
		{
			i++;	 
			hostFile = argv[i];
		}
		else if (strcmp(argv[i], "-n") == 0)
		{
			i++;
			numRanks = atoi(argv[i]); 
		}
		else
		{
			strcpy(endStr, ""); 
			strcat(endStr, argv[i]);
			i++;
			for ( ; i < argc; i++)
			{
				strcat(endStr, " ");
				strcat(endStr, argv[i]); 		
			}
			strcat(endStr, "'\""); 

		}

	}

	fp = fopen(hostFile, "r");
	if (fp)
	{
		char line[256];
		i = 0; 
		while (fgets(line, sizeof(line), fp))
		{
			line[strcspn(line, "\n")] = 0; 
			strcpy(hosts[i], line); 
			i++; 
			numHosts++;
			 
		}	
		fclose(fp);
	}
	else
	{
		strcpy(hosts[0], "c00");
		numHosts = 1; 
	}

	int accept_socket, to_client_socket;
	accept_socket = setup_to_accept(5025);

	j = 0; 
	for (i = 0; i < numRanks; i++)
	{
		if ((rc = fork()) == -1)
		{
			printf("Fork failed\n\n");
			exit(-1);
		}
		else if (rc == 0)
		{
			//printf("Starting process with pid %d\n", getpid());
			char* cwd;
			char buf[1024]; 
			cwd = getcwd(buf, 1024); 
			//printf("Getting cwd of %s\n", cwd); 
			strcpy(beginningStr, "\"bash -c 'PWD="); 
			strcat(beginningStr, cwd); 
			strcat(beginningStr, " ; cd \\$PWD ; PP_MPI_RANK=");
			sprintf(myRankStr, "%d", i);
			strcat(beginningStr, myRankStr);
			strcat(beginningStr, " PP_MPI_SIZE="); 
			sprintf(numRanksStr, "%d", numRanks);
			strcat(beginningStr, numRanksStr);
			strcat(beginningStr, " PP_MPI_HOST_PORT=");
			
			myHostStr[14] = '\0'; 
			gethostname(myHostStr, 14); 

			strcat(beginningStr, myHostStr);
			strcat(beginningStr, ":5025 "); 

			strcpy(progToRun, beginningStr);
			strcat(progToRun, endStr); 
			
			//printf("Running: %s\n", progToRun); 

			char *testArgs[] = {"ssh", hosts[j], "bash", "-c", progToRun, NULL}; 
			execvp("ssh",testArgs);
		
			exit(0);
		}
		else
		{
			
		}

		
		j++;
		if (j >= numHosts)
		{
			j = 0; 
		}
	}
	
	for (i = 0; i < numRanks; i++)
	{
		to_client_socket = accept_connection(accept_socket); 
		int sock_rc;
		char buf[1024];
		sock_rc = recv_msg(to_client_socket, buf); 
		while (sock_rc != -1)
		{
			printf("server received: %s\n", buf); 
			send_msg(to_client_socket, "ACK", 4); 
			
			sock_rc = recv_msg(to_client_socket, buf); 
		}

		wait(&status); 	
	}
	

	return 0; 
}

setup_to_accept(int port)	
{
    int rc, accept_socket;
    int optval = 1;
    struct sockaddr_in sin, from;

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);

    accept_socket = socket(AF_INET, SOCK_STREAM, 0);
    error_check(accept_socket,"setup_to_accept socket");

    setsockopt(accept_socket,SOL_SOCKET,SO_REUSEADDR,(char *)&optval,sizeof(optval));

    rc = bind(accept_socket, (struct sockaddr *)&sin ,sizeof(sin));
    error_check(rc,"setup_to_accept bind");

    rc = listen(accept_socket, DEFAULT_BACKLOG);
    error_check(rc,"setup_to_accept listen");

    return(accept_socket);
}

int accept_connection(int accept_socket)
{
	struct sockaddr_in from; 
	int fromlen, to_client_socket, gotit;
	int optval = 1;

	fromlen = sizeof(from);
	gotit = 0; 
	while(!gotit)
	{
		to_client_socket = accept(accept_socket, (struct sockaddr *) &from, &fromlen);
		if (to_client_socket == -1)
		{
			if (errno == EINTR)
			{
				continue;
			}
			else
			{
				error_check(to_client_socket, "accept_connection accept"); 
			}
		}
		else
		{
			gotit = 1; 
		}
	}

	setsockopt(to_client_socket, IPPROTO_TCP, TCP_NODELAY, (char *) &optval, sizeof(optval)); 
	return (to_client_socket); 
}

void send_msg(int fd, char *buf, int size)
{
	int n; 
	n = write(fd, buf, size); 
	error_check(n, "send_msg write"); 
}

int recv_msg(int fd, char *buf)
{
	int bytes_read; 

	bytes_read = read(fd, buf, 1024); 
	error_check(bytes_read, "recv_msg read"); 
	if (bytes_read == 0)
	{
		return(-1); 
	}
	return bytes_read; 

}

void error_check(int val, char *str)
{
	if (val < 0)
	{
		printf("%s :%d %s\n", str, val, strerror(errno)); 
		exit(1); 
	}
}

