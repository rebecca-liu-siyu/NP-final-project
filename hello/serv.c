# include "unp.h"
# include "serv.h"

// Functions
struct sockaddr_in sockaddr_setup(short family, int port, int addr) {
    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = family;
    serv_addr.sin_addr.s_addr = addr;
    serv_addr.sin_port = htons(port);
    return serv_addr;
}

// int main(int argc, char** argv)
int main(int argc, char** argv) {
    MAIN_START;
    srand(time(NULL));
    userfile_setup();
    rooms_setup();
    if (argc != 2) { fprintf(stderr, "Usage: %s <port>\n", argv[0]); exit(1); }

    listen_fd = Socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr = sockaddr_setup(AF_INET, atoi(argv[1]), htonl(INADDR_ANY));
    
    Bind(listen_fd, (SA*)&serv_addr, sizeof(serv_addr));
    Listen(listen_fd, LISTENQ);

    // char myIP[16];
    // inet_ntop(AF_INET, &serv_addr, myIP, INET_ADDRSTRLEN);
    printf("Server is running on port %s...\n", argv[1]);

    // client info.
    while (true) {
        struct sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);
        int connect_fd = Accept(listen_fd, (SA*)&cli_addr, &cli_len);

        pthread_t tid;
        int* connect_fd_ptr = malloc(sizeof(int));
        *connect_fd_ptr = connect_fd;
        pthread_create(&tid, NULL, handle_client, connect_fd_ptr);
    }
    MAIN_END;
}

void rooms_setup() {
    RoomCount = -1;
    Rooms = (Room_t*)malloc(sizeof(Room_t) * MAXCLIENT);
    for (int i = 0; i < MAXCLIENT; i++) {
        Rooms[i].owner = (char*)malloc(sizeof(char) * 50);
        Rooms[i].roomid = -1;
        Rooms[i].capacity = -1;
        Rooms[i].players = NULL;
        Rooms[i].ownerStatus = false;
        Rooms[i].isPublic = false;

        Rooms[i].onlineCount = 0;
        Rooms[i].onlineStatus = NULL;
        Rooms[i].Connfds = NULL;
        Rooms[i].gameEnd = false;
    }
}
void room_print(Room_t* room) {
    printf("\tRoom ID: %d\n", room->roomid);
    printf("\tOwner: %s\n", room->owner);
    printf("\tCapacity: %d\n", room->capacity);
    printf("\tOwner Status: %s\n", room->ownerStatus ? "online" : "offline");
    printf("\tPublic?: %s\n", room->isPublic ? "true" : "false");
    printf("\tPlayers:\n");
    for (int i = 0; i < room->capacity; i++) {
        printf("\t\t%d: %s -> %s\n", i + 1, room->players[i], (room->onlineStatus[i]) ? "online" : "offline");
    }
    printf("\tOnline Count: %d\n", room->onlineCount);
    printf("\n");
}

void userfile_setup() {
    FILE* file = fopen(USER_FILE, "r");
    if (!file) { perror("Could not open user file\n"); return -1; }

    FILE* tpfile = fopen(TP_FILE, "w");
    if (!tpfile) { fclose(file); perror("Could not open tp user file\n"); return -1; }

    char userinfo[MAXLINE], exist_username[MAXLINE], exist_password[MAXLINE], status[5], exist_fd[32];
    bzero(&userinfo, MAXLINE);
    while (fgets(userinfo, MAXLINE, file)) {
        bzero(&exist_username, MAXLINE);
        bzero(&exist_password, MAXLINE);
        bzero(&status, 5);
        bzero(&exist_fd, 32);

        sscanf(userinfo, "%s %s %s %s", exist_username, exist_password, status, exist_fd);
        snprintf(userinfo, MAXLINE, "%s %s off 0\n", exist_username, exist_password);
        fputs(userinfo, tpfile);
    }
    fclose(file);
    fclose(tpfile);
    remove(USER_FILE);
    rename(TP_FILE, USER_FILE);
    return;
}

