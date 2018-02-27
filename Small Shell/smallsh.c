#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

int fg = 0;

void exitFunc(){ //function for exiting
    exit(0);
}

void changeDir(char* argv[40]){
    if(argv[1] == NULL){        // if just cd is given then change to home dir
        chdir(getenv("HOME"));
    }
    else if(chdir(argv[1]) < 0){ //otherwise change to given dir
        printf("Error: Could Not Move to %s\n", argv[1]); fflush(stdout);
    }
}

int statUs(int stat){ //print the status of killed signal, stopped signal, or exit value
    if (WIFEXITED(stat)) {
        printf("Exit value %d\n", WEXITSTATUS(stat));
    } 
    else if (WIFSIGNALED(stat)) {
        printf("Child Killed (Signal %d)\n", WTERMSIG(stat));
    }
    else if (WIFSTOPPED(stat)) {
        printf("Child Stopped (signal %d)\n", WSTOPSIG(stat));
    }   
}

void initialize(int* ampersand,char* cur,char cmmd[513],char* argv[40],char** inFile,char** outFile,int* inFP, int* outFP){
    int i = 0;
    *ampersand = 0;
    *inFP = -5;
    *outFP = -5;
    cur = argv[0];
    memset(cmmd,513,'\0');
    while(cur != NULL){     //reset all variables to their default values to be refiled
        free(cur);
        memset(cur,40,'\0');
        cur = argv[++i];
    }
    free(*inFile);
    free(*outFile);
    *inFile = NULL;
    *outFile = NULL;
}

void parseInput(char* argv[40], char cmmd[100], char** inFile, char** outFile, int* ampersand){
    char* parsed = NULL;
    int numArg = 0;
    char buff[6];
    int i = 0, j = 0, x = 0;
    char og[100]; strncpy(og,cmmd,100); memset(cmmd,100,'\0');
    int len = (sizeof(cmmd) / sizeof(cmmd[0]));
    int beginIndex = 0, endIndex = 0;
    memset(buff,6,'\0');
    sprintf(buff, "%d", getpid());  //turn pid integer into string
    while(og[i] != '\n' && og[i] != '\0'){  //convert all $$ to pids,
        if(og[i] == '$' && og[i+1] == '$'){
            for(x =0; x < 5; x++){  //replace $$ with pid
                cmmd[j] = buff[x];
                j++;
            }
            i+=2;
        }
        else{
            cmmd[j] = og[i];
            j++;
            i++;
        }
    }
    cmmd[j] = '\0';

    parsed = strtok(cmmd," \n"); // parse the command until all details are pulled out
    while(parsed != NULL){  
        if(strcmp(parsed, "<") == 0){
            parsed = strtok(NULL," \n");
            *inFile = strdup(parsed);
        }
        else if(strcmp(parsed,">") == 0){
            parsed = strtok(NULL," \n");
            *outFile = strdup(parsed);
        }
        else if(strcmp(parsed,"&") == 0){
            if(fg == 0)
                *ampersand = 1;
            break;          
        }
        else{   //if not a redirection signal then add it to the arguements
            argv[numArg] = strdup(parsed);
            numArg++;
        }
        parsed = strtok(NULL," \n");                            
    }
    argv[numArg] = NULL;
}

void redirectStreams(char* newStream, int* fileDesc, int origFileDesc, int out){
    if(!out){
        *fileDesc = open(newStream,O_RDONLY);   //if there was no output or input file and and foreground is in place, redirect to /dev/null
    }
    else{
        *fileDesc = open(newStream,O_WRONLY | O_CREAT | O_TRUNC, 0644);
    }
    if(*fileDesc == -1){
        printf("Error Opening %s \n", newStream); fflush(stdout);
        _exit(1);
    }
    if(dup2(*fileDesc, origFileDesc) == -1){    //redirect 
        printf("Error(dup2) redirecting stdin to %s \n", newStream); fflush(stdout);
        _exit(1);
    }
    close(*fileDesc);  
}

