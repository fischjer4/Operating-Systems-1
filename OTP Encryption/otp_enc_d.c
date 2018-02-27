#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

const int MAXSIZE = 500;

void checkZombies(){
    int status = -5;
    pid_t pidReturn = waitpid(-1,&status,WNOHANG);
    while(pidReturn > 0){       //while there continues to be zombies ready to reap, reap them
        pidReturn = waitpid(-1,&status,WNOHANG);
    }
}

void error(const char *msg) { fprintf(stderr,"%s\n",msg); checkZombies();} // Error function used for reporting issues

void encrypt(char message[], char key[]){ //OTP encryption method where there are 26 UPPERCASE chars + 1 space = 27 options
    int i = 0;
    while(message[i] != '\0' && key[i] != '\0'){
        if(message[i] == ' '){
            message[i] = ((27 + (key[i] - 64)) % 27) + 64;
        }else
            message[i] = ( ((message[i] - 64) + (key[i] - 64)) % 27 ) + 64;
        i++;
    }
}
void setupClient(char* port, int* listenSocketFD, struct sockaddr_in* serverAddress){
	int portNumber;

    memset((char *)&(*serverAddress), '\0', sizeof(*serverAddress)); // Clear out the address struct
	portNumber = atoi(port); // Get the port number, convert to an integer from a string
	serverAddress->sin_family = AF_INET; // Create a network-capable socket
	serverAddress->sin_port = htons(portNumber); // Store the port number
	serverAddress->sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	*listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (*listenSocketFD < 0){ error("ENC SERVER: ERROR opening socket"); exit(1);}

	if (bind(*listenSocketFD, (struct sockaddr *)&(*serverAddress), sizeof(*serverAddress)) < 0){ // Connect socket to port
		error("ENC SERVER: ERROR on binding"); exit(1);}
	if(listen(*listenSocketFD, 5) < 0){ error("ENC SERVER: ERROR on listen"); exit(1);}// Flip the socket on - it can now receive up to 5 connections

}

int sendAll(int socket, char buffer[], size_t length){
    char *ptr = (char*) buffer;
	int charsProcessed = 0;
    while (length > 0){ //while we havnt sent all characters keep sending
        charsProcessed = send(socket, ptr, length,0);
        if (charsProcessed < 1) { if(buffer) free(buffer); error("ENC SERVER: ERROR writing to socket"); return -1;}
        ptr += charsProcessed;
        length -= charsProcessed;
    }
    return 1;
}

int recvAll(int socket, char *buffer, size_t length){
    char *ptr = (char*) buffer;
	int charsProcessed = 0;
    while (length > 0){ //while we havnt recieved all characters keep pulling
        charsProcessed = recv(socket, ptr, length,0);
        if (charsProcessed < 1){ if(buffer) free(buffer); error("ENC SERVER: ERROR recieving to socket"); return -1;}
        ptr += charsProcessed;
        length -= charsProcessed;
    }
    return 1;
}
void talkToServer(const int establishedConnectionFD){
    char buffer[MAXSIZE];
    char* message = NULL;
	char* key = NULL;
    int expectedRead = -5;
    
    memset(buffer, '\0', MAXSIZE);
    expectedRead = recv(establishedConnectionFD, buffer, MAXSIZE, 0);
    if(expectedRead < 0){ error("ENC SERVER: ERROR reading from socket"); return;}
    expectedRead = atoi(buffer); //get the number of chars we expect to read
    message = (char*)malloc(sizeof(char) * expectedRead + 1); memset(message, '\0', expectedRead + 1); //+1 for \0
    key = (char*)malloc(sizeof(char) * expectedRead + 1); memset(key, '\0', expectedRead + 1); //+1 for \0
    // printf("ENC SERVER: expectedRead = %d\n",expectedRead);
	
    if(recvAll(establishedConnectionFD, message, expectedRead) < 0) return; // Get the message from the client
    if(recvAll(establishedConnectionFD, key, expectedRead) < 0) return; //get the key from the client
	
    // printf("ENC SERVER: Message: %s      KEY: %s\n",message,key);
    encrypt(message,key); //note: opt_enc.c should have already verified message and key prereqs.
	// printf("ENC SERVER: Message: %s      KEY: %s\n",message,key);
    
    if(sendAll(establishedConnectionFD, message, expectedRead) < 0) return; // Send cyphertext message back to the client
    if(message) free(message);
    if(key) free(key);
}

int main(int argc, char* argv[]){
	int listenSocketFD, establishedConnectionFD;
	struct sockaddr_in serverAddress, clientAddress;
    char buffer[11];
    pid_t spawnPid = -5;
    socklen_t sizeOfClientInfo;

    if (argc < 2) { fprintf(stderr,"USAGE: %s opt_enc_c port & \n", argv[0]); exit(1);} // Check usage & args
	//accept connection with client
	//if connection is a go then fork and communicate
	
    setupClient(argv[1], &listenSocketFD, &serverAddress);

    while(1){
        sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
        establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);// Accept a connection, blocking if one is not available until one connects
        if (establishedConnectionFD < 0) error("ENC SERVER: ERROR on accept");
        memset(buffer, '\0', 11);
        recvAll(establishedConnectionFD, buffer, 10); //verify connection to otp_enc
        if(strncmp(buffer,"JFisch_ENC",10) != 0){
            close(establishedConnectionFD); //Connection is not otp_enc
            error("ENC SERVER: ERROR source other than otp_enc attempted to connect");
        }
        else{
            spawnPid = fork();
            switch(spawnPid){
                case -1:
                    error("ENC SERVER: ERROR Fork() Failed inside of opt_enc_d \n");
                    break;
                case 0: //child
                    talkToServer(establishedConnectionFD);
                    close(establishedConnectionFD); // Close the existing socket which is connected to the client
                    close(listenSocketFD); // Close the listening socket
                    checkZombies();
                    exit(0); //exit fork process
                    break;
                default: //parent
                    close(establishedConnectionFD); // Close the existing socket which is connected to the client
                    break;
            }
            checkZombies();
        }
    }
    close(listenSocketFD); // Close the listening socket

    return 0;
}