void cli_disconnect(int connfd) {
    FILE* file = fopen(USER_FILE, "r");
    if (!file) { perror("Could not open user file\n"); return -1; }

    FILE* tpfile = fopen(TP_FILE, "w");
    if (!tpfile) { fclose(file); perror("Could not open tp user file\n"); return -1; }

    char userinfo[MAXLINE], exist_username[MAXLINE], exist_password[MAXLINE], status[5], exist_fd[32], sendline[MAXLINE];
    bzero(&userinfo, MAXLINE);
    while (fgets(userinfo, MAXLINE, file)) {
        bzero(&exist_username, MAXLINE);
        bzero(&exist_password, MAXLINE);
        bzero(&status, 5);
        bzero(&exist_fd, 32);

        sscanf(userinfo, "%s %s %s %s", exist_username, exist_password, status, exist_fd);
        if (atoi(exist_fd) == connfd && strcmp(status, "on") == 0) {
            snprintf(userinfo, MAXLINE, "%s %s off %d\n", exist_username, exist_password, connfd);
            printf("userinfo update: %s\n", userinfo);
        }
        fputs(userinfo, tpfile);
    }
    fclose(file);
    fclose(tpfile);
    remove(USER_FILE);
    rename(TP_FILE, USER_FILE);
    return;
}

void* handle_client(void* connfd_ptr) {
    int connfd = *(int*)connfd_ptr;
    free(connfd_ptr);

    char buffer[MAXLINE], sendline[MAXLINE];
    while (true) {
        bzero(&buffer, MAXLINE);
        int readlen = read(connfd, buffer, sizeof(buffer));
        ///////////////////////////////////////////////////////////////////
        // client still connect???
        if (readlen <= 0) {
            printf("Client disconnected while register and login\n");
            cli_disconnect(connfd);
            break;
        }
        ///////////////////////////////////////////////////////////////////
        printf("parse input: %s\n", buffer);
        char command[10], username[50], password[50];
        bzero(&command, 10);
        bzero(&username, 50);
        bzero(&password, 50);
        sscanf(buffer, "%s %s %s", command, username, password);
        ///////////////////////////////////////////////////////////////////
        // Check client's input format
        printf("check input format: %s\n", buffer);
        if (!strcmp(username, "") || !strcmp(password, "")) {
            printf("Write: Invalid command format: `%s %s %s`\n", command, username, password);
            bzero(&sendline, MAXLINE);
            sprintf(sendline, "Invalid command format\n");
            // Write(connfd, sendline, sizeof(sendline));
            bzero(&sendline, MAXLINE);
            sprintf(sendline, "1");
            Write(connfd, sendline, sizeof(sendline));
            continue;
        }
        ///////////////////////////////////////////////////////////////////
        
        printf("Command: %s, Username: %s, Password: %s from %d\n", command, username, password, connfd);
        ///////////////////////////////////////////////////////////////////
        // Register
        if (strcmp(command, "1") == 0) {
            if (!register_user(connfd, username, password)) {
                printf("Write: Register successful!\n");
                bzero(&sendline, MAXLINE);
                sprintf(sendline, "Register successful!\nPlease login by command code '2'!\n");
                // Writen(connfd, sendline, sizeof(sendline));
                bzero(sendline, MAXLINE);
                sprintf(sendline, "2");
                Write(connfd, sendline, sizeof(sendline));

            } else {
                printf("Write: Username already exist!\n");
                bzero(&sendline, MAXLINE);
                sprintf(sendline, "Username already exist!\n");
                // Writen(connfd, sendline, sizeof(sendline));
                bzero(&sendline, MAXLINE);
                sprintf(sendline, "3");
                Write(connfd, sendline, sizeof(sendline));
            }
        } 
        ///////////////////////////////////////////////////////////////////
        // Login
        else if (strcmp(command, "2") == 0) {
            int login_status = login_user(connfd, username, password);
            if (!login_status) {
                printf("Write: Login successful!\n");
                bzero(&sendline, MAXLINE);
                sprintf(sendline, "Login successful!\nWelcome to the Lobby, %s\n", username);
                // Writen(connfd, sendline, sizeof(sendline));
                bzero(&sendline, MAXLINE);
                sprintf(sendline, "4");
                Write(connfd, sendline, sizeof(sendline));
                // Go to Lobby - 1 for newRoom, 2 for joinRoom, 3 for exit
                Lobby(connfd, username);
                break;
                
            } 
            else if (login_status == 1) {
                printf("Write: User already login via other device!\n");
                bzero(&sendline, MAXLINE);
                sprintf(sendline, "User already login via other device!\n");
                // Writen(connfd, sendline, sizeof(sendline));
                bzero(&sendline, MAXLINE);
                sprintf(sendline, "5");
                Write(connfd, sendline, sizeof(sendline));
                continue;
            }
            else {
                printf("Write: Wrong Username or password!\n");
                bzero(&sendline, MAXLINE);
                sprintf(sendline, "Wrong Username or password!\n");
                // Writen(connfd, sendline, sizeof(sendline));
                bzero(&sendline, MAXLINE);
                sprintf(sendline, "6");
                Write(connfd, sendline, sizeof(sendline));
                continue;
            }
        } 
        ///////////////////////////////////////////////////////////////////
        // Wrong command code
        else {
            printf("Write: Invalid command code: %s\n", command);
            bzero(&sendline, MAXLINE);
            sprintf(sendline, "Invalid command code\n");
            // Writen(connfd, sendline, sizeof(sendline));
            bzero(&sendline, MAXLINE);
            sprintf(sendline, "7");
            Write(connfd, sendline, sizeof(sendline));
            continue;
        }
        ///////////////////////////////////////////////////////////////////////
    }
    close(connfd);
    return NULL;
}

