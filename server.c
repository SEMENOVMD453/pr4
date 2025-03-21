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

    struct sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(clientaddr);
    int connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
    if (connfd < 0) {
        perror("accept");
        exit(1);
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip, INET_ADDRSTRLEN);

    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), "Игрок подключился: %s", client_ip);
    log_message(log_msg);

    char buffer[MAXLINE];
    int number = rand() % 100 + 1;
    int attempts = 0;

    while (1) {
        memset(buffer, 0, MAXLINE);
        ssize_t n = read(connfd, buffer, MAXLINE - 1);
        if (n <= 0) {
            snprintf(log_msg, sizeof(log_msg), "Клиент (%s) отключился.", client_ip);
            log_message(log_msg);
            break;
        }

        buffer[strcspn(buffer, "\n")] = '\0';
        snprintf(log_msg, sizeof(log_msg), "Клиент (%s) отправил: %s", client_ip, buffer);
        log_message(log_msg);

        if (strncmp(buffer, "/quit", 5) == 0) {
            snprintf(log_msg, sizeof(log_msg), "Клиент (%s) покинул игру.", client_ip);
            log_message(log_msg);
            break;
        } else if (strncmp(buffer, "/restart", 8) == 0) {
            number = rand() % 100 + 1;
            attempts = 0;
            write(connfd, "Новая игра началась!\n", 22);
            snprintf(log_msg, sizeof(log_msg), "Клиент (%s) перезапустил игру.", client_ip);
            log_message(log_msg);
            continue;
        }

        int guess = atoi(buffer);
        attempts++;

        if (guess < number) {
            write(connfd, "Выше\n", 12);
            snprintf(log_msg, sizeof(log_msg), "Клиент (%s) ввел %d. Ответ: Выше", client_ip, guess);
            log_message(log_msg);
        } else if (guess > number) {
            write(connfd, "Ниже\n", 12);
            snprintf(log_msg, sizeof(log_msg), "Клиент (%s) ввел %d. Ответ: Ниже", client_ip, guess);
            log_message(log_msg);
        } else {
            write(connfd, "Правильно!\n", 12);
            snprintf(log_msg, sizeof(log_msg), "Клиент (%s) угадал число!", client_ip);
            log_message(log_msg);
            break;
        }

        if (attempts >= MAX_ATTEMPTS) {
            char msg[64];
            snprintf(msg, sizeof(msg), "Вы проиграли! Число было %d\n", number);
            write(connfd, msg, strlen(msg));
            snprintf(log_msg, sizeof(log_msg), "Клиент (%s) проиграл игру.", client_ip);
            log_message(log_msg);
            break;
        }
    }

    close(connfd);
    close(listenfd);
    return 0;
}
