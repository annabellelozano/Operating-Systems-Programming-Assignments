# Operating-Systems-Programming-Assignments
A series of programmign assignments that build upon eachother, more information in the Read Me file

CSCI 4534 Operating Systems

This assignment is meant to:
	•	Introduce students to writing basic client/server programs in C using the UNIX/Linux platform
	•	Provide an opportunity to write code that will become the base for projects similar to that of a simulated operating system component
	•	Extend the server to be able to handle requests from multiple clients simultaneously.
	•	Add the functionality of a mutex synchronization system call (wait and signal)


Objective: Create programs that run independently and can perform simple IPC using FIFOs/named pipes to pass information back and forth in a client/server fashion.

This assignment requires the analysis, implementation, testing and documentation of two small programs written in C on the UHCL Linux server ruby or your own Linux box or virtual machine. A server program and a client program that will be run concurrently.

Server program:
There is no change in the functionality of the server program (as far as the “system calls previously handled”) other than:
	•	Make the server keep a simulated processID counter initialized to 1, increment and return the simulated processID value to each client that initiates a connection with the server. (Clients will need to save their simulated PID to include with each future simulated “system call” made and sent to the server afterwards).
	•	Save the file descriptors for the return fifo for each client in an array/list so that when a request comes in from client X, a proper reply is sent to client X through its own return fifo.

The server program will provide a simple “Echo&store” service to clients that connect with it and send it requests.

Server program will be an iterative server (can process ONLY one client at a time) and it needs to do the following:
	•	Create a well-known receive FIFOs where it will read its input/requests from the client and open it in READ/WRITE mode.
	•	Then go into an infinite loop to read requests from the clients, each request will be a “simulated system call”, each request/system call should include:
	•	Simulated or real Process ID of process sending/making the system call 
	•	System call number (integer or byte)
	•	Number “n” of parameters in the system call (integer or byte)
	•	Size of the rest of the message that includes
	•	Actual value(s) for the “n” parameters indicated in 3
As described below:


	•	System Call 1 – would be the first request sent by a new client (“connect system call”)
	•	Process ID
	•	System Call Number = 1
	•	Number “n” of parameters = 1 
	•	Size of 5
	•	Actual value(s) for the “1” parameter = the name of the client’s specific FIFO which the server should use to reply to that client. Server should open that client-specific FIFO in WRITE mode, save the file descriptor and the pid of the client for use when replies need to be sent to that client.
	•	No need to return anything. Increment the processID counter and return the next available simulated ClientID/processID

	•	System Call 2 – Number 
	•	Process ID
	•	System Call Number = 2
	•	Number “n” of parameters = 1
	•	Size of parameter (4 bytes for integer)
	•	Actual value(s) for the parameter
	•	Return the number received

	•	System Call 3 – Text 
	•	Process ID
	•	System Call Number = 3
	•	Number “n” of parameters = 1
	•	Size of parameter
	•	Actual value(s) for the “1” parameter (a string of characters)
	•	Return the string

	•	System Call 4 – Store
	•	Process ID
	•	System Call Number = 4
	•	Number “n” of parameters = 1
	•	 Size of parameter (integer)
	•	Actual value(s) for the “1” parameter
	•	Return stored value

	•	System Call 5 – Recall
	•	Process ID
	•	System Call Number = 5
	•	Number “n” of parameters = 0 
	•	Actual value(s) for the “1” parameter = N/A
	•	Return stored/recalled value

	•	System Call 0 – Exit
	•	Process ID
	•	System Call Number = 0
	•	Number “n” of parameters = 0 
	•	Actual value(s) for the parameter = N/A
	•	Return value N/A

	•	System Call -1 – Shutdown
	•	Process ID
	•	System Call Number = -1
	•	Number “n” of parameters = 0 
	•	Actual value(s) for the parameter = N/A
	•	Return value N/A

	•	System Call 6 -Wait operation
	•	Process ID
	•	System Call Number = 6
	•	Number “n” of parameters = 0
	•	 Size of parameter (integer) N/A
	•	Actual value(s) for the “1” parameter N/A

	•	System Call 7 -Signal operation
	•	Process ID
	•	System Call Number = 7
	•	Number “n” of parameters = 0
	•	 Size of parameter (integer) N/A
	•	Actual value(s) for the “1” parameter N/A


For System call 6 (Wait) from a connected client requesting access to the mutex. If the mutex is free (unlocked) a reply should be sent immediately to the client indicating the mutex has been locked, the reply would allow the client to continue. If the mutex is not free (i.e. it is locked by another client), the requesting (client) should be put in a wait queue and NO reply should be sent to the client (which will effectively block the client from continuing at this point).