// registerCheck() return 0 - register sucessful, 1 - username already exist
int registerCheck(FILE* file, const char* username, const char* passwd) {
    char exist_username[MAXLINE];
    while (fscanf(file, "%s", exist_username) != EOF) {
        if (strcmp(exist_username, username) == 0) {
            printf("Username '%s' already exist!\n", exist_username);
            return 1;
        }
    }
    return 0;
}
// register_user() return 0 - register sucessful, 1 - username already exist
int register_user(int connfd, const char* username, const char* passwd) {
    FILE* file = fopen(USER_FILE, "a+");
    if (!file) { perror("Could not open user file\n"); return -1; }
    if (registerCheck(file, username, passwd)) { fclose(file); return 1; }
    fprintf(file, "%s %s off %d\n", username, passwd, connfd);
    fclose(file);
    return 0;
}

// login_user() return 0 - login successful, 1 - user already login in via other device, 2 - invalid username or wrong password
int login_user(int connfd, const char* username, const char* password) {
    FILE* file = fopen(USER_FILE, "r");
    if (!file) { perror("Could not open user file\n"); return -1; }

    FILE* tpfile = fopen(TP_FILE, "w");
    if (!tpfile) { fclose(file); perror("Could not open tp user file\n"); return -1; }

    bool login_flag = false;
    char userinfo[MAXLINE], exist_username[MAXLINE], exist_password[MAXLINE], status[5], exist_fd[32], sendline[MAXLINE];
    bzero(&userinfo, MAXLINE);
    while (fgets(userinfo, MAXLINE, file)) {
        bzero(&exist_username, MAXLINE);
        bzero(&exist_password, MAXLINE);
        bzero(&status, 5);
        bzero(&exist_fd, 32);

        sscanf(userinfo, "%s %s %s %s", exist_username, exist_password, status, exist_fd);
        if (strcmp(username, exist_username) == 0 && strcmp(password, exist_password) == 0) {
            if (strcmp(status, "off") == 0) {
                login_flag = true;
                snprintf(userinfo, MAXLINE, "%s %s on %d\n", username, password, connfd);
                printf("new userinfo: %s\n", userinfo);
            } 
            else if (strcmp(status, "on") == 0) {
                // Write(connfd, "User already login via other device\n", MAXLINE);
                fclose(file);
                fclose(tpfile);
                remove(TP_FILE);
                return 1;
            } else {
                printf("status error: %s\n", status);
                fclose(file);
                fclose(tpfile);
                remove(TP_FILE);
                return 2;
            }
            printf("userinfo: %s\n", userinfo);
        }
        fputs(userinfo, tpfile);
    }
    fclose(file);
    fclose(tpfile);
    remove(USER_FILE);
    rename(TP_FILE, USER_FILE);

    return (login_flag) ? 0 : 2;
}
// Lobby() 0 - newRoom, 1 - joinRoom, 2 - exit
void Lobby(int connfd, const char* username) {
    printf("////////////////////////////////////\n");
    printf("Lobby\n");
    char command[10];
    bzero(&command, 10);
    while (true) {
        int n = read(connfd, command, sizeof(command));
        if (n <= 0) {
            printf("Client disconnected while in Lobby\n");
            cli_disconnect(connfd);
            return;
        }

        if (command[0] == '\0') continue;
        printf("Client command: %s\n", command);
        if (strcmp(command, "1") == 0) {
            printf("Choice: New Room\n");
            int newRoomResult = NewRoom(connfd, username);
            Room_t* room = NULL;
            for (int i = 0; i <= RoomCount; i++) {
                if (Rooms[i].roomid == newRoomResult) {
                    room = &Rooms[i];
                    break;
                }
            }
            if (newRoomResult == -1) break;
            else if (newRoomResult == -2) continue;
            else {
                // fork a new process to handle the new game room
                printf("You are the host, wait for other to join the room\n");
                HandleRoom(connfd, room->roomid, username);
            }

        } 
        else if (strcmp(command, "2") == 0) {
            printf("Choice: Join Room\n");
            int joinRoomResult = JoinRoom(connfd, username);
            Room_t* room = NULL;
            for (int i = 0; i <= RoomCount; i++) {
                if (Rooms[i].roomid == joinRoomResult) {
                    room = &Rooms[i];
                    break;
                }
            }
            if (joinRoomResult == -1) continue;
            else if (joinRoomResult == -2) continue;
            // else if (joinRoomResult == -3) break;
            // fork a new process to handle the join game room
            else {
                // join the forked game room by room id
                printf("wait for other to join the room\n");
                HandleRoom(connfd, room->roomid, username);
            }
        
        } else if (strcmp(command, "3") == 0) {
            printf("Choice: Exit\n");
            cli_disconnect(connfd);
            break;
        } else {
            printf("Choice: Invalid command\n");
            Write(connfd, "-1", MAXLINE);
        }
    }
}

