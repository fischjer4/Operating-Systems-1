#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


//Be easier to use ints behind the scenes in the adjaceny matrix. that way for room 2 we just access AM[1]
//we will then link up the ints to actual string room names when displaying to the user



struct Node{
    int room;
    struct Node* next;
};

struct AM{
    int numConnec;
    int room;
    struct Node* list;
};

int connect(struct AM* row , int randRoom){  //creates the next node and returns 1 if room isnt already in connections, 0 otherwise
    struct Node* cur = row->list;
    struct Node* prev = NULL;
    struct Node* newNode = malloc(sizeof(struct Node));
    if(cur == NULL){    //if its the first link in list
        newNode->next = NULL;
        newNode->room = randRoom;
        row->list = newNode;
        return 1;
    }

    while(cur != NULL){ //check if room is already a connection
        if(cur->room == randRoom){
            return 0;
        }
        prev = cur;
        cur = cur->next;
    }
    newNode->next = NULL;   //add the room to the end of list
    newNode->room = randRoom;
    prev->next = newNode;
    return 1;
}

void createConnections(struct AM** graph){
/*

    -For all 7 AMs
        -for number of connections left
            - randomly generate a room that is in the 7
            - if that room is not already in this rooms createConnections
                - add it to the list of connections 
                - add the current room to list of the connections room

*/
    
    int rooms[7];
    int i = 0, randNum = 0;
    for(i = 0; i < 7; i++){
        rooms[i] = graph[i]->room;
    }
    int row = 0,numCon = 0,randRoom = 0;
    for(row = 0; row < 7; row++){
        numCon = graph[row]->numConnec;
        while(numCon < 3){
            randNum = (rand() % 7);
            randRoom = rooms[randNum];
            if(randRoom != graph[row]->room && connect(graph[row],randRoom)){ //if the connection is not equal to itself and isnt already a con
                graph[row]->numConnec++;
                if(connect(graph[randNum], graph[row]->room)){ //connect other room at end of connection (A->B then B->A)
                    graph[randNum]->numConnec++;
                }
                numCon++;
            }
        }
    }

}

struct AM** createRooms(){
    struct AM** graph = malloc(sizeof(struct AM*) * 7);
    struct AM* roomInit = NULL;
    int roomTaken[10] = {0,0,0,0,0,0,0,0,0,0}; //If a room is already used then put a 1 in its spot
    int randRoom = 0, roomCount = 0;
    srand(time(NULL));
    while(roomCount < 7){   //This while loop creates the headers for the Adjaceny matrix
        randRoom = (rand() % 10);
        if(roomTaken[randRoom] == 0){
            roomInit = malloc(sizeof(struct AM));
            roomInit->numConnec = 0;
            roomInit->room = randRoom;
            roomInit->list = NULL;
            graph[roomCount] = roomInit;
            roomCount++;
            roomTaken[randRoom] = 1;
        }
    }
    createConnections(graph);
    return graph;
}

void printAM(struct AM** graph){ //This is just an extra function to print the adjaceny matrix
    int i = 0, rooms = 0;
    struct Node * cur = NULL;
    for(i = 0; i < 7; i++){ //for all 7 rooms print their connections
        printf("%d: ",graph[i]->room);
        cur = graph[i]->list;
        if(cur != NULL){
            for(rooms = 0; rooms < graph[i]->numConnec; rooms++){
                printf("%d, ",cur->room);
                cur = cur->next;
            }
            printf("\n");
        }
    }
}

void fillRoom(FILE* fp, int i, struct Node* cur, struct AM* graphRow, int rooms[]){
    int con = 0;
    fprintf(fp,"ROOM NAME: %d\n",rooms[i]); 
    for(con = 0; con < graphRow->numConnec; con++){
        fprintf(fp,"CONNECTION %d: %d\n",con+1,cur->room);
        cur = cur->next;
    }
    if(i == 0){
        fprintf(fp,"ROOM TYPE: START_ROOM");
    }
    else if(i == 6){
        fprintf(fp,"ROOM TYPE: END_ROOM");
    }       
    else{
        fprintf(fp,"ROOM TYPE: MID_ROOM");
    }
}


void createRoomFiles(struct AM** graph,int pid){
    int rooms[7];
    FILE* fp;
    char command[40];
    struct Node* cur = NULL;
    char file[40];
    int i = 0, con = 0;
    for(i = 0; i < 7; i++){     //for the 7 random rooms chosen create their rooms and fill them
        rooms[i] = graph[i]->room;
        cur = graph[i]->list;
        if(i == 0){
            sprintf(file,"fischjer.rooms.%d/roomStart",pid);
            sprintf(command,"touch %s",file);
            system(command);
            fp = fopen(file,"w+");
            fillRoom(fp,i,cur,graph[i],rooms);
            fclose(fp);
            sprintf(file,"fischjer.rooms.%d/room%d",pid,rooms[i]);
        }
        else{
            sprintf(file,"fischjer.rooms.%d/room%d",pid,rooms[i]);
        }
        sprintf(command,"touch %s",file);
        system(command);
        fp = fopen(file,"w+");
        fillRoom(fp,i,cur,graph[i],rooms);
        fclose(fp);
    }


}


int main(int argc, char** argv){
    int pid = getpid();
    char command[40];
    sprintf(command,"mkdir fischjer.rooms.%d",pid);
    system(command);
    struct AM** graph = createRooms();
    createRoomFiles(graph,pid);
    // printAM(graph); //this will print the adjaceny matrix

    return 0;
}