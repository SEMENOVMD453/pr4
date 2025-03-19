#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define MAXLINE 1024
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_RESET "\033[0m"

void auto_mode(int sockfd) {
    int low = 1, high = 100;
    char recvline[MAXLINE];
    char sendline[MAXLINE];
    int attempts_left = 5;

    while (attempts_left > 0) {
        int guess = (low + high) / 2;
        snprintf(sendline, sizeof(sendline), "%d\n", guess);
        write(sockfd, sendline, strlen(sendline));
        printf("Робот: %d (%d попыток осталось)\n", guess, attempts_left);

        memset(recvline, 0, MAXLINE);
        if (read(sockfd, recvline, MAXLINE - 1) <= 0) {
            printf(COLOR_RED "Сервер выключен. Выход..." COLOR_RESET "\n");
            break;
        }
        printf(COLOR_YELLOW "Сервер: %s" COLOR_RESET, recvline);

        if (strstr(recvline, "Выше")) {
            low = guess + 1;
            attempts_left--;
        } else if (strstr(recvline, "Ниже")) {
            high = guess - 1;
            attempts_left--;
        } else if (strstr(recvline, "Правильно!")) {
            printf(COLOR_GREEN "Сервер: %s" COLOR_RESET, recvline);
            break;
        } else if (strstr(recvline, "Игра окончена")) {
            break;
        }
        sleep(1);
    }
}

void interactive_mode(int sockfd) {
    char recvline[MAXLINE];
    char sendline[MAXLINE];
    int attempts_left = 5;
    int game_over = 0;

    while (1) {
        if (!game_over) {
            printf("Введите ваш ответ (%d попыток осталось) или /restart, /quit: ", attempts_left);
        }

        fgets(sendline, MAXLINE, stdin);
        if (strcmp(sendline, "\n") == 0) continue;
        write(sockfd, sendline, strlen(sendline));

        if (strncmp(sendline, "/quit", 5) == 0) {
            printf("Выход...\n");
            break;
        }

        memset(recvline, 0, MAXLINE);
        if (read(sockfd, recvline, MAXLINE - 1) <= 0) {
            printf(COLOR_RED "Сервер выключен. Выход..." COLOR_RESET "\n");
            break;
        }

        if (strstr(recvline, "Выше") || strstr(recvline, "Ниже")) {
            attempts_left--;
            printf(COLOR_YELLOW "Сервер: %s" COLOR_RESET, recvline);
        } else if (strstr(recvline, "Правильно!")) {
            printf(COLOR_GREEN "Сервер: %s" COLOR_RESET, recvline);
            game_over = 1;
        } else if (strstr(recvline, "Игра окончена")) {
            game_over = 1;
        } else if (strstr(recvline, "Новая игра началась")) {
            attempts_left = 5;
            game_over = 0;
            printf(COLOR_GREEN "Сервер: %s" COLOR_RESET, recvline);
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Использование: %s <айпи сервера> <порт>\n", argv[0]);
        exit(1);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr = {0};
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        exit(1);
    }

    printf("Выбери режим:\n1 - Авто (робот)\n2 - Интерактивный (вручную)\n> ");
    int mode;
    scanf("%d", &mode);
    getchar();

    if (mode == 1) {
        auto_mode(sockfd);
    } else {
        interactive_mode(sockfd);
    }

    close(sockfd);
    return 0;
}