int NewRoom(int connfd, const char* username) {
    // printf("- New Room Setup:\n");
    char input[MAXLINE], sendline[MAXLINE];
    int RoomID;
    while (true) {
        printf("- New Room Setup:\n");
        bzero(&input, MAXLINE);
        int n = read(connfd, input, MAXLINE);
        if (n <= 0) {
            printf("Client disconnected while creating New Room\n");
            cli_disconnect(connfd);
            return -1;
        }
        if (input[0] == '\0') continue;

        char roomname[50], capacity[10], isPublic[5];
        bzero(&roomname, 50);
        bzero(&capacity, 10);
        bzero(&isPublic, 5);
        sscanf(input, "%s\n%s %s", roomname, capacity, isPublic);
        
        if (atoi(capacity) < 2 || atoi(capacity) > 10) {
            printf("Invalid room capacity: %s\n", capacity);
            bzero(&sendline, MAXLINE);
            sprintf(sendline, "-1");
            Write(connfd, sendline, sizeof(sendline));
            continue;
        }
        if (strcmp(isPublic, "T") != 0 && strcmp(isPublic, "F") != 0) {
            printf("Invalid room isPublic: %s\n", isPublic);
            bzero(&sendline, MAXLINE);
            sprintf(sendline, "-2");
            Write(connfd, sendline, sizeof(sendline));
            continue;
        }
        // Create new room
        RoomCount++;
        if (RoomCount >= MAXCLIENT) {
            printf("Room is full\n");
            bzero(&sendline, MAXLINE);
            sprintf(sendline, "-3");
            Write(connfd, sendline, sizeof(sendline));
            return -2;
        }

        int roomid;
        bool isUnique;
        do {
            isUnique = true;
            roomid = rand() % 3000 + 1;
            for (int i = 0; i < RoomCount; i++) {
                if (Rooms[i].roomid == roomid) {
                    isUnique = false;
                    break;
                }
            }
        } while (!isUnique);

        printf("Create new room(%d): %s %s %s\n", RoomCount + 1, roomname, capacity, isPublic);
        strcpy(Rooms[RoomCount].owner, username);
        Rooms[RoomCount].roomid = roomid; // 1 ~ 3000
        Rooms[RoomCount].capacity = atoi(capacity);
        Rooms[RoomCount].players = (char**)malloc(sizeof(char*) * Rooms[RoomCount].capacity);
        Rooms[RoomCount].onlineStatus = (bool*)malloc(sizeof(bool) * Rooms[RoomCount].capacity);
        Rooms[RoomCount].Connfds = (int*)malloc(sizeof(int) * Rooms[RoomCount].capacity);
        for (int i = 0; i < Rooms[RoomCount].capacity; i++) {
            Rooms[RoomCount].players[i] = (char*)malloc(sizeof(char) * 50);
            bzero(Rooms[RoomCount].players[i], 50);
            Rooms[RoomCount].onlineStatus[i] = false;
            Rooms[RoomCount].Connfds[i] = -1;
        }
        strcpy(Rooms[RoomCount].players[0], username);
        Rooms[RoomCount].onlineStatus[0] = true;
        Rooms[RoomCount].ownerStatus = true;
        Rooms[RoomCount].onlineCount = 1;
        Rooms[RoomCount].isPublic = (strcmp(isPublic, "T") == 0) ? true : false;
        Rooms[RoomCount].Connfds[0] = connfd;
        Rooms[RoomCount].gameEnd = false;

        // Debug room info
        room_print(&Rooms[RoomCount]);
        
        bzero(&sendline, MAXLINE);
        sprintf(sendline, "%d", Rooms[RoomCount].roomid);
        Write(connfd, sendline, sizeof(sendline));
        RoomID = Rooms[RoomCount].roomid;
        break;
    }
    return RoomID;
}
/**/
int JoinRoom(int connfd, const char* username) {
    char input[MAXLINE], sendline[MAXLINE];
    int RoomID;
    while (true) {
        int n = read(connfd, input, MAXLINE);
        if (n <= 0) {
            printf("Client disconnected while joining Room\n");
            cli_disconnect(connfd);
            return -3;
        }
        if (input[0] == '\0') continue;

        sscanf(input, "%d", &RoomID);
        printf("Join Room ID: %d\n", RoomID);
        for (int i = 0; i <= RoomCount; i++) {
            if (Rooms[i].roomid == RoomID) {
                if (Rooms[i].onlineCount == Rooms[i].capacity) {
                    printf("Room is full\n");
                    bzero(&sendline, MAXLINE);
                    sprintf(sendline, "-2");
                    Write(connfd, sendline, sizeof(sendline));
                    return -2;
                }
                for (int j = 0; j < Rooms[i].capacity; j++) {
                    if (!Rooms[i].onlineStatus[j]) {
                        strcpy(Rooms[i].players[j], username);
                        Rooms[i].onlineStatus[j] = true;
                        Rooms[i].Connfds[j] = connfd;
                        Rooms[i].onlineCount++;
                        break;
                    }
                }
                
                room_print(&Rooms[i]);
                bzero(&sendline, MAXLINE);
                sprintf(sendline, "0");
                Write(connfd, sendline, sizeof(sendline));
                return RoomID;
            }
        }
        printf("Room ID not found\n");
        bzero(&sendline, MAXLINE);
        sprintf(sendline, "-1");
        Write(connfd, sendline, sizeof(sendline));
        return -1;
    }
}


