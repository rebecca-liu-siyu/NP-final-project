#include <stdio.h>
#include "client.h"

void clear_screen() {
    printf("\033[H\033[J"); // ANSI Escape Code 清屏
}

void show_menu() {
    printf("=== Welcome to Online Werewolf ===\n");
    printf("1. Join Game\n");
    printf("2. Exit\n");
    printf("Choose an option: ");
}

int main_menu() {
    int choice;
    show_menu();
    scanf("%d", &choice);
    getchar(); // Consume the newline character
    return choice;
}
