#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define MAXLINE 1024
#define MAX_ATTEMPTS 5

void log_message(const char *message) {
    time_t now = time(NULL);
    char *time_str = ctime(&now);
    time_str[strcspn(time_str, "\n")] = '\0';
    printf("[%s] %s\n", time_str, message);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <port>\n", argv[0]);
        exit(1);
    }

    srand(time(NULL));

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(atoi(argv[1]));

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(listenfd, 1) < 0) {
        perror("listen");
        exit(1);
    }

    log_message("Сервер запущен и слушает порт");

    int connfd = accept(listenfd, NULL, NULL);
    if (connfd < 0) {
        perror("accept");
        exit(1);
    }

    log_message("Игрок подключился. Начинаем игру!");

    char buffer[MAXLINE];
    int number = rand() % 100 + 1;
    int attempts = 0;

    while (1) {
        memset(buffer, 0, MAXLINE);
        ssize_t n = read(connfd, buffer, MAXLINE - 1);
        if (n <= 0) {
            log_message("Клиент отключился.");
            break;
        }

        buffer[strcspn(buffer, "\n")] = '\0';
        log_message(buffer);

        if (strncmp(buffer, "/quit", 5) == 0) {
            log_message("Клиент покинул игру.");
            break;
        } else if (strncmp(buffer, "/restart", 8) == 0) {
            number = rand() % 100 + 1;
            attempts = 0;
            write(connfd, "Новая игра началась!\n", 22);
            log_message("Игра перезапущена.");
            continue;
        }

        int guess = atoi(buffer);
        attempts++;

        if (guess < number) {
            write(connfd, "Выше\n", 12);
            log_message("Игрок ввел число. Ответ: Выше");
        } else if (guess > number) {
            write(connfd, "Ниже\n", 12);
            log_message("Игрок ввел число. Ответ: Ниже");
        } else {
            write(connfd, "Правильно!\n", 12);
            log_message("Игрок угадал число!");
            break;
        }

        if (attempts >= MAX_ATTEMPTS) {
            char msg[64];
            snprintf(msg, sizeof(msg), "Вы проиграли! Число было %d\n", number);
            write(connfd, msg, strlen(msg));
            log_message("Игрок проиграл игру.");
            break;
        }
    }

    close(connfd);
    close(listenfd);
    return 0;
}
