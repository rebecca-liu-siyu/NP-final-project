#ifndef CLIENT_H
#define CLIENT_H

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

// client_network.c
int connect_to_server();
void send_to_server(int sock, const char *message);
int receive_from_server(int sock, char *buffer, size_t size);

// client_ui.c
void clear_screen();
void show_menu();
int main_menu();

// client_logic.c
void daytime_phase(int sock);
void nighttime_phase(int sock, int role);

#endif // CLIENT_H