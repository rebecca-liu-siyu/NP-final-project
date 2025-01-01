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
    
    int flag = 1;
    if (setsockopt(connfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)) < 0) {
        perror("setsockopt(TCP_NODELAY) failed");
    }
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
    bzero(&exist_username, MAXLINE);
    while (fscanf(file, "%s %s %s %s", exist_username, NULL, NULL, NULL) != EOF) {
        if (strcmp(exist_username, username) == 0) {
            printf("Username '%s' already exist!\n", exist_username);
            return 1;
        }
        bzero(&exist_username, MAXLINE);
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
        if (atoi(command) == 1) {
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
            else if (newRoomResult <= -2) continue;
            else {
                // fork a new process to handle the new game room
                printf("You are the host, wait for other to join the room\n");
                HandleRoom(connfd, room->roomid, username);
                continue;
            }

        } 
        else if (atoi(command) == 2) {
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
                continue;
            }
        
        } else if (atoi(command) == 3) {
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
        
        if (atoi(capacity) < 6 || atoi(capacity) > 6) {
            printf("Invalid room capacity: %s\n", capacity);
            bzero(&sendline, MAXLINE);
            sprintf(sendline, "-1");
            Write(connfd, sendline, sizeof(sendline));
            return -4;
        }
        if (strcmp(isPublic, "T") != 0 && strcmp(isPublic, "t") != 0 && strcmp(isPublic, "F") != 0 && strcmp(isPublic, "f") != 0) {
            printf("Invalid room isPublic: %s\n", isPublic);
            bzero(&sendline, MAXLINE);
            sprintf(sendline, "-2");
            Write(connfd, sendline, sizeof(sendline));
            return -3;
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
        Rooms[RoomCount].gameStart = false;

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
                if (Rooms[i].gameStart) {
                    printf("Game started\n");
                    bzero(&sendline, MAXLINE);
                    sprintf(sendline, "-3");
                    Write(connfd, sendline, sizeof(sendline));
                    return -2;
                }
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
    // FIND the room!
    printf("Handling room ID: %d\n", roomID);
    Room_t* room = NULL;
    for (int i = 0; i <= RoomCount; i++) {
        if (Rooms[i].roomid == roomID) {
            room = &Rooms[i];
            break;
        }
    }

    // while (room->onlineCount < room->capacity && room->onlineCount < 2) {
    // WAIT for client_num == capacity!
    while (room->onlineCount < room->capacity) { sleep(1); }
    if (room->capacity == 6) RunGame6(room->roomid, room, username);
}

void RunGame6(int roomID, Room_t* room, const char* username) {
    printf("RunGame start function!: hello %s\n", username);
    
    int my_i = 0;
    char sendline[MAXLINE], recvline[MAXLINE];
    for (int i = 0; i < room->capacity; i++) {
        if (room->onlineStatus[i] == false) continue;
        bzero(&sendline, MAXLINE);
        // 送 game start code 給 "username"
        if (strcmp(username, room->players[i]) == 0) {
            my_i = room->Connfds[i];
            sprintf(sendline, "1"); // game start code: 1
            Write(room->Connfds[i], sendline, strlen(sendline));
            break;
        }
    }
    sleep(1);

    ////////////////////////////////////////////////////////////////
    // 遊戲即將開始
    room->gameStart = true;

    // 確認玩家存活 code: 1
    bzero(&recvline, MAXLINE);
    int n = read(my_i, recvline, MAXLINE);
    if (n <= 0) {
        room->onlineCount--;
        room->onlineStatus[my_i] = false;
        cli_disconnect(room->Connfds[my_i]);
        Close(room->Connfds[my_i]);
        room->Connfds[my_i] = -1;
    }

    ////////////////////////////////////////////////////////////////////////////////
    // 初始遊戲玩家資訊廣播
    // printf("////////////////////////////////////////////\n");
    // printf("initial broadcast:\n");
    // for (int i = 0; i < room->capacity; i++) {
    //     if (room->onlineStatus[i] == false || room->Connfds[i] == my_i) continue;
    //     bzero(&sendline, MAXLINE);
    //     sprintf(sendline, "598 (%s) %s in the room", room->players[i], room->players[i]);
    //     Writen(my_i, sendline, strlen(sendline));
    //     printf("%s\n", sendline);
    //     sleep(1);
    // }
    ////////////////////////////////////////////////////////////////////////////

    // for (int i = 0; i < room->capacity; i++) {
    //     if (room->onlineStatus[i] == false || room->Connfds[i] == my_i) continue;
    //     bzero(&sendline, MAXLINE);
    //     sprintf(sendline, "599 FIN flag", room->players[i], room->players[i]);
    //     Write(my_i, sendline, strlen(sendline));
    //     printf("%s\n", sendline);
    //     sleep(1);
    // }
    
    // if (room->onlineCount > 2) {
    //     for (int i = 0; i < room->capacity; i++) {
    //         if (room->onlineStatus[i] == false || room->Connfds[i] == my_i) continue;
    //         bzero(&sendline, MAXLINE);
    //         sprintf(sendline, "(%s) %s in the room\n", username, username);
    //         Write(room->Connfds[i], sendline, strlen(sendline));
    //         printf("%s", sendline);
    //     }
    // }
    printf("////////////////////////////////////////////\n");

    
    while (!room->gameEnd && strcmp(username, room->owner)) sleep(3);
    if (strcmp(username, room->owner)) return;
    // if (strcmp(username, room->owner)) { printf("///////////\nGAME END\n"); return; }
    int result = Game6(roomID, room);
    /*
    int maxfdp1, numFdReady;
    fd_set rset;

    while (true && !room->gameEnd) {
        maxfdp1 = -1;
        FD_ZERO(&rset);
        for (int i = 0; i < room->capacity; i++) {
            if (room->Connfds[i] == -1) continue;
            FD_SET(room->Connfds[i], &rset);
            maxfdp1 = max(maxfdp1, room->Connfds[i] + 1);
        }

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
                room->Connfds[i] = -1;


                bzero(&sendline, sizeof(sendline));
                // if (room->onlineCount == 0) room->gameEnd = true;
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
    }*/
    printf("RUN GAME result: %s\n", (result) ? "good wins" : "bad wins");
    printf("///////////\nRun GAME END\n");
    room->gameEnd = true;
}

int Game6(int roomID, Room_t* room) {
    int n;
    char sendline[MAXLINE], recvline[MAXLINE];
    int charactor[6];
    bool alive[6] = { true, true, true, true, true, true };
    int wolfs[2] = {-1, -1}, civilians[2] = {-1, -1};
    int guard, seer;

        
    int maxfdp1 = 0, numFdReady;
    fd_set rset, rset_template;
    struct timeval timeOut, timeOut_template;
    timeOut_template.tv_sec = 0;
    timeOut_template.tv_usec = 100000;
    FD_ZERO(&rset);
    FD_ZERO(&rset_template);
    for (int i = 0; i < 6; i++) {
        if (!room->onlineStatus[i]) continue;
        FD_SET(room->Connfds[i], &rset_template);
        maxfdp1 = max(maxfdp1, room->Connfds[i] + 1);
    }
    
    
    // FIN of 初始遊戲玩家資訊廣播
    for (int i = 0; i < room->capacity; i++) {
        if (room->onlineStatus[i] == false) continue;
        sleep(1);
        bzero(&sendline, MAXLINE);
        sprintf(sendline, "599 FIN flag", room->players[i], room->players[i]);
        write(room->Connfds[i], sendline, strlen(sendline));
        printf("%s\n", sendline);
    }



    // 發布玩家名單
    printf("\n發布玩家名單:\n");
    sleep(1.5);
    for (int i = 0; i < room->capacity; i++) {
        if (!room->onlineStatus[i] || room->Connfds[i] == -1) continue;
        
        bzero(&sendline, MAXLINE);
        sprintf(sendline, "599 %d %s\n", i, room->players[i]);
        printf("send: %s", sendline);
        broadcastMSG(room, sendline);
        sleep(3);//sleepmore
    }
    printf("//////////////////////////////////\n");


    printf("600 FLAG\n");
    broadcastMSG(room, "600\n");
    // for (int i = 0; i < room->capacity; i++) {
    //     if (!room->onlineStatus[i]) continue;
    //     fsync(room->Connfds[i]);
    //     sleep(1);
    //     Writen(room->Connfds[i], "600\n", 3);

    // }
    printf("//////////////////////////////////\n");

    // 分配身分
    // 預言家+守衛+2平民 v.s. 2狼人
    printf("分配身分\n");
    sleep(2);
    srand(time(0)); // 初始化隨機數生成器
    int numberCheck[6] = { 0, 0, 0, 0, 0, 0 };
    for (int i = 0; i < 6; i++) {
        int randNum = rand() % 6;
        while (numberCheck[randNum]) {
            randNum = rand() % 6;
        }
        charactor[i] = randNum;
        numberCheck[randNum] = 1; // 標記該數字已經使用
        printf("charactor[%d]: %d\n", i, charactor[i]);
    }
    for (int i = 0; i < 6; i++) printf("charactor[%d]: %d\n", i, charactor[i]);
    printf("//////////////////////////////////\n");
    // send role to players
    for (int j = 0; j < 6; j++) {
        // charactor[i]: Player i 的角色是: rolecode 'charactor[i]'
        if (charactor[j] == 0 || charactor[j] == 1) wolfs[charactor[j]] = j;
        else if (charactor[j] == 2) seer = j;
        else if (charactor[j] == 3) guard = j;
        else civilians[charactor[j] - 4] = j;


        if (!room->onlineStatus[j]) continue;
        sleep(1);
        bzero(&sendline, MAXLINE);
        printf("600 %d\t", charactor[j]);
        sprintf(sendline, "600 %d", charactor[j]); // role code: 600
        writen(room->Connfds[j], sendline, strlen(sendline));
        printf("send: %s to %s\n", sendline, room->players[j]);

    }
    printf("//////////////////////////////////\n");


    ////////////////////////////////////////////////////////////////////////////
    // broadcast game start!
    int round = 1;
    int killed;
    int Game6_winnergroup = 0;
    while (!Game6_winnergroup) {
        // round i NIGHT
        sleep(1);
        bzero(&sendline, MAXLINE);
        sprintf(sendline, "666 %d 1 0", round); // start game code: 666
        rset = rset_template;
        timeOut = timeOut_template;
        numFdReady = Select(maxfdp1, &rset, NULL, NULL, &timeOut);
        printf("\nGame6 check 1: %d ready\n", numFdReady);
        for (int i = 0; i < 6; i++) {
            if (!room->onlineStatus[i] || !FD_ISSET(room->Connfds[i], &rset)) continue;
            bzero(&recvline, MAXLINE);
            n = read(room->Connfds[i], recvline, MAXLINE);
            if (n <= 0) {
                room->onlineCount--;
                room->onlineStatus[i] = false;
                alive[i] = false;

                // Close the connection and free space
                cli_disconnect(room->Connfds[i]);
                Close(room->Connfds[i]);
                FD_CLR(room->Connfds[i], &rset_template);
                room->Connfds[i] = -1;
            }
            else printf("player %d (%s): %s\n", i + 1, room->players[i], recvline);
        }
        printf("Game6 check 1 END====\n\n");
        broadcastMSG(room, sendline);
        
        
        printf("發動技能time\n");
        //////////////////////////////////////////////////////////////////////
        // GUARD
        // 發動技能time
        int safed;
        sleep(1);
        printf("GUARD 的決定是?\n");
        bzero(&sendline, MAXLINE);
        sprintf(sendline, "666 %d 1 3 %d -1", round, guard); // guard time
        rset = rset_template;
        timeOut = timeOut_template;
        numFdReady = Select(maxfdp1, &rset, NULL, NULL, &timeOut);
        printf("\nGame6 check 2: %d ready\n", numFdReady);
        for (int i = 0; i < 6; i++) {
            if (!room->onlineStatus[i] || !FD_ISSET(room->Connfds[i], &rset)) continue;
            bzero(&recvline, MAXLINE);
            n = read(room->Connfds[i], recvline, MAXLINE);
            if (n <= 0) {
                room->onlineCount--;
                room->onlineStatus[i] = false;
                alive[i] = false;

                // Close the connection and free space
                cli_disconnect(room->Connfds[i]);
                Close(room->Connfds[i]);
                room->Connfds[i] = -1;

                Game6_winnergroup = Game6_EndCheck(charactor, alive);
                if (Game6_winnergroup) break;
            }
            else printf("player %d (%s): %s\n", i + 1, room->players[i], recvline);
        }
        printf("Game6 check 2 END====\n\n");
        broadcastMSG(room, sendline);

        if (!alive[guard]) { 
            safed = -1; 
            sleep(5); 
            rset = rset_template;
            timeOut = timeOut_template;
            numFdReady = Select(maxfdp1, &rset, NULL, NULL, &timeOut);
            printf("\nGame6 check 3.1: %d ready\n", numFdReady);
            for (int i = 0; i < 6; i++) {
                if (!room->onlineStatus[i] || !FD_ISSET(room->Connfds[i], &rset)) continue;
                bzero(&recvline, MAXLINE);
                n = read(room->Connfds[i], recvline, MAXLINE);
                if (n <= 0) {
                    room->onlineCount--;
                    room->onlineStatus[i] = false;
                    alive[i] = false;

                    // Close the connection and free space
                    cli_disconnect(room->Connfds[i]);
                    Close(room->Connfds[i]);
                    FD_CLR(room->Connfds[i], &rset_template);
                    room->Connfds[i] = -1;

                    Game6_winnergroup = Game6_EndCheck(charactor, alive);
                    if (Game6_winnergroup) break;
                }
                else printf("player %d (%s): %s\n", i + 1, room->players[i], recvline);
            }
            printf("Game6 check 3.1 END====\n\n");
            broadcastMSG(room, "0"); 
            sleep(1); 
        }
        else {
            // 守衛要守護誰?
            bzero(&recvline, MAXLINE);
            n = read(room->Connfds[guard], recvline, MAXLINE);
            if (n <= 0) { // client guard leave
                room->onlineCount--;
                room->onlineStatus[guard] = false;
                alive[guard] = false;

                // Close the connection and free space
                cli_disconnect(room->Connfds[guard]);
                Close(room->Connfds[guard]);
                FD_CLR(room->Connfds[guard], &rset_template);
                room->Connfds[guard] = -1;

                Game6_winnergroup = Game6_EndCheck(charactor, alive);
                if (Game6_winnergroup) break;
            }
            else {
                safed = atoi(recvline);
                printf("Round %d: guard safe - player %d (%s)\n", round, safed + 1, room->players[safed]);
                bzero(&sendline, MAXLINE);
                sprintf(sendline, "%d", safed);
                // 回傳ack
                rset = rset_template;
                timeOut = timeOut_template;
                numFdReady = Select(maxfdp1, &rset, NULL, NULL, &timeOut);
                printf("\nGame6 check 3.2: %d ready\n", numFdReady);
                for (int i = 0; i < 6; i++) {
                    if (!room->onlineStatus[i] || !FD_ISSET(room->Connfds[i], &rset)) continue;
                    bzero(&recvline, MAXLINE);
                    n = read(room->Connfds[i], recvline, MAXLINE);
                    if (n <= 0) {
                        room->onlineCount--;
                        room->onlineStatus[i] = false;
                        alive[i] = false;

                        // Close the connection and free space
                        cli_disconnect(room->Connfds[i]);
                        Close(room->Connfds[i]);
                        FD_CLR(room->Connfds[i], &rset_template);
                        room->Connfds[i] = -1;

                        Game6_winnergroup = Game6_EndCheck(charactor, alive);
                        if (Game6_winnergroup) break;
                    }
                    else printf("player %d (%s): %s\n", i + 1, room->players[i], recvline);
                }
                printf("Game6 check 3.2 END====\n\n");
                broadcastMSG(room, sendline);
                sleep(1);
            }
        }
        

        //////////////////////////////////////////////////////////////////////
        // WOLF
        // 發動技能time
        sleep(2);
        bzero(&sendline, MAXLINE);
        sprintf(sendline, "666 %d 1 1 %d %d", round, wolfs[0], wolfs[1]); // wolf time
        rset = rset_template;
        timeOut = timeOut_template;
        numFdReady = Select(maxfdp1, &rset, NULL, NULL, &timeOut);
        printf("\nGame6 check 4: %d ready\n", numFdReady);
        for (int i = 0; i < 6; i++) {
            if (!room->onlineStatus[i] || !FD_ISSET(room->Connfds[i], &rset)) continue;
            bzero(&recvline, MAXLINE);
            n = read(room->Connfds[i], recvline, MAXLINE);
            if (n <= 0) {
                room->onlineCount--;
                room->onlineStatus[i] = false;
                alive[i] = false;

                // Close the connection and free space
                cli_disconnect(room->Connfds[i]);
                Close(room->Connfds[i]);
                FD_CLR(room->Connfds[i], &rset_template);
                room->Connfds[i] = -1;

                Game6_winnergroup = Game6_EndCheck(charactor, alive);
                if (Game6_winnergroup) break;
            }
            else printf("player %d (%s): %s\n", i + 1, room->players[i], recvline);
        }
        printf("Game6 check 4 END====\n\n");
        broadcastMSG(room, sendline);

        // get wolf decision
        printf("Wolf 的決定是?\n");
        bool hasKill = false;

        while (!hasKill) {
            FD_ZERO(&rset);
            if (alive[wolfs[0]]) FD_SET(room->Connfds[wolfs[0]], &rset);
            if (alive[wolfs[1]]) FD_SET(room->Connfds[wolfs[1]], &rset);
            if (alive[civilians[0]]) FD_SET(room->Connfds[civilians[0]], &rset);
            if (alive[civilians[1]]) FD_SET(room->Connfds[civilians[1]], &rset);
            if (alive[seer]) FD_SET(room->Connfds[seer], &rset);
            if (alive[guard]) FD_SET(room->Connfds[guard], &rset);

            numFdReady = Select(maxfdp1, &rset, NULL, NULL, &timeOut);
            if (!alive[wolfs[0]]) { if (!FD_ISSET(room->Connfds[wolfs[1]], &rset)) continue; }
            else if (!alive[wolfs[1]]) { if (!FD_ISSET(room->Connfds[wolfs[0]], &rset)) continue; }
            else { // 2 wolf alive
                if (!FD_ISSET(room->Connfds[wolfs[1]], &rset) || !FD_ISSET(room->Connfds[wolfs[0]], &rset))
                    continue;
            }
            
            // 可以回應的都回應了!
            printf("wolf 可以回應的都回應了!");
            if (FD_ISSET(room->Connfds[wolfs[1]], &rset)) {
                bzero(&recvline, MAXLINE);
                n = read(room->Connfds[wolfs[1]], recvline, MAXLINE);
                if (n <= 0) {
                    room->onlineCount--;
                    room->onlineStatus[wolfs[1]] = false;
                    alive[wolfs[1]] = false;

                    // Close the connection and free space
                    cli_disconnect(room->Connfds[wolfs[1]]);
                    Close(room->Connfds[wolfs[1]]);
                    FD_CLR(room->Connfds[wolfs[1]], &rset_template);
                    room->Connfds[wolfs[1]] = -1;

                    Game6_winnergroup = Game6_EndCheck(charactor, alive);
                    if (Game6_winnergroup) break;
                }
                printf("read: %s (from %s)\n", recvline, room->players[wolfs[1]]);
                killed = atoi(recvline);
                hasKill = true;
            }
            if (FD_ISSET(room->Connfds[wolfs[0]], &rset)) {
                bzero(&recvline, MAXLINE);
                n = read(room->Connfds[wolfs[0]], recvline, MAXLINE);
                if (n <= 0) {
                    room->onlineCount--;
                    room->onlineStatus[wolfs[0]] = false;
                    alive[wolfs[0]] = false;

                    // Close the connection and free space
                    cli_disconnect(room->Connfds[wolfs[0]]);
                    Close(room->Connfds[wolfs[0]]);
                    FD_CLR(room->Connfds[wolfs[0]], &rset_template);
                    room->Connfds[wolfs[0]] = -1;

                    Game6_winnergroup = Game6_EndCheck(charactor, alive);
                    if (Game6_winnergroup) break;
                }
                printf("read: %s (from %s)\n", recvline, room->players[wolfs[1]]);
                killed = atoi(recvline);
                hasKill = true;
            }
            printf("\nGame6 check 5: %d ready\n", numFdReady - 2);
            /*if (FD_ISSET(room->Connfds[seer], &rset)) {
                bzero(&recvline, MAXLINE);
                n = read(room->Connfds[seer], recvline, MAXLINE);
                if (n <= 0) {
                    room->onlineCount--;
                    room->onlineStatus[seer] = false;
                    alive[seer] = false;

                    // Close the connection and free space
                    cli_disconnect(room->Connfds[seer]);
                    Close(room->Connfds[seer]);
                    FD_CLR(room->Connfds[seer], &rset_template);
                    room->Connfds[seer] = -1;
                }
                else printf("player %d (%s): %s\n", seer + 1, room->players[seer], recvline);
            }
            if (FD_ISSET(room->Connfds[guard], &rset)) {
                bzero(&recvline, MAXLINE);
                n = read(room->Connfds[guard], recvline, MAXLINE);
                if (n <= 0) {
                    room->onlineCount--;
                    room->onlineStatus[guard] = false;
                    alive[guard] = false;

                    // Close the connection and free space
                    cli_disconnect(room->Connfds[guard]);
                    Close(room->Connfds[guard]);
                    FD_CLR(room->Connfds[guard], &rset_template);
                    room->Connfds[guard] = -1;
                }
                else printf("player %d (%s): %s\n", guard + 1, room->players[guard], recvline);
            }
            if (FD_ISSET(room->Connfds[civilians[0]], &rset)) {
                bzero(&recvline, MAXLINE);
                n = read(room->Connfds[civilians[0]], recvline, MAXLINE);
                if (n <= 0) {
                    room->onlineCount--;
                    room->onlineStatus[civilians[0]] = false;
                    alive[civilians[0]] = false;

                    // Close the connection and free space
                    cli_disconnect(room->Connfds[civilians[0]]);
                    Close(room->Connfds[civilians[0]]);
                    FD_CLR(room->Connfds[civilians[0]], &rset_template);
                    room->Connfds[civilians[0]] = -1;
                }
                else printf("player %d (%s): %s\n", civilians[0] + 1, room->players[civilians[0]], recvline);
            }
            if (FD_ISSET(room->Connfds[civilians[1]], &rset)) {
                bzero(&recvline, MAXLINE);
                n = read(room->Connfds[civilians[1]], recvline, MAXLINE);
                if (n <= 0) {
                    room->onlineCount--;
                    room->onlineStatus[civilians[1]] = false;
                    alive[civilians[1]] = false;

                    // Close the connection and free space
                    cli_disconnect(room->Connfds[civilians[1]]);
                    Close(room->Connfds[civilians[1]]);
                    FD_CLR(room->Connfds[civilians[1]], &rset_template);
                    room->Connfds[civilians[1]] = -1;
                }
                else printf("player %d (%s): %s\n", civilians[1] + 1, room->players[civilians[1]], recvline);
            }*/
            printf("Game6 check 5 END====\n\n");
        }
        printf("Round %d: wolfs kill - player %d (%s)\n", round, killed + 1, room->players[killed]);
        bzero(&sendline, MAXLINE);
        sprintf(sendline, "%d", killed + 1);
        rset = rset_template;
        timeOut = timeOut_template;
        numFdReady = Select(maxfdp1, &rset, NULL, NULL, &timeOut);
        printf("\nGame6 check 6: %d ready\n", numFdReady);
        for (int i = 0; i < 6; i++) {
            if (!room->onlineStatus[i] || !FD_ISSET(room->Connfds[i], &rset)) continue;
            bzero(&recvline, MAXLINE);
            n = read(room->Connfds[i], recvline, MAXLINE);
            if (n <= 0) {
                room->onlineCount--;
                room->onlineStatus[i] = false;
                alive[i] = false;

                // Close the connection and free space
                cli_disconnect(room->Connfds[i]);
                Close(room->Connfds[i]);
                FD_CLR(room->Connfds[i], &rset_template);
                room->Connfds[i] = -1;

                Game6_winnergroup = Game6_EndCheck(charactor, alive);
                if (Game6_winnergroup) break;
            }
            else printf("player %d (%s): %s\n", i + 1, room->players[i], recvline);
        }
        printf("Game6 check 6 END====\n\n");
        broadcastMSG(room, sendline);
        sleep(2);

        printf("Guard check: ");
        if (killed == safed) { printf("safe successful!\n"); killed = -1; }
        else { printf("safe fail!\n"); }

        //////////////////////////////////////////////////////////////////////
        // SEER
        // 發動技能time
        printf("SEER 的決定是?\n");
        bzero(&sendline, MAXLINE);
        sprintf(sendline, "666 %d 1 2 %d -1", round, seer); // seer time
        rset = rset_template;
        timeOut = timeOut_template;
        numFdReady = Select(maxfdp1, &rset, NULL, NULL, &timeOut);
        printf("\nGame6 check 7: %d ready\n", numFdReady);
        for (int i = 0; i < 6; i++) {
            if (!room->onlineStatus[i] || !FD_ISSET(room->Connfds[i], &rset)) continue;
            bzero(&recvline, MAXLINE);
            n = read(room->Connfds[i], recvline, MAXLINE);
            if (n <= 0) {
                room->onlineCount--;
                room->onlineStatus[i] = false;
                alive[i] = false;

                // Close the connection and free space
                cli_disconnect(room->Connfds[i]);
                Close(room->Connfds[i]);
                FD_CLR(room->Connfds[i], &rset_template);
                room->Connfds[i] = -1;

                Game6_winnergroup = Game6_EndCheck(charactor, alive);
                if (Game6_winnergroup) break;
            }
            else printf("player %d (%s): %s\n", i + 1, room->players[i], recvline);
        }
        printf("Game6 check 7 END====\n\n");
        broadcastMSG(room, sendline);
            
        if (!alive[seer]) { 
            sleep(5); 
            broadcastMSG(room, "2"); 
            sleep(1); 
        }
        else {
            // 預言家要問誰?
            bzero(&recvline, MAXLINE);
            n = read(room->Connfds[seer], recvline, MAXLINE);
            if (n <= 0) { // client seer leave
                room->onlineCount--;
                room->onlineStatus[seer] = false;
                alive[seer] = false;
                
                // Close the connection and free space
                cli_disconnect(room->Connfds[seer]);
                Close(room->Connfds[seer]);
                FD_CLR(room->Connfds[seer], &rset_template);
                room->Connfds[seer] = -1;

                Game6_winnergroup = Game6_EndCheck(charactor, alive);
                if (Game6_winnergroup) break;
            }
            else {
                int asked = atoi(recvline);
                printf("Round %d: seer ask - player %d (%s)\n", round, asked + 1, room->players[asked]);
                printf("\t---Player %d (%s) is %s\n", asked + 1, room->players[asked], (asked == wolfs[0] || asked == wolfs[1]) ? "bad" : "good");
                bzero(&sendline, MAXLINE);
                sprintf(sendline, "%d", (asked == wolfs[0] || asked == wolfs[1]) ? 0 : 1);
                // bad: 0, good: 1
                // 回傳結果
                rset = rset_template;
                timeOut = timeOut_template;
                numFdReady = Select(maxfdp1, &rset, NULL, NULL, &timeOut);
                printf("\nGame6 check 8: %d ready\n", numFdReady);
                for (int i = 0; i < 6; i++) {
                    if (!room->onlineStatus[i] || !FD_ISSET(room->Connfds[i], &rset)) continue;
                    bzero(&recvline, MAXLINE);
                    n = read(room->Connfds[i], recvline, MAXLINE);
                    if (n <= 0) {
                        room->onlineCount--;
                        room->onlineStatus[i] = false;
                        alive[i] = false;

                        // Close the connection and free space
                        cli_disconnect(room->Connfds[i]);
                        Close(room->Connfds[i]);
                        FD_CLR(room->Connfds[i], &rset_template);
                        room->Connfds[i] = -1;

                        Game6_winnergroup = Game6_EndCheck(charactor, alive);
                        if (Game6_winnergroup) break;
                    }
                    else printf("player %d (%s): %s\n", i + 1, room->players[i], recvline);
                }
                printf("Game6 check 8 END====\n\n");
                broadcastMSG(room, sendline);
                sleep(1);
            }
        }
    /////////////////////////////////////////////////////////////////////////////
    // day comes
        printf("\x1B[0;33m\n天亮了\n\x1B[0m");
        bzero(&sendline, MAXLINE);
        sprintf(sendline, "%d", killed + 1);
        rset = rset_template;
        timeOut = timeOut_template;
        numFdReady = Select(maxfdp1, &rset, NULL, NULL, &timeOut);
        printf("\nGame6 check 9: %d ready\n", numFdReady);
        for (int i = 0; i < 6; i++) {
            if (!room->onlineStatus[i] || !FD_ISSET(room->Connfds[i], &rset)) continue;
            bzero(&recvline, MAXLINE);
            n = read(room->Connfds[i], recvline, MAXLINE);
            if (n <= 0) {
                room->onlineCount--;
                room->onlineStatus[i] = false;
                alive[i] = false;

                // Close the connection and free space
                cli_disconnect(room->Connfds[i]);
                Close(room->Connfds[i]);
                FD_CLR(room->Connfds[i], &rset_template);
                room->Connfds[i] = -1;

                Game6_winnergroup = Game6_EndCheck(charactor, alive);
                if (Game6_winnergroup) break;
            }
            else printf("player %d (%s): %s\n", i + 1, room->players[i], recvline);
        }
        printf("Game6 check 9 END====\n\n");
        broadcastMSG(room, sendline);
        printf("殺人結果已送出\n");
        sleep(2);
        alive[killed] = false;

        Game6_winnergroup = Game6_EndCheck(charactor, alive);
        if (Game6_winnergroup) break;

    // msg exchange
    /*
    int maxfdp1, numFdReady;
    fd_set rset;

    while (true && !room->gameEnd) {
        maxfdp1 = -1;
        FD_ZERO(&rset);
        for (int i = 0; i < room->capacity; i++) {
            if (room->Connfds[i] == -1) continue;
            FD_SET(room->Connfds[i], &rset);
            maxfdp1 = max(maxfdp1, room->Connfds[i] + 1);
        }

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
                room->Connfds[i] = -1;


                bzero(&sendline, sizeof(sendline));
                // if (room->onlineCount == 0) room->gameEnd = true;
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
    }*/

        round++;
    }
    bzero(&sendline, MAXLINE);
    sprintf(sendline, "667 %d 1 0", Game6_winnergroup); // end game code: 667
    broadcastMSG(room, sendline);
    return (Game6_winnergroup == 1); 
}


void broadcastMSG(Room_t* room, const char* sendline) {
    for (int i = 0; i < room->capacity; i++) {
        if (!room->onlineStatus[i]) continue;
        writen(room->Connfds[i], sendline, strlen(sendline));
    }
}

// 0: for game conti, 1: good win, 2: bad win
int Game6_EndCheck(int* charactor, bool* alive) {
    bool GroupBad_remain = false, GroupGood_remain = false;
    for (int i = 0; i < 6; i++) {
        if (charactor[i] <= 1 && alive[i]) GroupBad_remain = true;
        else if (charactor[i] >= 2 && alive[i]) GroupGood_remain = true;
    }
    if (GroupGood_remain && GroupBad_remain) return 0;
    if (GroupGood_remain) return 1;
    return 2;
}