void checkZombie(int* status){
    pid_t pidReturn = waitpid(-1,&(*status),WNOHANG);
    while(pidReturn > 0){       //while there continues to be zombies ready to reap, reap them
        printf("+ Child %d has exited. ",pidReturn);
        statUs(*status);
        pidReturn = waitpid(-1,&(*status),WNOHANG);
    }
}
void handler(int n){    //if ^Z then switch the flag controlling & significance 
    if(n == 20 && fg == 0){
        printf("Entering Foreground mode, & will be ignored"); fflush(stdout);
        fg = 1;
    }
    else if(n == 20 && fg == 1){
        printf("Exiting Foreground mode, & will be NOT be ignored"); fflush(stdout);
        fg = 0;        
    }
    printf("\n");fflush(stdout); //if ^C then new line
}

void startSmShell(){
    int childExit = -5,i = -5, ampersand = 0, status = -5, inFP = -5, outFP = -5, result = -5;
    char* cur = NULL; char* inFile = NULL; char* outFile = NULL;   
    char cmmd[100]; memset(cmmd,100,'\0');
    char* argv[40]; argv[0] = NULL;
    pid_t spawnPid = -5;
    struct sigaction stopSig = {0};
    
    argv[0] = NULL;
    stopSig.sa_handler = SIG_IGN; //Stop CTRL + C to end program
    stopSig.sa_flags = 0;
    stopSig.sa_handler = handler;
    sigfillset(&(stopSig.sa_mask));
    sigaction(SIGINT, &stopSig, NULL); 
    sigaction(SIGTSTP, &stopSig, NULL); 
    
    while(1){
        initialize(&ampersand,cur,cmmd,argv,&inFile,&outFile,&inFP,&outFP);
        printf(": "); fflush(stdout);
        fgets(cmmd,100,stdin); //get input
        parseInput(argv, cmmd,&inFile, &outFile, &ampersand);

        if(argv[0] == NULL || argv[0][0] == '#'){ 
            checkZombie(&status);
        }
        else if(strcmp(argv[0],"cd") == 0){
            changeDir(argv);
        }
        else if(strcmp(argv[0],"status") == 0){
            statUs(status);
        }
        else if(strcmp(argv[0],"exit") == 0){
            exitFunc();
        }
        else{
            spawnPid = fork();
            switch(spawnPid){
                case -1: //if things didn't work out then error
                    perror("Fork failed\n");
                    status = 1;
                    break;
                case 0: //If it's the child then do the work
                    if(!ampersand){ //if it's in the foreground then allow the user to ctrl + c out of it
                        stopSig.sa_handler = SIG_DFL;
                        sigaction(SIGINT, &stopSig, NULL);
                    }
                    if(inFile != NULL){
                        redirectStreams(inFile,&inFP,STDIN_FILENO, 0);
                    }
                    if(outFile != NULL){
                        redirectStreams(outFile,&outFP,STDOUT_FILENO, 1);
                    }
                    if(ampersand){  //if background is on and no input/output file were givem, redirect to no where
                        if(inFile == NULL){
                            redirectStreams("/dev/null",&inFP,STDIN_FILENO, 0);
                        }
                        if(outFile == NULL){
                            redirectStreams("/dev/null",&outFP,STDOUT_FILENO, 1);
                        }
                    }

                    if (execvp(*argv, argv) < 0){     
                        printf("%s: ",argv[0]); fflush(stdout);
                        perror("");
                        status = 1;
                        checkZombie(&status);
                        _exit(1);
                    }
                    break;
                default:   //if it's the parent then wait for the child (fork) to terminate
                    if(!ampersand){
                        waitpid(spawnPid, &status, 0);
                    }
                    else{
                        printf("Background process ID %d\n",spawnPid); fflush(stdout);
                    }
                    break;
            }
            checkZombie(&status);
        }
    }
}

int main(){
    startSmShell();
    
    return 0;
}