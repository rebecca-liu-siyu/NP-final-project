# include "unp.h"
# include "serv.h"
# include <stdbool.h>

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
    userfile_setup();
    if (argc != 2) { fprintf(stderr, "Usage: %s <port>\n", argv[0]); exit(1); }

    int listen_fd = Socket(AF_INET, SOCK_STREAM, 0);
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
        if (atoi(exist_fd) == connfd) {
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

        char command[10], username[50], password[50];
        bzero(&command, 10);
        bzero(&username, 50);
        bzero(&password, 50);
        sscanf(buffer, "%s %s %s", command, username, password);
        ///////////////////////////////////////////////////////////////////
        // Check client's input format
        if (!strcmp(username, "") || !strcmp(password, "")) {
            printf("Write: Invalid command format: `%s %s %s`\n");
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
                Write(connfd, "User already login via other device\n", MAXLINE);
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
            return;
        }

        if (command[0] == '\0') continue;
        printf("Client command: %s\n", command);
        if (strcmp(command, "1") == 0) {
            printf("New Room\n");
        } else if (strcmp(command, "2") == 0) {
            printf("Join Room\n");
        } else if (strcmp(command, "3") == 0) {
            printf("Exit\n");
            break;
        } else {
            printf("Invalid command\n");
        }
    }

}