void HandleRoom(int connfd, int roomID, const char* username) {
    printf("Handling room ID: %d\n", roomID);
    Room_t* room = NULL;
    for (int i = 0; i <= RoomCount; i++) {
        if (Rooms[i].roomid == roomID) {
            room = &Rooms[i];
            break;
        }
    }

    while (room->onlineCount < room->capacity) {
        sleep(1);
    }
    printf("start game!, %s\n", username);
    RunGame(room->roomid, room, username);

}

void RunGame(int roomID, Room_t* room, const char* username) {
    printf("helloooooo %s\n", username);
    char sendline[MAXLINE], recvline[MAXLINE];
    for (int i = 0; i < room->capacity; i++) {
        bzero(&sendline, MAXLINE);
        if (strcmp(username, room->players[i]) == 0) {
            sprintf(sendline, "1", 1);
            Write(room->Connfds[i], sendline, strlen(sendline));
            break;
        }
    }
    sleep(2);
    for (int i = 0; i < room->capacity; i++) {
        bzero(&sendline, MAXLINE);
        if (strcmp(username, room->players[i]) == 0) continue;
        sprintf(sendline, "(%s) Helloo, this is %s\n", username, username);
        Write(room->Connfds[i], sendline, strlen(sendline));
    }
    
    while (!room->gameEnd && strcmp(username, room->owner)) sleep(3);

    int maxfdp1 = -1, numFdReady;
    fd_set rset, rset_template;

    FD_ZERO(&rset_template);
    for (int i = 0; i < room->capacity; i++) {
        FD_SET(room->Connfds[i], &rset_template);
        maxfdp1 = max(maxfdp1, room->Connfds[i] + 1);
    }

    while (true) {
        rset = rset_template;
        numFdReady = Select(maxfdp1, &rset, NULL, NULL, NULL);
        // check every client
        for (int i = 0; i < room->capacity; i++) {
            if (room->Connfds[i] == -1 || !FD_ISSET(room->Connfds[i], &rset)) continue;

            bzero(&sendline, sizeof(sendline));
            bzero(&recvline, sizeof(recvline));
            if (!read(room->Connfds[i], recvline, sizeof(recvline))) {
                // client i leave...
                room->onlineCount--;
                room->onlineStatus[i] = false;
                
                // Close the connection and free space
                cli_disconnect(room->Connfds[i]);
                Close(room->Connfds[i]);
                FD_CLR(room->Connfds[i], &rset_template);
                room->Connfds[i] = -1;


                bzero(&sendline, sizeof(sendline));
                if (room->onlineCount <= 1) sprintf(sendline, "(%s left the room. You are the last one. Press Ctrl+D to leave or wait for a new user.\n)", room->players[i]);
                else sprintf(sendline, "(%s left the room. %d users left)\n", room->players[i], room->onlineCount);

                for (int j = 0; j < room->capacity; j++) {
                    if (i != j && room->Connfds[j] != -1) Writen(room->Connfds[j], sendline, strlen(sendline));
                } 
            }
            else {
                sprintf(sendline, "(%s) %s\n", room->players[i], recvline);
                printf("write msg: %s", sendline);
                for (int j = 0; j < room->capacity; j++) {
                    if (i != j && room->Connfds[j] != -1) Writen(room->Connfds[j], sendline, strlen(sendline));
                }
            }
        }

    }

    
}