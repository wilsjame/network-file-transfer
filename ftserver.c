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
void sendNumber(int sockfd, int number);
void sendMessage(int sockfd, char* buffer);
char* getFile(char* file_name);
void sendFile(int sockfd, char* file_name);

int main(int argc, char* argv[])
{
	int connection_server_sockfd;
	int controlConnection_sockfd;
	pid_t spawnPid;
	
	// Validate parameters
	if(argc != 2)
	{
		printUsage();
		exit(1);
	}

	connection_server_sockfd = startup(argv[1]);
	printf("Server open on %s\n", argv[1]);
	
	// Begin queuing connection requests until limit is reached.
	// It can receive up to 5 connections.
	listen(connection_server_sockfd, 5); 

	// Loop to accept clients indefinitely.
	while(1)
	{

		// Accept a connection, blocking until one is available.
		// accept() generates a new socket to be used for the actual connection.
		controlConnection_sockfd = accept(connection_server_sockfd, NULL, NULL);
		if(controlConnection_sockfd < 0) perror("ERROR on control connection accept!\n"); 

		// Fork off child to complete the transaction with the client using
		// the newly generated socket from accept().
		spawnPid = fork();

		switch(spawnPid)
		{
			case -1:
				perror("fork() failure!\n"); exit(1);
			
			case 0: ; 	// In child. 

				int data_port;
				int command;
				int data_server_sockfd;
				int dataConnection_sockfd;


				printf("Begin transaction with client! *In child process*\n");
				printf("*************************************************\n");

				// Accept and interpret command from client.
				command = handleRequest(controlConnection_sockfd, &data_port);

				// Check what command the client sent and act accordingly.
				if(command == 1) // Directory contents requested.
				{
					
					// "-l" get the directory contents.
					printf("List directory requested on port %d\n", data_port);

					char* contents[256] = {NULL};
					int total_length = 0;
					int i = 0;

					// Set up data connection socket.
					// Convert data_port from int to a string.
					char data_port_str[50]; memset(data_port_str, '\0', sizeof(data_port_str));
					sprintf(data_port_str, "%d", data_port);

					// Create server for the data connection.
					data_server_sockfd = startup(data_port_str);

					// Block until accepts gets a client request.
					listen(data_server_sockfd, 1); 
					dataConnection_sockfd = accept(data_server_sockfd, NULL, NULL);
					if(dataConnection_sockfd < 0) perror("ERROR on data connection accept!\n"); 

					// Send directory to client by first sending the length
					// and then the actual contents.
					printf("Sending directory contents to client on port: %d\n", data_port);
					total_length = getDirectory(contents);
					//printf("sending file name: %s\n", contents[i]);

					sendNumber(dataConnection_sockfd, total_length);
					while(contents[i] != NULL)
					{
						//printf("sending file name: %s\n", contents[i]);
						sendMessage(dataConnection_sockfd, contents[i]);
						i++;
					}

					// Don't leave open sockets! 
					close(data_server_sockfd);
					close(dataConnection_sockfd);

					// Exit the child process.
					exit(0);
					
				}
				else if(command == 2) // File requested.
				{

					printf("File requested on port %d\n", data_port);

					int file_name_length;
					char file_name[256]; memset(file_name, '\0', sizeof(file_name));
					
					// Receive the length of the filename the client is requesting.
					file_name_length = receiveNumber(controlConnection_sockfd);

					// Receive the filename now.
					receiveMessage(controlConnection_sockfd, file_name, file_name_length);

					// Debug
					//printf("Length of filename requested: %d.\n", file_name_length);
					//printf("Filename requested: %s.\n", file_name);

					// Now check if the file exists in the current directory.
					char* contents[256] = {NULL};
					//int total_length = 0;
					int i = 0;
					int match_found = 0;
					getDirectory(contents);

					while(contents[i] != NULL)
					{

						// Debug
						//printf("Comparing file #%d: %s with %s\n", i, contents[i], file_name);
						
						if(strcmp(file_name, contents[i]) == 0)
						{

							printf("File found!\n");
							match_found = 1;
						}
						
						i++;
					}

					// Take action depending on whether a match was found or not.
					if(match_found == 1)
					{
						sendNumber(controlConnection_sockfd, (int)strlen("File found!"));
						sendMessage(controlConnection_sockfd, "File found!");

						// Set up data connection socket.
						// Convert data_port from int to a string.
						char data_port_str[50]; memset(data_port_str, '\0', sizeof(data_port_str));
						sprintf(data_port_str, "%d", data_port);

						// Create server for the data connection.
						data_server_sockfd = startup(data_port_str);

						// Block until accepts gets a client request.
						listen(data_server_sockfd, 1); 
						dataConnection_sockfd = accept(data_server_sockfd, NULL, NULL);
						if(dataConnection_sockfd < 0) perror("ERROR on data connection accept!\n"); 

						// Send the found requested file contents.
						sendFile(dataConnection_sockfd, file_name);

						// Don't leave open sockets! 
						close(data_server_sockfd);
						close(dataConnection_sockfd);

					}
					else
					{
						printf("File not found!\n");
						sendNumber(controlConnection_sockfd, (int)strlen("File not found!"));
						sendMessage(controlConnection_sockfd, "File not found!");
					}

					// Exit the child process.
					exit(0);

				}
				else if(command == 0)
				{
					perror("ERROR invalid command!");
				}
				
				//break;
				exit(0); 
				
		}

	}

	return 0;

}

