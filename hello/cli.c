#include "unp.h"
#include "cli.h"
#include <stdbool.h>
#include <stdio.h>

struct sockaddr_in sockaddr_setup(short family, int port, int addr) {
    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = family;
    serv_addr.sin_addr.s_addr = addr;
    serv_addr.sin_port = htons(port);
    return serv_addr;
}

void signalHandle(int sig_num) {
    signal(SIGINT, signalHandle);
    printf("\x1B[0;31m\n==Ctrl + C cannot terminate the program!==\n\x1B[0m");
    fflush(stdout);
}

int main(int argc, char** argv) {
    // signal(SIGINT, signalHandle);
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server IP> <server port>\n", argv[0]);
        exit(1);
    }

    // Create socket
    int sock_fd = Socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr = sockaddr_setup(AF_INET, atoi(argv[2]), htonl(INADDR_ANY));
    Inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);

    // Connect to server
    Connect(sock_fd, (SA*)&serv_addr, sizeof(serv_addr));
    printf("Connected to server at %s:%s\n", argv[1], argv[2]);

    char input[MAXLINE], recvline[MAXLINE];
    char command[10], username[50], password[50];
    while (1) {
        // Display menu
        printf("\nMenu:\n");
        printf("1. Register (Usage: register <username> <password>)\n");
        printf("2. Login (Usage: login <username> <password>)\n");
        printf("3. Exit\n");
        printf("Enter command: ");

        // Get user input
        bzero(&input, MAXLINE);
        fgets(input, MAXLINE, stdin);

        // Exit if user enters "3" or "exit"
        if (strncmp(input, "3", 1) == 0 || strncmp(input, "exit", 4) == 0) {
            printf("Exiting...\n");
            close(sock_fd);
            return 0;
        }

        bzero(&command, 10);
        bzero(&username, 50);
        bzero(&password, 50);
        sscanf(input, "%s %s %s", command, username, password);
        printf("Command: %s, Username: %s, Password: %s\n", command, username, password);

        // Send command to server
        Writen(sock_fd, input, strlen(input));
        printf("Sent command: %s", input);

        // Read server response
        bzero(&recvline, MAXLINE);
        int n = read(sock_fd, recvline, MAXLINE);
        if (n <= 0) {
            printf("Server terminated prematurely\n");
            close(sock_fd);
            return -1;
        }
        if (strcmp(recvline, "1") == 0) {
            printf("Invalid command format!\n");
            continue;
        }
        else if (strcmp(recvline, "2") == 0) {
            printf("Register successful!\nPlease login by command code '2'!\n");
            continue;
        }
        else if (strcmp(recvline, "3") == 0) {
            printf("Username already exist!\n");
            continue;
        }
        else if (strcmp(recvline, "4") == 0) {
            printf("Login successful!\nWelcome to the Lobby!, %s\n", username);
            break;
        }
        else if (strcmp(recvline, "5") == 0) {
            printf("User already login via other device!\n");
            continue;
        }
        else if (strcmp(recvline, "6") == 0) {
            printf("Wrong Username or password!\n");
            continue;
        }
        else if (strcmp(recvline, "7") == 0) {
            printf("Invalid command code!\n");
            continue;
        }
        else {
            printf("Invalid response from server: %s\n", recvline);
            continue;
        }
    }

    Lobby(sock_fd, username);
    // Close socket
    close(sock_fd);
    return 0;
}

int Lobby(int sock_fd, const char* username) {
    char input[MAXLINE], sendline[MAXLINE];
    while(true) {
        fflush(STDIN_FILENO);
        printf("\n=====================Lobby=====================\n");
        printf("Now, You can enter a game room\n");
        printf("1. New Room\n");
        printf("2. Join Room\n");
        printf("3. Exit\n");
        printf("Enter command: ");
        bzero(&input, MAXLINE);
        fgets(input, MAXLINE, stdin);
        printf("input: %s\n", input);

        if (strcmp(input, "1\n") == 0) {
            printf("New Room\n");
            bzero(&sendline, MAXLINE);
            sprintf(sendline, "1");
            Write(sock_fd, sendline, sizeof(sendline));
            int newRoomResult = NewRoom(sock_fd, username);
            if (newRoomResult <= -1) {
                continue;
            }
            else if(newRoomResult == 0) { Game(sock_fd, username); continue; }
            else { printf("Server error\n"); return 0;}
            // break;
        } else if (strcmp(input, "2\n") == 0) {
            printf("Join Room\n");
            bzero(&sendline, MAXLINE);
            sprintf(sendline, "2");
            Write(sock_fd, sendline, sizeof(sendline));
            int joinRoomResult = JoinRoom(sock_fd, username);
            if (joinRoomResult == -1) continue;
            else if (joinRoomResult == 0) { Game(sock_fd, username); continue; }
            // break;
        } else if (strcmp(input, "3\n") == 0) {
            printf("Exit\n");
            bzero(&sendline, MAXLINE);
            sprintf(sendline, "3");
            Write(sock_fd, sendline, sizeof(sendline));
            return 0;
        } else {
            printf("Invalid command\n");
        }
    }
}

