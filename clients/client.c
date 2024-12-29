#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "client.h"

int main(){
    int sock = connect_to_server();
    int role;

    while (1) {
        clear_screen();
        int choice = main_menu();
        if (choice == 1) {
            printf("Waiting for server to start the game...\n");

            // 等待伺服器分配角色
            recv(sock, &role, sizeof(int), 0);
            printf("Your role: %d\n", role);

            // 回合開始
            while (1) {
                daytime_phase(sock);
                nighttime_phase(sock, role);
            }
        } else if (choice == 2) {
            printf("Exiting game...\n");
            close(sock);
            break;
        } else {
            printf("Invalid choice. Try again.\n");
        }
    }

    return 0;
}