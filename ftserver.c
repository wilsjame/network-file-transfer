/*********************************************************************
** Author: James G Wilson
** Date: 5/29/2018
** Description: ftserver.c
** 		A simple file transfer server written in C. To be used
		with the Python program, ftclient. 
*********************************************************************/
#include <stdlib.h>
#include <stdio.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

void printUsage();
int handleRequest(int sockfd, int* data_port);
void receivePayload(int sockfd, char* buffer, size_t size);
int startup(char* server_port);

int main(int argc, char* argv[])
{
	int server_sockfd;
	int controlConnection_sockfd;
	pid_t spawnPid;
	
	// Check for incorrect command line usage.
	if(argc != 2)
	{
		printUsage();
		exit(1);
	}

	server_sockfd = startup(argv[1]);
	printf("Server open on %s\n", argv[1]);
	
	// Begin queuing connection requests until limit is reached.
	// It can receive up to 5 connections.
	listen(server_sockfd, 5); 

	// Loop to accept clients indefinitely.
	while(1)
	{

		// Accept a connection, blocking until one is available.
		// accept() generates a new socket to be used for the actual connection.
		printf("trace1\n");
		controlConnection_sockfd = accept(server_sockfd, NULL, NULL);
		if(controlConnection_sockfd < 0) perror("ERROR on accept!\n"); 

		// Fork off child to complete the transaction with the client using
		// the newly generated socket from accept().
		spawnPid = fork();

		switch(spawnPid)
		{
			case -1:
				perror("fork() failure!\n"); exit(1);
			
			case 0: 	// In child. 
				printf("Contact made! In child process!\n");

				int command;
				int data_port;
				int dataConnection_sockfd;

				handleRequest(controlConnection_sockfd, &data_port);

				break;
				
		}

	}

	return 0;

}

// Print command line argument format.
void printUsage()
{ 
	printf("Usage ./chatclient <SERVER_PORT>\n");
}

// Handles the request from the client.
int handleRequest(int sockfd, int* data_port)
{
	char buffer[100]; memset(buffer, '\0', sizeof(buffer));
	char client_command[3]; memset(client_command, '\0', sizeof(client_command));

	// Receive entire message from client
	receivePayload(sockfd, buffer, sizeof(buffer));

	// *****************LEFT OFF HERE*********************
	printf("Request received from client: %s\n", buffer);

}

// Read client's message, up to size bytes, from the socket.
void receivePayload(int sockfd, char* buffer, size_t size)
{
	char tempBuffer[1]; memset(buffer, '\0', sizeof(tempBuffer));
	int bytes_read;

	do
	{
		bytes_read = recv(sockfd, tempBuffer, sizeof(tempBuffer), 0); 
		strcat(buffer, tempBuffer);

	}while(bytes_read > 0);

	buffer[strlen(buffer)-1] = '\0';
}

// Does all the dirty work to start up the server on the given port.
// Returns a succesful socket file descriptor.
int startup(char* server_port)
{
	struct addrinfo hints;		// Fill out with relevent info
	struct addrinfo *result, *rp;	// Will point to results
	int sockfd, status;
	int yes = 1;

	memset(&hints, 0, sizeof(hints));	// Make sure the struct is empty
	hints.ai_family = AF_UNSPEC;		// Don't care IPv4 or IPv6
	hints.ai_socktype = SOCK_STREAM;	// TCP stream sockets (woot!)
	hints.ai_flags = AI_PASSIVE;		// Fill in my IP for me
	hints.ai_protocol = 0;			// Any protocol
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	
	// Set up the address struct for this process (the server)
	if((status = getaddrinfo(NULL, server_port, &hints, &result)) != 0)
	{
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(EXIT_FAILURE);
	}

	// getaddrinfo returns a linked-list of address structures, rp.
	// Try each address until a succesful bind().
	// If socket() (or bind()) fails, close the socket 
	// and try the next address.
	for(rp = result; rp != NULL; rp = rp->ai_next)
	{
		sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol); // Create socket

		if(sockfd == -1);
		{
			fprintf(stderr, "Server: socket error\n");
		}

		if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		{
			fprintf(stderr, "Server: setsockopt error\n");
			exit(EXIT_FAILURE);
		}

		if(bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) // Bind socket to port #
		{
			printf("bind success!\n");
			break; // Success!
		}
		else
		{
			fprintf(stderr, "Server: bind error\n");
		}

		close(sockfd);
	}

	if(rp == NULL) // No address succeeded
	{
		fprintf(stderr, "Server: failed to bind\n");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(result); // Free the linked-list, no longer needed
	
	return sockfd;
	
}

