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

int main(int argc, char** argv) {
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
        printf("\nNow, You can enter a game room\n");
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
            if (newRoomResult == -1) {
                continue;
            }
            else if(newRoomResult == 0) { Game(sock_fd); }
            else { printf("Server error\n"); return 0;}
            // break;
        } else if (strcmp(input, "2\n") == 0) {
            printf("Join Room\n");
            bzero(&sendline, MAXLINE);
            sprintf(sendline, "2");
            Write(sock_fd, sendline, sizeof(sendline));
            int joinRoomResult = JoinRoom(sock_fd, username);
            if (joinRoomResult == -1) continue;
            else if (joinRoomResult == 0) { Game(sock_fd); }
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
        fflush(stdin);
        printf("\nNew Room Setup:\n");
        printf("1. Set Room Name:\n");
        fgets(RoomName, 50, stdin);
        printf("2. Set Room Capacity (6~10)\n");
        scanf("%d", &RoomCapacity); getchar();
        printf("3. Set Room to Public? (T/F)\n");
        scanf("%c", &isPublic); getchar();

        bzero(&sendline, MAXLINE);
        sprintf(sendline, "%s %d %c", RoomName, RoomCapacity, isPublic);
        Write(sock_fd, sendline, sizeof(sendline));
        
        bzero(&recvline, MAXLINE);
        Read(sock_fd, recvline, MAXLINE);
        printf("Server response: %s\n", recvline);
        if (atoi(recvline) > 0 && atoi(recvline) < 3000) {
            printf("Room created successful\n");
            printf("Room ID: %d\n", atoi(recvline));
            return 0;
        }
        else if (atoi(recvline) == -1) {
            printf("Invalid room capacity\n");
            continue;
        } 
        else if (atoi(recvline) == -2) {
            printf("Invalid room isPublic - please enter 'T' or 'F'\n");
            continue;
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
        printf("\nJoin Room:\n");
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
        else {
            printf("Join Room failed\n");
            break;
        }
    }
    return -1;
}

void Game(int sock_fd) {
    printf("Wait for the game to start\n");

    char recvline[MAXLINE];
    while (true) {
        bzero(&recvline, MAXLINE);
        int n = read(sock_fd, recvline, MAXLINE);
        if (atoi(recvline) == 0) continue;
        break;
    }

    printf("Game started\n");
    bzero(&recvline, MAXLINE);
    int n = read(sock_fd, recvline, MAXLINE);
    printf("Game message: %s\n", recvline);
    return;
}