For System call 7 (signal) from a connected client requesting to free the mutex. If the mutex is not locked by this client, the server should respond with a “failure” message. If the mutex IS locked by the requesting client and if no other client is waiting for it, the mutex should be unlocked and a simple ack reply should be sent to the client. If at least one client is waiting for the semaphore, in addition to the ack sent to the client doing the signal, one of the clients waiting for the semaphore should be removed from the wait queue and a reply should be sent to THAT client indicating that it now owns the mutex, effectively unblocking it and allowing it to continue.

Server must print to the screen a message indicating the “system call received”, something like:
Client pid: 1
System Call Requested: 3 with 2 parameters which are:
Param1=xxxx Param2=YYYY Result=XXXX

Server must reply back to the client through the client specific fifo with a reply message that should include a result as appropriate.

If the request is the system call 0 “EXIT”, the server program must close the client specific fifo and continue to receive the next system call (ready for the next client to connect)

When the last client terminates, i.e. sends system call -1, the server should close the well known FIFO, delete it and terminate as well.

Client Program:
The client program will “connect” to the server through the well-known FIFO and send requests through it to the server, obtaining information from the user as to what “system call to make” and the corresponding values for the parameter(s), more specifically, the client program should:
	•	The server will now return the ClientID/processID upon successful connection. This ID will need to be saved and included with all future system calls.
	•	Acquire from the user (through the command line or reading from the keyboard) what the client number this instance of the program will be (i.e., client 1, client 2, etc.) or use the actual processid if you prefer.
	•	Open the well-known server’s fifo in write mode to communicate with the server.
	•	Create the client-specific FIFO using an appropriate name (e.g., “./ClientNfifo”, where N is the client number and send the initial “connect system call” to the server including Client number and name of the client-specific FIFO.
	•	Open the client-specific FIFO in READ mode to be able to read replies from the server. (This will block the client until the server opens the client-specific FIFO in write mode).
	•	After the client specific fifo has been opened (server connected), the client should go into a loop where the client will ask the user what to do next? providing three choices:
	•	1 – Send request to server, in this case it will ask the user for data:
	•	What sytem call?
	•	How many parameters? (user enters 0, 1, 2, 3, etc.)
	•	For each of the “n” parameters indicated above,
Read a value
Take all the information gathered, appropriately format a “system call” request and send it to the server. Request should include:
	•	Process ID
	•	System call number (integer)
	•	Number “n” of parameters in the system call (integer)
	•	Size of the parameter(s) data
	•	Actual value(s) for the “n” parameter(s) indicated above
After sending the request to the server, read the reply from the server in the client-specific FIFO and write it to the screen.

	•	2 – EXIT - indicates THIS client does not want to issue more requests to the server, it should send a “EXIT” system call to the server, close its client specific FIFO, delete it and exit.

	•	3 – SHUTDOWN - indicates THIS client does not want to issue more requests to the server, and is flagging the server to also exit. it should send a “SHUTDOWN” system call to the server, close its client specific FIFO, delete it and exit.


	•	In the client program, add an option in the main menu for the user to select executing a critical section

something like:
{
	// NON – CRITICAL SECTION
An inner loop that will run 20-30 times where the client will continually print a message (maybe every second or so) to the screen indicating that it is processing in its NON-CRITICAL SECTION
After the NON-CRITICAL SECTION LOOP the client should print a message:
	WAITING TO GET INTO THE CRITICAL SECTION
	*Send a Wait system call request to the server (requesting access to the mutex) and wait for a reply
	// CRITICAL SECTION
An inner loop that will also run 20-30 times where the client will continually print a message (maybe every second or so) to the screen indicating that it is processing “inside” its CRITICAL SECTION
	After the CRITICAL SECTION LOOP the client should print a message:
	LEAVING THE CRITICAL SECTION
	*Send a  Signal system call request to the server (freeing the mutex) and wait for a reply
}

You should implement a Wait and Signal functions that are called from inside the block above when appropriate. The Wait function should send the Wait system call request to the server and expect a reply before returning. The Signal function should send a signal request to the server and expect a reply before returning.
	•	Make sure your demo shows clients trying to get into the critical section at the same time and showing only one of them in it at a time.

The first step in writing a client/server application is define the communications protocol between both applications. In other words, how are you going to encode the requests and replies into a message, you can encode data in string forms, separating each piece with a “,” “-“ a space, a new line character, or any other kind of separator, or you can use integer data, strucs, etc.

Create a zip file with both your programs source file(s) and executables, do a screen recording showing your programs working (with multiple clients running one after the other and interacting with the server) and upload to canvas. Alternatively, just upload a zip file of your source files and executables and visit with the TA during office hours so you can do a demo of your client/server system.