int NewRoom(int sock_fd, const char* username) {
    char input[MAXLINE], sendline[MAXLINE], recvline[MAXLINE];
    char RoomName[50];
    int RoomCapacity;
    char isPublic;
    while(true) {
        printf("\nNew Room Setup:\n");
        printf("1. Set Room Name:\n");
        fgets(RoomName, 50, stdin);
        printf("2. Set Room Capacity (6 people supported right now)\n");
        scanf("%d", &RoomCapacity); getchar();
        printf("3. Set Room to Public? (T/F)\n");
        scanf("%c", &isPublic); getchar();

        bzero(&sendline, MAXLINE);
        sprintf(sendline, "%s %d %c", RoomName, RoomCapacity, isPublic);
        Write(sock_fd, sendline, sizeof(sendline));
        
        bzero(&recvline, MAXLINE);
        Read(sock_fd, recvline, MAXLINE);
        // printf("Server response: %s\n", recvline);
        if (atoi(recvline) > 0 && atoi(recvline) < 3000) {
            printf("Room created successful\n");
            printf("Room ID: %d\n", atoi(recvline));
            return 0;
        }
        else if (atoi(recvline) == -1) {
            printf("Invalid room capacity\n");
            return -3;
        } 
        else if (atoi(recvline) == -2) {
            printf("Invalid room isPublic - please enter 'T' or 'F'\n");
            return -2;
        }
        else if (atoi(recvline) == -3) {
            printf("Room is full\n");
            return -1;
        }
        else {
            printf("Room creation failed!\n");
            continue;
        }
    }
}

int JoinRoom(int sock_fd, const char* username) {
    char input[MAXLINE], sendline[MAXLINE], recvline[MAXLINE];
    int RoomID;
    while (true) {
        printf("Join Room:\n");
        printf("Enter Room ID: ");
        scanf("%d", &RoomID); getchar();
        bzero(&sendline, MAXLINE);
        sprintf(sendline, "%d", RoomID);
        Write(sock_fd, sendline, sizeof(sendline));
        int n = read(sock_fd, recvline, MAXLINE);
        if (n <= 0) {
            printf("Server terminated prematurely\n");
            close(sock_fd);
            return -2;
        }

        if (atoi(recvline) == 0) {
            printf("Join Room successful\n");
            return 0;
        }
        else if (atoi(recvline) == -1) {
            printf("Room ID not found\n");
            break;
        }
        else if (atoi(recvline) == -2) {
            printf("Room is full\n");
            break;
        }
        else if (atoi(recvline) == -3) {
            printf("Game started already");
            break;
        }
        else {
            printf("Join Room failed\n");
            break;
        }
    }
    return -1;
}

void Game(int sock_fd, const char* username) {
    printf("Wait for the game to start\n");
    char recvline[MAXLINE];
    while (true) {
        bzero(&recvline, MAXLINE);
        int n = read(sock_fd, recvline, MAXLINE);
        if (atoi(recvline) == 1) break;
    }

    // when game start: server broadcast code 1
    printf("Game started\n");
    // xchg_data(sock_fd, stdin);
    int gameResult = Game6(sock_fd, stdin, username);
    printf("Game end\n");
    return;
}

