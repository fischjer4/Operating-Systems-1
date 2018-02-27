#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>


void printTime(){
    time_t rawtime;
    char line[256]; memset(line,'\0',sizeof(line));
    FILE* fp = fopen("currentTime.txt","wr");
    struct tm* timeP;
    char* days[] = {"Monday","Tuesday","Wednesday","Thursday","Friday","Saturday","Sunday"};
    char* months[] = {"January","February","March","April","May","June","July","August","September","October","November","December"};
    time(&rawtime);
    timeP = localtime(&rawtime);

    if(timeP->tm_hour > 12){    //If PM then add the pm after the time and paste all to a file
        fprintf(fp,"%d:%02dpm, %s, %s %d, %d\n", (timeP->tm_hour)%12, timeP->tm_min, days[timeP->tm_wday-1], months[timeP->tm_mon], timeP->tm_mday, timeP->tm_year+1900);
    }
    else{
        fprintf(fp,"%d:%02dam, %s, %s %d, %d", (timeP->tm_hour)%12, timeP->tm_min, days[timeP->tm_wday-1], months[timeP->tm_mon], timeP->tm_mday, timeP->tm_year+1900);
    }
    fclose(fp);
    fp = fopen("currentTime.txt","r");
    fgets(line, sizeof(line), fp); //get the time that was just pasted in the file
    printf("\n%s",line);
    fclose(fp);
    
}


int checkIfValid(char nextDest[], char currConnec[]){
    char* singleWord = NULL;
    char copyConnec[200];
    if(strncmp(nextDest,"time", sizeof(nextDest)) == 0){ //return 2 if the word is time
        return 2;
    }

    strcpy(copyConnec, currConnec);
    singleWord = strtok(copyConnec," ,."); //this parses the string into words based upon spaces commas or periods
    while(singleWord != NULL){
        if(strncmp(nextDest,singleWord, sizeof(nextDest)) == 0){ //return 0 if the word matches the nextDest entered
            return 0;
        }
        singleWord = strtok(NULL," ,.");
    }
    return 1;                               //return 1 if the nextDest entered doesnt match any words in the connections string
}


char* getCurRoom(FILE* fp){
    char line[256];
    char* singleWord;
    fgets(line, sizeof(line), fp);
    singleWord = strtok(line,":");
    singleWord = strtok(NULL,"");
    singleWord[strcspn(singleWord, "\n")] = 0;
    
    return singleWord;
}

char* getCurCons(FILE* fp){
    //NOTE: This relies on the fact that getCurRoom is called and uses a fgets(). If that isn't '=the case you need to add an fgets before loop
    char line[256]; memset(line,'\0',sizeof(line));
    char* path = malloc(sizeof(char)*256); memset(path,'\0',sizeof(path));
    char* singleWord;
    char next;
    int count = 0;
    char* leadingWord = "ROOM NAME:";
    while(1){
        fgets(line, sizeof(line), fp);  //get the next line in file
        leadingWord = strtok(line,":"); //parse out the CONNECTION #:
        singleWord = strtok(NULL,": ");  //The actual room connection
        if(strcmp(leadingWord,"ROOM TYPE") != 0){
            singleWord[strcspn(singleWord, "\n")] = 0;
            strcat(path,singleWord);
            strcat(path,",");
        }
        else{ 
            singleWord[strcspn(singleWord, "\n")] = 0;
            strcat(path,"#");
            strcat(path,singleWord);
            
            next = path[count];
            while(next != '#'){ //remove the comma on the last room name
                count++;
                next = path[count];
            }
            path[count-1] = ' ';

            break;
        }
    }
    
    return path;
}

void game(char pid[30]){
    char currRoom[20]; memset(currRoom,'\0',sizeof(currRoom));
    char currConnec[256]; memset(currConnec,'\0',sizeof(currConnec));
    char path[100];memset(path,'\0',sizeof(path));
    char nextDest[30]; memset(nextDest,'\0',sizeof(nextDest));
    char* roomT;
    char file[90];memset(file,'\0',sizeof(file));
    int numMoves = 0;
    int error = 0;
    FILE* fp;
    
    sprintf(file,"fischjer.rooms.%s/roomStart",pid);
    fp = fopen(file,"r");
    if(fp == NULL) {printf("*********** Error opening: %s\n",file); exit(-1);}
    if(fp != NULL){
        strcpy(currRoom, getCurRoom(fp)); //add the start room to the path
        strcat(path,currRoom);
        fclose(fp);fp = fopen(file,"r");    //close and reopen the file now or else you would have been in the next room already
        while(strncmp("END_ROOM",roomT,sizeof(roomT)) != 0){
            strcpy(currRoom, getCurRoom(fp));
            strcpy(currConnec,getCurCons(fp));
            roomT = strtok(currConnec,"#"); 
            roomT = strtok(NULL,"#");
            
            if(strncmp("END_ROOM",roomT,sizeof(roomT)) == 0){ break;}
        
            do{
                printf("\nCURRENT LOCATION: %s \n",currRoom);
                printf("POSSIBLE CONNECTIONS: %s \n",currConnec);
                printf("WHERE TO? >");
                scanf("%s",nextDest);
                error = checkIfValid(nextDest, currConnec);
                while(error == 2){ //if time was entered instead of the destination
                    printTime();
                    printf("WHERE TO? >");
                    scanf("%s",nextDest);
                    error = checkIfValid(nextDest, currConnec);
                }
                if(error == 1){
                    printf("\nHUH? I DON’T UNDERSTAND THAT ROOM. TRY AGAIN. \n");
                }
            }while(error == 1);
            
            numMoves++;
            strcat(path,","); strcat(path,nextDest);    //add the move to the path taken, open up new room and repeat
            fclose(fp);
            sprintf(file,"fischjer.rooms.%s/room%s",pid,nextDest);
            fp = fopen(file,"r");
            if(fp == NULL) {printf("*********** Error opening: %s\n",file);exit(-1);}
            
        }
        
        printf("\nYOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");
        printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:%s\n",numMoves,path);
    }

}



char* parseForPID(char directory[200]){ //from the name of the most recent directory parse the pid from it
    char* str = strtok(directory,".");
    str = strtok(NULL,".");
    str = strtok(NULL,".");
    return str;
}
char* numDirs(const char* path){ //get the most recent directory
    long curTime = 0;
    long maxT = 0;
    char* pid = malloc(sizeof(char)*30);
    struct dirent* dent;
    struct stat attr;
    char directory[200];
    DIR* srcdir = opendir(path);

    while((dent = readdir(srcdir)) != NULL){ //while there are subdirectories in the location im at
        struct stat st;
        if(strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
            continue;
        if (fstatat(dirfd(srcdir), dent->d_name, &st, 0) < 0){
            perror(dent->d_name);
            continue;
        }
        if(S_ISDIR(st.st_mode)){    //if it is a directory
            curTime = st.st_atim.tv_sec;    //record when it was created
            if(curTime > maxT){ //compare it against current max time and if it is newer, then update the cur directory
                maxT = curTime;
                strcpy(directory,dent->d_name);
            }
        }
    }
    closedir(srcdir);
    strcpy(pid,parseForPID(directory));
    return pid;
}
int main(int argc, char** argv){
    char* pid = numDirs("./");
    game(pid);

    return 0;
}