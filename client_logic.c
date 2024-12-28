#include <stdio.h>
#include <string.h>
#include "client.h"

void daytime_phase(int sock) {
    char buffer[BUFFER_SIZE];
    printf("Daytime Phase: Chat or Vote\n");

    while (1) {
        printf("You: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        send_to_server(sock, buffer);

        if (strncmp(buffer, "vote", 4) == 0) {
            printf("Voting submitted.\n");
            break;
        }
    }
}

void nighttime_phase(int sock, int role) {
    char buffer[BUFFER_SIZE];
    printf("Nighttime Phase: Special actions for role %d\n", role);

    if (role == 1) { // 假設 1 是狼人
        printf("Choose a target to attack: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        send_to_server(sock, buffer);
    } else if (role == 2) { // 假設 2 是預言家
        printf("Choose a player to investigate: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        send_to_server(sock, buffer);
    }
}
