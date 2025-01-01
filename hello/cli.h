#ifndef	CHI_H
#define	CHI_H

int Lobby(int sock_fd, const char* username);
int NewRoom(int sock_fd, const char* username);
int JoinRoom(int sock_fd, const char* username);
void Game(int sock_fd, const char* username);
void Game6(int sock_fd, FILE* fp, const char* username);
# endif // CHI_H