void xchg_data(int sock_fd, FILE *fp) {
    int maxfdp1, stdineof, peer_exit, n, idx;
    fd_set rset;
    char sendline[MAXLINE], recvline[MAXLINE];

    stdineof = 0;
	peer_exit = 0;
    Writen(sock_fd, "1", 1);

    while (true) {
        FD_ZERO(&rset);
        maxfdp1 = 0;
        if (stdineof == 0) {
            FD_SET(fileno(fp), &rset);
            maxfdp1 = fileno(fp) + 1;
        }
        if (peer_exit == 0) {
            FD_SET(sock_fd, &rset);
            maxfdp1 = max(maxfdp1, sock_fd + 1);
        }

        Select(maxfdp1, &rset, NULL, NULL, NULL);
        if (FD_ISSET(sock_fd, &rset)) { // socket is readable
        bzero(&recvline, MAXLINE);
            n = read(sock_fd, recvline, MAXLINE);
            if (n == 0) {
                if (stdineof == 1) {
                    printf("normal termination\n");
                    return;
                }
                else {
                    printf("end of input from the peer\n");
                    peer_exit = 1;
                    return;
                }
            }
            else if (n > 0) {
                recvline[n] = '\0';
                printf("\x1B[0;36m%s\x1B[0m", recvline);
				fflush(stdout);
            }
            else {
                printf("(server down)\n");
                return;
            }
        }
        if (FD_ISSET(fileno(fp), &rset)) {
            bzero(&sendline, MAXLINE);
            if (Fgets(sendline, MAXLINE, fp) == NULL) {
                if (peer_exit) return;
                else {
                    printf("(LEAVING...)\n");
                    stdineof = 1;
                    Shutdown(sock_fd, SHUT_WR);
                }
            }
            else {
                n = strlen(sendline);
                sendline[n] = '\0';
                Writen(sock_fd, sendline, n + 2);
                printf(" - write: %s", sendline);
            }
        }
    }
}

