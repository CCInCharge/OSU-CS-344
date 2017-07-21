#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

struct room {
    int id;
    char* name;
    char* roomType;
    int connections;
    int connected[6];
};

struct room* createNewRoom(int newID, char* newName, char* newRoomType) {
    struct room* newRoom = malloc(sizeof(struct room));
    newRoom->id = newID;
    newRoom->name = newName;
    newRoom->connections = 0;
    newRoom->roomType = newRoomType;

    for (int i = 0; i < 6; i++) {
        newRoom->connected[i] = -1;
    }
    return newRoom;
}

char** createRoomNames() {
    /*
    Returns array of 10 room names, in a random order. To get 7 random room
    names, just pick the first 7 room names of the output array.
    */

    // Seed random number generator
    srand(time(NULL));

    // Hardcoded room names and room name order
    char** rooms = malloc(11 * sizeof(char*));
    char* room0 = "Red";
    char* room1 = "Green";
    char* room2 = "Yellow";
    char* room3 = "Blue";
    char* room4 = "White";
    char* room5 = "Black";
    char* room6 = "Purple";
    char* room7 = "Grey";
    char* room8 = "Magenta";
    char* room9 = "Party";
    rooms[0] = room0;
    rooms[1] = room1;
    rooms[2] = room2;
    rooms[3] = room3;
    rooms[4] = room4;
    rooms[5] = room5;
    rooms[6] = room6;
    rooms[7] = room7;
    rooms[8] = room8;
    rooms[9] = room9;
    rooms[10] = room0;          // Temporary location for swaps

    // Create new array to store output of randomized array order
    char** randRooms = malloc(10 * sizeof(char*));

    // For every element of randRooms, select a random index from rooms
    int i = 10;
    while (i > 0) {
        int curRand = rand() % i;
        randRooms[i - 1] = rooms[curRand];
        rooms[10] = rooms[curRand];
        // Need to move just-selected room to the end of the rooms array
        // This prevents the same room from being selected twice
        for (int j = curRand; j < 10; j++) {
            rooms[j] = rooms[j + 1];
        }
        i--;
    }

    free(rooms);
    return randRooms;
}

void freeRoomNames(char** rooms) {
    free(rooms);
}

int roomsConnectionsFull(struct room** rooms) {
    /*
    Returns 1 if all rooms in given array have between 3 and 6 connections,
    inclusive. Returns 0 otherwise
    */
    for (int i = 0; i < 7; i++) {
        if (rooms[i]->connections < 3 || rooms[i]->connections > 6) {
            return 0;
        }
    }
    return 1;
}

void addRoomConnection(struct room* room1, struct room* room2) {
    // Check to see if maximum number of room connections has been added
    if (room1->connections == 6 || room2->connections == 6) {
        return;
    }

    // Check to see if these rooms are already connected
    for (int i = 0; i < room1->connections; i++) {
        if (room1->connected[i] == room2->id) {
            return;
        }
    }
    for (int i = 0; i < room2->connections; i++) {
        if (room2->connected[i] == room1->id) {
            return;
        }
    }
    if (room1 == room2) {
        return;
    }

    // If these rooms can be connected, then connect them
    room1->connected[room1->connections] = room2->id;
    room2->connected[room2->connections] = room1->id;
    room1->connections++;
    room2->connections++;
}

void connectRooms(struct room** rooms) {
    while (roomsConnectionsFull(rooms) == 0) {
        addRoomConnection(rooms[rand() % 7], rooms[rand() % 7]);
    }
}

int main() {
    int curPID = getpid();

    char dirName[30] = "chenchar.rooms.";
    char curPIDChar[10];
    snprintf(curPIDChar, 10, "%d", curPID);

    strcat(dirName, curPIDChar);
    int newDir = mkdir(dirName, 0755);
    if (newDir < 0) {
        fprintf(stderr, "Could not create directory\n");
        perror("Error opening file");
        exit(1);
    }

    // Returns randomized list of room names
    char** roomNames = createRoomNames();

    // Creates room structs, initializes them, and gives them room types
    struct room* rooms[7];
    for (int i = 0; i < 7; i++) {
        if (i == 0) {
            rooms[i] = createNewRoom(i, roomNames[i], "START_ROOM");
        }
        else if (i == 6) {
            rooms[i] = createNewRoom(i, roomNames[i], "END_ROOM");
        }
        else {
            rooms[i] = createNewRoom(i, roomNames[i], "MID_ROOM");
        }
    }

    // Randomly generate connections betweeen rooms
    connectRooms(rooms);

    // Output room details to files
    char roomFilename[30];
    char roomFileNum[10];
    ssize_t nwritten;
    for (int i = 0; i < 7; i++) {
        strcpy(roomFilename, dirName);
        snprintf(roomFileNum, 10, "%d", i);
        strcat(roomFilename, "/room");
        strcat(roomFilename, roomFileNum);

        int fileDesc = open(roomFilename, O_WRONLY | O_CREAT, 0644);

        if (fileDesc < 0) {
            fprintf(stderr, "Could not open %s\n", roomFilename);
            perror("Error opening file");
            exit(1);
        }

        // Build up strings to write and write them to file
        char textToWrite[100];
        strcpy(textToWrite, "ROOM NAME: ");
        strcat(textToWrite, rooms[i]->name);
        strcat(textToWrite, "\n");
        nwritten = write(fileDesc, textToWrite,
            strlen(textToWrite) * sizeof(char));
        for (int j = 1; j <= rooms[i]->connections; j++) {
            // Convert connection number to string for output
            char connectionNum[3];
            snprintf(connectionNum, 3, "%d", j);

            strcpy(textToWrite, "CONNECTION ");
            strcat(textToWrite, connectionNum);
            strcat(textToWrite, ": ");
            strcat(textToWrite, rooms[rooms[i]->connected[j - 1]]->name);
            strcat(textToWrite, "\n");
            nwritten = write(fileDesc, textToWrite,
                strlen(textToWrite) * sizeof(char));
        }
        strcpy(textToWrite, "ROOM TYPE: ");
        strcat(textToWrite, rooms[i]->roomType);
        strcat(textToWrite, "\n");
        nwritten = write(fileDesc, textToWrite,
            strlen(textToWrite) * sizeof(char));

        close(fileDesc);
    }

    freeRoomNames(roomNames);
    return 0;
}
