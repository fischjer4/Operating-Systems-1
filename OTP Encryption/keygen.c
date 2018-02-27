#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void createKey(const int numChars){
    int i = 0, letter = -5;
    char* keyChars = (char*)malloc(sizeof(char) * (numChars + 2)); //+ 2, 1 for \n and 1 for \0
    if(keyChars < 0){fprintf(stderr,"KEYGEN: ERROR malloc() in keygen failed\n"); exit(1);}
    memset(keyChars,'\0',sizeof(keyChars));
    srand(time(NULL));
    for(i = 0; i < numChars; i++){
        letter = (rand()  % 27) + 65; //random number between 0 and 26, add 65 to it for ascii upper case
        if(letter == 91) letter = 32; //if 91 make it a space
        keyChars[i] = (char)letter; //cast the random number into a character
    }
    keyChars[i] = '\n'; //add a \n to the end of the string as a file needs one
    printf("%s",keyChars); fflush(stdout);
    free(keyChars);
}

int main(int argc, char* argv[]){
    if(argc < 2) {fprintf(stderr,"USAGE: Keygen numChars \n"); exit(1);}
    int numChars = atoi(argv[1]);
    createKey(numChars);

    return 0;
}