#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>

// TODO: Add support for multiple child processes
struct children {
    int *children_pids;
    int n;
};

struct children* initChildren();
void clearChildren(struct children*);
void addChild(struct children*, int);
void removeChild(struct children*, int);
int childrenLeft(struct children*);
int getLastChild(struct children*);
void popLastChild(struct children*);
void waitChildren(struct children*);

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues

// Prints a usage statement
void print_usage() {
    printf("Usage:\notp_enc_d port\n");
	fflush(stdout);
    printf("port is an int on which to listen on\n");
	fflush(stdout);
}

// Initializes a children struct
// Allocates space for 6 integers in children_pids array and sets
// n to 0
struct children* initChildren() {
    // Maximum of 5 children
    // Using last index as a garbage can
    int* arr = malloc(sizeof(int) * 6);
    struct children* childrenStruct = malloc(sizeof(struct children));
    childrenStruct->children_pids = arr;

    // n attribute represents the number of children in the children_pids array
    childrenStruct->n = 0;
    return childrenStruct;
}

// Frees memory allocated for struct
void clearChildren(struct children* children) {
    free(children->children_pids);
    free(children);
    return;
}

// Adds the given pid to the children_pids array
void addChild(struct children* children, int child_pid) {
    // Only adds new value if there is space left in the array
    if (children->n < 5) {
        children->children_pids[children->n] = child_pid;
        children->n++;
    }
    return;
}

// Removes the given pid from the children_pids array
void removeChild(struct children* children, int child_pid) {
    if (children->n == 0) {
        return;
    }

    // Find the index of the array in which the pid canbe found
    int i;
    int found = 0;
    for (i = 0; i < children->n; i++) {
        if (children->children_pids[i] == child_pid) {
            found = 1;
            break;
        }
    }

    // Remove the given pid from the children_pids array, and shift following
    // elements up
    if (found == 1) {
        children->children_pids[5] = children->children_pids[i];
        int j;
        for (j = i; j < children->n; j++) {
            children->children_pids[j] = children->children_pids[j + 1];
        }
        children->n--;
    }
}

// Returns the number of children left
int childrenLeft(struct children* children) {
    return children->n;
}

// Gets the value of the last child's pid
int getLastChild(struct children* children) {
    return children->children_pids[children->n-1];
}

// Removes the last child's pid
void popLastChild(struct children* children) {
    children->n--;
}

void waitChildren(struct children* children) {
    int i;
	int lastChild = children->n;
	int childExitMethod;
	if (children->n == 0) {
		return;
	}
    for (i = lastChild - 1; i >= 0; i--) {
		pid_t childEndedPID = waitpid(children->children_pids[i], &childExitMethod, WNOHANG);
		if (childEndedPID == 0) {
            continue;
        }
		else {
			removeChild(children, children->children_pids[i]);
		}
    }
}

void encrypt_key(char* plainText, char* key, int lenOfText) {
	int i;
	for (i = 0; i < lenOfText; i++) {
		if (plainText[i] == 32) {
			plainText[i] = 91;
		}
		if (key[i] == 32) {
			plainText[i] = 91;
		}
		int curKey = (plainText[i] - 65) + (key[i] - 65);
		curKey = curKey % 27;
		if (curKey == 26) {
			curKey = 32;
		}
		else {
			curKey += 65;
		}
		plainText[i] = curKey;
	}
}

