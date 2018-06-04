INSTRUCTIONS:

< > - Required parameter.
[ ] - Optional parameter.

1) Compile ftserver.c.
	$ make

2) Give ftclient executable permission.
	$ chmod +x ftclient
	
3) Run ftserver.
	$ ftserver <SERVER_PORT>
	
4) Run ftclient.
	$ ./ftclient <SERVER_HOST> <SERVER_PORT> <COMMAND> [FILENAME] <DATA_PORT> 

5) Choose client command: "-l" or "-g".

	5a) "-l" lists the server's directory. Do not include a [FILENAME].
	
	5B) "-g" gets the [FILENAME] from the server and copies it to the client's current directory. 
	
6) To send more commands from the client repeat steps 4 and 5.

9) Terminate the server by sending it SIGINT with
   CTRL+C. 


CITED SOURCES:

I couldn't have made this without the aid from
these sources, check them out!

- https://beej.us/guide/bgnet/
- https://docs.python.org/3/library/socket.html
- https://wiki.python.org/moin/TcpCommunication
- gettaddrinfo(3) man page example
- https://github.com/gregmankes/cs372-project2/blob/master
- https://github.com/mustang25/CS372/blob/master/Project2
- https://github.com/joneric/CS-372/blob/master/Project-2


NOTES:

This program was tested on Oregon State University's
Linux engineering server: flip1.engr.oregonstate.edu

