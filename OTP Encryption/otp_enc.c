#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

const int MAXSIZE = 500;

void error(const char *msg, int exitValue) { fprintf(stderr,"%s\n",msg); fflush(stderr); exit(exitValue); } // Error function used for reporting issues

int loadFileAndGetCount(char const* path, char* storage[]){
	long length; char curChar; int n = 0;
	FILE* fp = fopen (path, "r");

	if(fp){
		fseek(fp, 0, SEEK_END);
		length = ftell(fp); //length counting newlines
		fseek(fp, 0, SEEK_SET); //seek to end of file
		*storage = (char*)malloc((length+1)*sizeof(char)); //+1 for \0
		length = 0; //reset length to be recounted for no newlines
		while((curChar = fgetc(fp)) != EOF){
			if(curChar == ' ' || (curChar >='A' &&  curChar <= 'Z') ){ 	//if it is A-Z or space then count it (NOTE \n isnt counted)
	        	(*storage)[n++] = curChar;
				++length;
			}
			else if(curChar != '\n'){ //not in range and not a \n then error
				if(*storage) free(*storage);
				error("ENC CLIENT: ERROR file has invalid characters",1);
			}
   	 	}
		fclose(fp);
	}
	(*storage)[length] = '\0';

	return length;
}

int verifyAndExtractFiles(char* plainT[],char* key[], char* textFile, char* keyFile){
	int numPlainT = -5, numKey = -5;
	numPlainT = loadFileAndGetCount(textFile, plainT); //get the char count in textFile and load plainT
	numKey = loadFileAndGetCount(keyFile, key); //get the char count in keyFile and load key
	if(numKey < numPlainT){
		if(*plainT) free(*plainT);
		if(*plainT) free(*key);
		fprintf(stderr, "USAGE: The size of the key is less than the size of plain text\n");
		exit(1);
	}

	return numPlainT;
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
	if (serverHostInfo == NULL) { fprintf(stderr, "ENC CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("ENC CLIENT: ERROR opening socket",1);
	
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		error("ENC CLIENT: ERROR connecting, cannot connect to otp_enc_d on given port",2);
	
	return socketFD;
}

void sendAll(int socket, char* buffer, int length){
	int charsProcessed = 0, i = 0;
    for(i = 0; i < length; i+= charsProcessed){ //for the number of expected chars keep sending until all are sent
        charsProcessed = send(socket, buffer + i, length,0);
        if (charsProcessed < 1){ if(buffer) free(buffer); error("ENC CLIENT: ERROR writing to socket",1); }
    }
}

void recvAll(int socket, char* buffer, int length){
	int charsProcessed = 0, i = 0;
    for(i = 0; i < length; i+= charsProcessed){ //for the number of expected chars keep pulling until all are recieved
        charsProcessed = recv(socket, buffer + i, length,0);
        if (charsProcessed < 1){ if(buffer) free(buffer); error("ENC CLIENT: ERROR recieving form socket",1); }
    }
}

int main(int argc, char* argv[]){
	// argv[0]: prog Name - argv[1]: the file the plaintext resides in - argv[2]: the file the key resides - argv[3]: the port number

	char* plainT = NULL;
	char* key = NULL;
	char* cypher = NULL;
	char numChars[MAXSIZE]; memset(numChars,'\0',MAXSIZE);
	int numPlainT = -5,socketFD,charsWritten = 0, charsRead = 0, charsProcessed = 0;
	
	if (argc < 4) { fprintf(stderr,"USAGE: %s opt_enc plaintext key port \n", argv[0]); exit(1);} // Check usage & args

	numPlainT = verifyAndExtractFiles(&plainT,&key,argv[1],argv[2]);

	//the plainT and key are now verified and obtained
	socketFD = setUpClient(argv[3]); // Set up the server address struct

	sendAll(socketFD,"JFisch_ENC",10); //let otp_enc_d know it's otp_enc
	snprintf(numChars, MAXSIZE, "%d", numPlainT); // turns number of chars in plaintext to a string to send to server
	sendAll(socketFD, numChars, MAXSIZE); // Write to the server

	sendAll(socketFD, plainT, numPlainT);
	sendAll(socketFD, key, numPlainT);
	
	cypher = (char*)malloc((numPlainT+1)*sizeof(char));
	memset(cypher,'\0',numPlainT + 1);
	recvAll(socketFD,cypher,numPlainT); // Get return plainT from server

	printf("%s\n",cypher); fflush(stdout);
	close(socketFD); // Close the socket
	if(plainT) free(plainT);
	if(key) free(key);
	if(cypher) free(cypher);

	return 0;
}