// Print command line argument format.
void printUsage()
{ 
	printf("Usage ./chatclient <SERVER_PORT>\n");
}

// Sends a string (message) to the client.
void sendMessage(int sockfd, char* buffer)
{
	ssize_t n;
	size_t message_length = strlen(buffer) + 1;
	size_t running_total = 0;

	// Send the entire message.
	while(running_total < message_length)
	{
		n = write(sockfd, buffer, message_length - running_total);
		running_total += n;

		// Debug
		//printf("bytes sent: %d\n", running_total);

		if(n < 0)
		{
			perror("ERROR sending message\n");
		}
		else if(n == 0)
		{
			running_total = message_length - running_total;
		}

	}

}

// Sends an integer value to the client. Primarly used to send the length of 
// the upcoming message.
void sendNumber(int sockfd, int number)
{
	ssize_t n = 0;

	n = write(sockfd, &number, sizeof(int));

	if(n < 0) perror("ERROR receiving number\n");

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

// Sends the contents of a requested file in the current directory to the client.
void sendFile(int sockfd, char* file_name)
{
	char* file_contents;
	file_contents = getFile(file_name);
	printf("length of file contents %zu", strlen(file_contents));


	sendNumber(sockfd, strlen(file_contents));
	sendMessage(sockfd, file_contents);
}

// Gets the contents of the requested file and stores them in a string.
char* getFile(char* file_name)
{

	char* file_contents = NULL;

	FILE* fp = fopen(file_name, "r");

	if(fp == NULL) 
	{
		perror("ERROR opening file");
	}

	if(fp != NULL) // The file opened succesfully.
	{

		// Start reading the file.
		if(fseek(fp, 0L, SEEK_END) == 0)
		{

			// Obtain the current value of the file pointer (position).
			long buffer_size = ftell(fp);

			if(buffer_size == -1) // Error checking.
			{
				perror("ERROR invalid file pointer\n");
			}

			file_contents = malloc(sizeof(char) * (buffer_size + 1));

			if(fseek(fp, 0L, SEEK_SET) != 0) // Error checking.
			{
				perror("ERROR not reading from beginning of file\n.");
			}

			// Okay, now begin reading the file.
			size_t new_length = fread(file_contents, sizeof(char), buffer_size, fp);

			if(ferror(fp) != 0)
			{
				perror("ERROR while reading file!\n");
			}
			else
			{
				file_contents[new_length++] = '\0';
			}

		}

	}

	// Don't leave poen file streams.
	fclose(fp);

	return file_contents;

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

		return 1;

	}
	else if(strcmp(command, "-g") == 0)
	{

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

	if(n < 0) perror("ERROR receiving number\n");

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
			//Debug
			//fprintf(stderr, "Server: socket error\n");
		}

		if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		{
			fprintf(stderr, "Server: setsockopt error\n");
			exit(EXIT_FAILURE);
		}

		if(bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) // Bind socket to port #
		{

			//Debug
			//printf("bind success!\n");
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

