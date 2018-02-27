#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

const int MAXSIZE = 256;

void error(const char *msg, int exitValue) { fprintf(stderr,"%s\n",msg); fflush(stderr); exit(exitValue); } // Error function used for reporting issues

int loanumCypherileAndGetCount(char const* path, char* storage[]){
	long length; char curChar; int n = 0;
	FILE* fp = fopen (path, "r");

	if(fp){
		fseek(fp, 0, SEEK_END);
		length = ftell(fp); //length counting newlines
		fseek(fp, 0, SEEK_SET);
		*storage = (char*)malloc((length+1)*sizeof(char)); //+1 for \0
		while((curChar = fgetc(fp)) != EOF){
	        	(*storage)[n++] = curChar;
		}
		fclose(fp);
	}
	return length - 1; //return length -1 to subtract that ending \n (NOTE: assumes only newline is that at the end of a file)
}

int verifyAndExtractFiles(char* cypher[],char* key[], char* textFile, char* keyFile){
	int numCypher = -5, numKey = -5;
	numCypher = loanumCypherileAndGetCount(textFile, cypher); //get the char count in textFile and load cypher
	numKey = loanumCypherileAndGetCount(keyFile, key); //get the char count in keyFile and load key
	if(numKey < numCypher){
		if(*cypher) free(*cypher);
		if(*key) free(*key);
		fprintf(stderr, "USAGE: The size of the key is less than the size of plain text\n");
		exit(1);
	}

	return numCypher;
}

int setUpClient(char* port){
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	int portNumber,socketFD;
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(port); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname("localhost"); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "DEC CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("DEC CLIENT: ERROR opening socket",1);
	
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		error("DEC CLIENT: ERROR connecting, cannot connect to otp_dec_d on given port",2);
	
	return socketFD;
}

void sendAll(int socket, char* buffer, int length){
	int charsProcessed = 0, i = 0;
    for(i = 0; i < length; i+= charsProcessed){ //for the number of expected chars keep sending until all are sent
        charsProcessed = send(socket, buffer + i, length,0);
        if (charsProcessed < 1){ if(buffer) free(buffer); error("DEC CLIENT: ERROR writing to socket",1);}
    }
}

void recvAll(int socket, char* buffer, int length){
	int charsProcessed = 0, i = 0;
    for(i = 0; i < length; i+= charsProcessed){ //for the number of expected chars keep recieving until all are sent
        charsProcessed = recv(socket, buffer + i, length,0);
        if (charsProcessed < 1){ if(buffer) free(buffer); error("DEC CLIENT: ERROR recieving form socket",1);}
    }
}

int main(int argc, char* argv[]){
	// argv[0]: prog Name - argv[1]: the file the cypher resides in - argv[2]: the file the key resides - argv[3]: the port number

	char* cypher = NULL;
	char* key = NULL;
	char* plainT = NULL;
	char numChars[MAXSIZE]; memset(numChars,'\0',MAXSIZE);
	int numCypher = -5,socketFD,charsWritten = 0, charsRead = 0, charsProcessed = 0;
	
	if (argc < 4) { fprintf(stderr,"USAGE: %s opt_enc cypherext key port \n", argv[0]); exit(1);} // Check usage & args

	numCypher = verifyAndExtractFiles(&cypher,&key,argv[1],argv[2]); //make sure len(key) >= len(cypher)
	//the cypher and key are now verified and obtained
	socketFD = setUpClient(argv[3]); // Set up the server address struct
	sendAll(socketFD,"JFisch_DEC",10); //let otp_enc_d know it's otp_enc
	snprintf(numChars, MAXSIZE, "%d", numCypher); // turns number of chars in cypher to a string to send to server
	sendAll(socketFD, numChars, MAXSIZE); // Write to the server

	sendAll(socketFD, cypher, numCypher); //send the cypher over to server
	sendAll(socketFD, key, numCypher); //send the key over to the server
	
	plainT = (char*)malloc((numCypher+1)*sizeof(char));
	memset(plainT,'\0',numCypher + 1);
	recvAll(socketFD,plainT,numCypher); // Get return plainT from server

	printf("%s\n",plainT); fflush(stdout);
	close(socketFD); // Close the socket
	
	if(cypher) free(cypher);
	if(key) free(key);
	if(plainT) free(plainT);

	return 0;
}