int Game6(int sock_fd, FILE* fp, const char* username) {
    int maxfdp1, stdineof, peer_exit, n, idx, my_i;
    int roleCode, gameCode, roundCode, DayNight, myRole;
    char playersName[6][100];
    bool alive[6] = { true, true, true, true, true, true};
    bool my_alive = true;
    fd_set rset;

    char sendline[MAXLINE], recvline[MAXLINE];

    stdineof = 0;
    peer_exit = 0;
    FD_ZERO(&rset);
    
    // 確認玩家存活 code: 1
    Writen(sock_fd, "1", 1);

    ////////////////////////////////////////////////////////////////////////////////////////
    // 初始遊戲玩家資訊廣播
    printf("初始遊戲玩家資訊廣播\n");
    char msg1[100], msg2[100], code[10];
    do {
        bzero(&recvline, MAXLINE);
        bzero(&msg1, 100);
        bzero(&msg2, 100);
        bzero(&code, 10);
        n = read(sock_fd, recvline, MAXLINE);
        // printf("recv: %s\n", recvline);
        sscanf(recvline, "%s %s %s", code, msg1, msg2);
        gameCode = atoi(code);
        // printf("gamecode: %s -> atoi: %d\n", code, gameCode);
        if (gameCode != 598) break;
        printf("\x1B[0;36m(%s) %s in the room\n\x1B[0m", msg2, msg2);
    } while (gameCode == 598);
    printf("////////////////////////////////////////////\n");
    // Write(sock_fd, "-598", 4);

    ////////////////////////////////////////////////////////////////////////////////////////
    // PLAYERS NAME
    gameCode = 599;
    printf("PLAYERS NAME:\n");
    do {
        bzero(&recvline, MAXLINE);
        bzero(&msg1, 100);
        bzero(&msg2, 100);
        bzero(&code, 10);
        n = Read(sock_fd, recvline, MAXLINE);
        // printf("recv: %s", recvline);
        sscanf(recvline, "%s %s %s", code, msg1, msg2);
        gameCode = atoi(code);
        idx = atoi(msg1);
        if (gameCode != 599) break;
        strcpy(playersName[idx], msg2);
        printf("\x1B[0;33mPlayer %d: %s\n\x1B[0m", idx + 1, playersName[idx]);
        if (strcmp(username, playersName[idx]) == 0) my_i = idx;
    } while (gameCode == 599);
    printf("////////////////////////////////////////////\n");


    // ROLE ASSIGNMENT
    printf("serv: role assigning...\n");
    bzero(&recvline, MAXLINE);
    n = read(sock_fd, recvline, MAXLINE);
    printf("serv: %s\n", recvline);
    sscanf(recvline, "%d %d", &gameCode, &roleCode);
    myRole = roleCode;
    if (gameCode != 600) {
        printf("server gameCode failure(role assignment) - code: %d\n", gameCode);
        return;
    }

    if (myRole == 0) {
        printf("你的身分是: '狼人(狼王)'\n");
    }
    else if (myRole == 1) {
        printf("你的身分是: '狼人'\n");
    }
    else if (myRole == 2) {
        printf("你的身分是: '預言家'\n");
    }
    else if (myRole == 3) {
        printf("你的身分是: '守衛'\n");
    }
    else if (myRole == 4) {
        printf("你的身分是: '平民'\n");
    }
    else if (myRole == 5) {
        printf("你的身分是: '平民'\n");
    }
    ///////////////////////////////////////////////////////////////
    printf("////////////////////////////////\n");
    printf("GAME START:\n");
    printf("\x1B[0;31m\n== !!請勿在中途退出遊戲!! ==\n\x1B[0m");
    
    bzero(&recvline, MAXLINE);
    n = read(sock_fd, recvline, MAXLINE);
    // printf("recv: %s\n", recvline);
    sscanf(recvline, "%d %d %d %d", &gameCode, &roundCode, &DayNight, &roleCode);
    int player1_i, player2_i;
    int wolf1_i, wolf2_i;
    while (gameCode == 666) {
        printf("\n\nRound %2d - %s\n", roundCode, (DayNight == 1) ? "Night" : "Morning");
        printf("serv: 天黑請閉眼\n");

    // guard time
        int safed;
        bzero(&recvline, MAXLINE);
        n = read(sock_fd, recvline, MAXLINE);
        // printf("recv: %s\n", recvline);
        sscanf(recvline, "%d %d %d %d %d %d", &gameCode, &roundCode, &DayNight, &roleCode, &player1_i, &player2_i);
        // 守衛宣告守護對象
        if (roleCode == 3 && DayNight == 1) printf("serv: 守衛請睜眼，你要守護的玩家是...\n");
        while (true) {
            if (myRole == 3 && my_alive) {
                printf("\x1B[0;90m現存玩家:\n\x1B[0m");
                for (int i = 0; i < 6; i++) {
                    if (!alive[i]) continue;
                    if (my_i == i) printf("\x1B[0;90m\tPlayer %d (%s) - me\n\x1B[0m", i + 1, playersName[i]);
                    else printf("\x1B[0;90m\tPlayer %d (%s)\n\x1B[0m", i + 1, playersName[i]);
                }
                printf("(請輸入玩家編號): ");
                bzero(&sendline, MAXLINE);
                fgets(sendline, MAXLINE, stdin);
                // 輸入不成立...
                safed = atoi(sendline) - 1;
                if (safed > 5 || safed < 0) { printf("玩家id: %d 不存在\n", safed + 1); continue; }
                if (!alive[safed]) { printf("玩家%d (%s) 已淘汰!\n", safed + 1, playersName[safed]); continue; }

                //送出守護選擇
                bzero(&sendline, MAXLINE);
                sprintf(sendline, "%d", safed);
                writen(sock_fd, sendline, strlen(sendline));
                printf("send: %s\n", sendline);
                fflush(STDIN_FILENO);
                break;
            }
            else { printf("serv: 守衛選擇中...\n"); fflush(STDIN_FILENO); break; }
        }
        // 守衛行動結束
        bzero(&recvline, MAXLINE);
        n = read(sock_fd, recvline, MAXLINE);
        if (n <= 0) { gameCode = 667; break; }
        int ack = atoi(recvline);
        if (myRole == 3 && my_alive) printf("serv ack: %d\n守衛宣告成功\n", ack + 1);
        else printf("守衛宣告成功\n");

        ////////////////////////////////////////////////////////////////////  

    // wolf time
        bzero(&recvline, MAXLINE);
        n = read(sock_fd, recvline, MAXLINE);
        // printf("recv: %s\n", recvline);
        sscanf(recvline, "%d %d %d %d %d %d", &gameCode, &roundCode, &DayNight, &roleCode, &player1_i, &player2_i);
        
        wolf1_i = player1_i; wolf2_i = player2_i;
        // 確認同伴
        if (roleCode == 1 && DayNight == 1) printf("serv: 狼人請睜眼\n你的同伴是: ");
        if (myRole == 0) printf("player %2d (%s)", player2_i + 1, playersName[player2_i]);
        else if (myRole == 1) printf("player %2d (%s)", player1_i + 1, playersName[player1_i]);
        printf("\n");

        // 殺人
        while (true) {
            printf("\x1B[0;32m<<若狼群選擇不一，依狼王選擇為主>>\n\x1B[0m");
            printf("你要殺的人是...\n");
            if (my_alive && (myRole == 0 || myRole == 1)) {
                printf("\x1B[0;90m現存玩家:\n\x1B[0m");
                for (int i = 0; i < 6; i++) {
                    if (!alive[i]) continue;
                    if (my_i == i) printf("\x1B[0;90m\tPlayer %d (%s) - me\n\x1B[0m", i + 1, playersName[i]);
                    else printf("\x1B[0;90m\tPlayer %d (%s)\n\x1B[0m", i + 1, playersName[i]);
                }
                printf("(請輸入玩家編號): ");
                bzero(&sendline, MAXLINE);
                fgets(sendline, MAXLINE, stdin);
                // 輸入不成立...
                int killed = atoi(sendline) - 1;
                if (killed > 5 || killed < 0) { printf("玩家id: %d 不存在\n", killed + 1); continue; }
                if (killed == player1_i || killed == player2_i) { printf("玩家id: %d 為自己或夥伴...\n", killed + 1); continue; }
                if (!alive[killed]) { printf("玩家%d (%s) 已淘汰!\n", killed + 1, playersName[killed]); continue; }

                //送出殺人選擇
                bzero(&sendline, MAXLINE);
                sprintf(sendline, "%d", killed);
                writen(sock_fd, sendline, strlen(sendline));
                printf("send: %s\n", sendline);
                printf("serv: 狼人請閉眼\n");
                fflush(STDIN_FILENO);
                break;
            }
            else { printf("serv: 狼人選擇中...\n"); fflush(STDIN_FILENO); break; }
        }
        // 狼人行動結束
        bzero(&recvline, MAXLINE);
        n = read(sock_fd, recvline, MAXLINE);
        if (n <= 0) { gameCode = 667; break; }
        int dead = atoi(recvline) - 1;
        // printf("狼人行動結束 recv: %s\n", recvline);
        printf("狼人行動結束\n");
        sprintf(sendline, "--player %d (%s) was killed\n", dead + 1, playersName[dead]);
        // 還不一定會死
            // printf("Broadcast: %s\n", sendline);
            // alive[dead] = false;

    // 預言家行動開始
        int asked = -1;
        bzero(&recvline, MAXLINE);
        n = read(sock_fd, recvline, MAXLINE);
        // printf("recv: %s\n", recvline);
        sscanf(recvline, "%d %d %d %d %d %d", &gameCode, &roundCode, &DayNight, &roleCode, &player1_i, &player2_i);
        // 預言家提問
        if (roleCode == 2 && DayNight == 1) printf("serv: 預言家請睜眼，你要驗的玩家是...\n");
        while (true) {
            if (myRole == 2 && my_alive) {
                printf("\x1B[0;90m現存玩家:\n\x1B[0m");
                for (int i = 0; i < 6; i++) {
                    if (!alive[i]) continue;
                    if (my_i == i) printf("\x1B[0;90m\tPlayer %d (%s) - me\n\x1B[0m", i + 1, playersName[i]);
                    else printf("\x1B[0;90m\tPlayer %d (%s)\n\x1B[0m", i + 1, playersName[i]);
                }
                printf("(請輸入玩家編號): ");
                bzero(&sendline, MAXLINE);
                fgets(sendline, MAXLINE, stdin);
                // 輸入不成立...
                asked = atoi(sendline) - 1;
                if (asked > 5 || asked < 0) { printf("玩家id: %d 不存在\n", asked + 1); continue; }
                if (asked == my_i) { printf("玩家id: %d 為自己...\n", asked + 1); continue; }
                if (!alive[asked]) { printf("玩家%d (%s) 已淘汰!\n", asked + 1, playersName[asked]); continue; }

                //送出提問選擇
                bzero(&sendline, MAXLINE);
                sprintf(sendline, "%d", asked);
                writen(sock_fd, sendline, strlen(sendline));
                printf("send: %s\n", sendline);
                fflush(STDIN_FILENO);
                break;
            }
            else { printf("serv: 預言家選擇中...\n"); fflush(STDIN_FILENO); break; }
        }
        // 預言家行動結束
        bzero(&recvline, MAXLINE);
        n = read(sock_fd, recvline, MAXLINE);
        if (n <= 0) { gameCode = 667; break; }
        bool ask_isGood = (atoi(recvline) == 1);
        if (myRole == 2 && my_alive) {
            printf("--player %d (%s) is %s\n", asked + 1, playersName[asked], (ask_isGood) ? "good" : "bad");
        }
        else {
            printf("--player X (XX) is XX\n");
        }


    
    // 接收server廣播: DAY come
        printf("\x1B[0;33m\n天亮請睜眼\n\n\x1B[0m");
        bzero(&recvline, MAXLINE);
        n = read(sock_fd, recvline, MAXLINE);
        if (n <= 0) { gameCode = 667; printf("break\n"); break; }
        // printf("recv: %s", recvline);
        dead = atoi(recvline) - 1;
        bzero(&recvline, MAXLINE);
        if (dead >= 0) {
            sprintf(recvline, "-- Player %d (%s) was killed last night\n", dead + 1, playersName[dead]);
            alive[dead] = false;
            if (dead == my_i) my_alive = false;
        }
        else sprintf(recvline, "-- 昨天是平安夜!\n");
        printf("Broadcast: %s\n", recvline);


        sleep(1);

        ///////Day////////
        while(1){
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(sock_fd, &read_fds);

            int ready = select(sock_fd + 1, &read_fds, NULL, NULL, NULL);

            if(ready > 0 && FD_ISSET(sock_fd, &read_fds)){
                bzero(recvline, MAXLINE);
                n = read(sock_fd, recvline, MAXLINE);
                if(n <= 0){
                    break;
                }
                printf("%s", recvline);

                if(strstr(recvline, "討論時間結束") != NULL){
                    break;
                }

                if(strstr(recvline, "It's your turn to speak") != NULL){
                    bzero(sendline, MAXLINE);
                    fgets(sendline, MAXLINE, stdin);
                    write(sock_fd, sendline, strlen(sendline));
                }
            }
        }
        fflush(stdin);

        //////////////voting///////////////
        while(my_alive){
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(sock_fd, &read_fds);

            int ready = select(sock_fd + 1, &read_fds, NULL, NULL, NULL);

            if(ready > 0 && FD_ISSET(sock_fd, &read_fds)){
                bzero(recvline, MAXLINE);
                int n = read(sock_fd, recvline, MAXLINE);
                if(n <= 0){
                    break;
                }

                printf("%s", recvline);

                if (strstr(recvline, "投票時間開始") != NULL) {
                    bzero(sendline, MAXLINE);
                    printf("輸入您的投票編號：");
                    fgets(sendline, MAXLINE, stdin);
                    write(sock_fd, sendline, strlen(sendline));
                }

                if (strstr(recvline, "投票時間結束") != NULL) {
                    break;
                }
            }
        }

        fflush(stdin);

        sleep(2);

        struct timeval timeout;
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(sock_fd, &read_fds);

        int ready = select(sock_fd + 1, &read_fds, NULL, NULL, &timeout);

        if(ready > 0 && FD_ISSET(sock_fd, &read_fds)){
            bzero(recvline, MAXLINE);
            n = read(sock_fd, recvline, MAXLINE);
            if(n <= 0){
                break;
            }
            int a = atoi(recvline);
            printf("player %d OUT\n", a + 1);
            a--;
            alive[a] = false;
            if (a == my_i) my_alive = false;
        }

        ///////End////////

        bzero(&recvline, MAXLINE);
        n = read(sock_fd, recvline, MAXLINE);
        if (n <= 0) { gameCode = 667; break; }
        // printf("recv: %s\n", recvline);
        sscanf(recvline, "%d %d %d %d", &gameCode, &roundCode, &DayNight, &roleCode);
        printf("////////////////////////////////\n");
    } 
    
    printf("game end!\n");
    printf("Result: %s\n", (roundCode == 1) ? "good win" : "bad win");
    // while (true) sleep(3);
}
