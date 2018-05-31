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
#include <dirent.h>

void printUsage();
int getDirectory(char* contents[]);
int handleRequest(int sockfd, int* data_port);
void receiveMessage(int sockfd, char* buffer, size_t size);
int receiveNumber(int sockfd);
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
				printf("Connection made with client! In child process!\n");

				//int dataConnection_sockfd;

				// Accept and interpret command from client.
				int data_port;
				int command;
				command = handleRequest(controlConnection_sockfd, &data_port);

				// Check what command the client sent and act accordingly.
				if(command == 1)
				{
					printf("List directory requested on port %d\n", data_port);
				}
				else if(command == 2)
				{
					printf("File requested.\n");
				}
				else if(command == 0)
				{
					perror("ERROR invalid command!");
				}
				

				
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

// Gets the contents of a directory and stores them in the parameter passed.
// Returns the length in bytes of the sum of the regular file names.
int getDirectory(char* contents[])
{
	DIR *dirp;
	struct dirent *entry;
	int total_length = 0;
	int number_of_files = 0;
	int i = 0;

	// Open the current directory.
	dirp = opendir(".");

	if(dirp != NULL)
	{

		// Begin getting directory contents.
		while((entry = readdir(dirp)) != NULL)
		{
			
			// Only care about regular files.
			if(entry->d_type == DT_REG)
			{

				// Store the name of the files in the array of strings
				// and add its byte length to the total length.
				contents[i] = entry->d_name;
				total_length += strlen(contents[i]);
				i++;
			}

		}

		number_of_files = i - 1;
	}

	closedir(dirp);

	return number_of_files + total_length;
		
}

// Handles the request from the client by parsing the message.
// Returns 1 for "-l", 2 for "-g", and 0 otherwise. 
int handleRequest(int sockfd, int* data_port)
{
	char command[3] = "\0";

	// Get command and port number from clients message.
	receiveMessage(sockfd, command, 3);
	*data_port = receiveNumber(sockfd);

	if(strcmp(command, "-l") == 0)
	{
		printf("trace -l\n");

		return 1;

	}
	else if(strcmp(command, "-g") == 0)
	{
		printf("trace -g\n");

		return 2;

	}
	else
	{

		// Invalid argument!
		return 0;

	}
											
}

// Read client's message from the socket  
// and stores it in payload.
void receiveMessage(int sockfd, char* payload, size_t size)
{
	char buffer[size + 1]; memset(buffer, '\0', sizeof(buffer));
	ssize_t n;
	size_t total = 0;

	while(total < size)
	{
		n = read(sockfd, buffer + total, size - total);
		total += n;

		if( n < 0) perror("ERROR receiving message\n");
	}

	strncpy(payload, buffer, size);

}

// Read client's message that is known to contain a number.
int receiveNumber(int sockfd)
{
	int number;
	ssize_t n = 0;

	n = read(sockfd, &number, sizeof(int));

	if( n < 0) perror("ERROR receiving number\n");

	return number;

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

