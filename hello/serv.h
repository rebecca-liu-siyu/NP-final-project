#ifndef SERV_H
#define SERV_H

// Extra definitions
# define MAIN_START printf("\n= MAIN START =\n")
# define MAIN_END printf("\n= MAIN END =\n")
# define MAXCLIENT  100
# define USER_FILE  "users.txt"
# define TP_FILE    "tp.txt"

// Functions
void  userfile_setup(); // initial status: off
void  cli_disconnect(int connfd);
void* handle_client(void* connfd_ptr);
int register_user(int connfd, const char* username, const char* passwd);
int login_user(int connect_fd, const char* username, const char* password);
void Lobby(int connfd, const char* username);
#endif // SERV_H