int main(int argc, char *argv[]) {
	int listenSocketFD, establishedConnectionFD, portNumber;
	socklen_t sizeOfClientInfo;
	struct sockaddr_in serverAddress, clientAddress;
	ssize_t charsWritten, charsRead;

	struct children* children = initChildren();

	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

	// Check that parameter given is a valid integer
	int i;
	for (i = 0; i < strlen(argv[1]); i++) {
        if (!isdigit(argv[1][i])) {
            fprintf(stderr, "%s", "Error: parameter should be an integer\n");
			fflush(stdout);
            print_usage();
            exit(1);
        }
    }

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the listen socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) {
		fprintf(stderr, "%s", "Error: Unable to open listen socket\n");
		fflush(stdout);
		exit(1);
	}

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
		fprintf(stderr, "%s", "Error: Unable to bind listen socket\n");
		fflush(stdout);
		exit(1);
	}
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections

	while(1) {
		waitChildren(children);
		pid_t pid = -5;

		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
		if (establishedConnectionFD < 0) {
			fprintf(stderr, "%s", "Error: Unable to accept connection\n");
			exit(1);
		}
		// Once connection is available, unblock
		pid = fork();
		if (pid < 0) {
			fprintf(stderr, "Error: Unable to fork() child process\n");
			exit(1);
		}
		// If this is the child process, begin communication with encryption client
		if (pid == 0) {
			// Send daemon type to client
			char daemonType[2];
			daemonType[0] = 'e';
			daemonType[1] = '\0';
			charsWritten = send(establishedConnectionFD, daemonType, 2, 0);
			if (charsWritten < 0) {
				fprintf(stderr, "Error: Unable to write to socket\n");
				exit(1);
			}

			// Receive client type (encryption)
			char clientType[2];
			memset(clientType, '\0', sizeof(clientType));
			charsRead = recv(establishedConnectionFD, clientType, 2, 0);
			if (charsRead < 0) {
				fprintf(stderr, "%s", "Error: unable to read from socket");
				exit(1);
			}

			// If connected to the incorrect client, need to exit and close socket
			if (strcmp(clientType, "e") != 0) {
				close(establishedConnectionFD);
				exit(1);
			}

			// Get the size of the plaintext buffer
			char bufferSizeChar[8];
			memset(bufferSizeChar, '\0', 8);
			charsRead = recv(establishedConnectionFD, bufferSizeChar, 8, 0);
			if (charsRead < 0) {
				fprintf(stderr, "%s", "Error: unable to read from socket");
				exit(1);
			}

			// Number of characters to encrypt
			int plaintextBufferSize = atoi(bufferSizeChar);

			char *textToEncrypt = malloc(sizeof(char) * (plaintextBufferSize + 1));
			memset(textToEncrypt, '\0', plaintextBufferSize + 1);
			charsRead = recv(establishedConnectionFD, textToEncrypt, plaintextBufferSize, 0);
			if (charsRead < 0) {
				fprintf(stderr, "%s", "Error: unable to read from socket");
				free(textToEncrypt);
				exit(1);
			}

			char *keyText = malloc(sizeof(char) * (plaintextBufferSize + 1));
			memset(keyText, '\0', plaintextBufferSize + 1);
			charsRead = recv(establishedConnectionFD, keyText, plaintextBufferSize, 0);
			if (charsRead < 0) {
				fprintf(stderr, "%s", "Error: unable to read from socket");
				free(textToEncrypt);
				free(keyText);
				exit(1);
			}
			fflush(stdout);

			encrypt_key(textToEncrypt, keyText, plaintextBufferSize);

			// Send encrypted text back to client
			charsWritten = send(establishedConnectionFD, textToEncrypt, plaintextBufferSize, 0);
			if (charsWritten < 0) {
				fprintf(stderr, "Error: Unable to write to socket\n");
				free(textToEncrypt);
				free(keyText);
				exit(1);
			}

			// Terminate child process
			close(establishedConnectionFD);
			close(listenSocketFD);
			free(textToEncrypt);
			free(keyText);
			exit(0);
		}

		// Once child process terminates, close the socket
		else {
			close(establishedConnectionFD);
			int childExitMethod;

			// Call wait to reap process if terminated
			pid_t childEndedPID = waitpid(pid, &childExitMethod, WNOHANG);

			// If child has not terminated, add child to struct
			// Check to see if it has completed later
			if (childEndedPID == 0) {
            	addChild(children, (int)pid);
        	}
		}
	}
	clearChildren(children);
	close(listenSocketFD); // Close the listening socket
	return 0;
}
