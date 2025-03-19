#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#define MAXLINE 1024
#define MAX_ATTEMPTS 5

#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_RESET "\033[0m"

FILE *logfile;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
int client_counter = 0;

void log_message(const char *client_info, const char *message, const char *color) {
    pthread_mutex_lock(&log_mutex);
    time_t now = time(NULL);
    char *time_str = ctime(&now);
    time_str[strcspn(time_str, "\n")] = '\0';
    printf("%s[%s] %s: %s%s\n", color, time_str, client_info, message, COLOR_RESET);
    fprintf(logfile, "[%s] %s: %s\n", time_str, client_info, message);
    fflush(logfile);
    pthread_mutex_unlock(&log_mutex);
}

void *handle_client(void *arg) {
    int connfd = *((int *)arg);
    free(arg);

    char client_ip[INET_ADDRSTRLEN];
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    getpeername(connfd, (struct sockaddr *)&addr, &len);
    inet_ntop(AF_INET, &addr.sin_addr, client_ip, sizeof(client_ip));

    int client_id = __sync_add_and_fetch(&client_counter, 1);
    char client_label[64];
    snprintf(client_label, sizeof(client_label), "%s#%d", client_ip, client_id);

    log_message(client_label, "Игрок присоединлся к серверу", COLOR_GREEN);

    char buffer[MAXLINE];
    int number = rand() % 100 + 1;
    int attempts = 0;
    int game_over = 0;

    struct timeval timeout = {60, 0};
    setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    while (1) {
        memset(buffer, 0, MAXLINE);
        ssize_t n = read(connfd, buffer, MAXLINE - 1);
        if (n <= 0) {
            log_message(client_label, "Отключен или превышено время ожидания", COLOR_RED);
            break;
        }

        buffer[strcspn(buffer, "\n")] = '\0';
        if (strlen(buffer) == 0) {
            write(connfd, "Неверный ввод\n", strlen("Неверный ввод\n"));
            continue;
        }

        log_message(client_label, buffer, COLOR_YELLOW);

        if (game_over && (strcmp(buffer, "/restart") != 0 && strcmp(buffer, "/quit") != 0)) {
            write(connfd, "Игра окончена. Пиши /restart or /quit\n", strlen("Игра окончена. Пиши /restart or /quit\n"));
            continue;
        }

        if (strncmp(buffer, "/quit", 5) == 0) {
            log_message(client_label, "Клиент досрочно покинул игру", COLOR_RED);
            break;
        } else if (strncmp(buffer, "/restart", 8) == 0) {
            number = rand() % 100 + 1;
            attempts = 0;
            game_over = 0;
            write(connfd, "Новая игра началась!\n", strlen("Новая игра началась!\n"));
            continue;
        }

        char *endptr;
        int guess = strtol(buffer, &endptr, 10);
        if (*endptr != '\0' || guess < 1 || guess > 100) {
            write(connfd, "Неверный ввод. Число может быть от 1 до 100.\n", strlen("Неверный ввод. Число может быть от 1 до 100.\n"));
            continue;
        }

        if (!game_over) attempts++;

        if (guess < number) {
            write(connfd, "Выше\n", strlen("Выше\n"));
        } else if (guess > number) {
            write(connfd, "Ниже\n", strlen("Ниже\n"));
        } else {
            write(connfd, "Правильно!\n", strlen("Правильно!\n"));
            write(connfd, "Практическая работа 4.\n", strlen("Практическая работа 4.\n"));
            log_message(client_label, "Клиент победил", COLOR_GREEN);
            game_over = 1;
            continue;
        }

        if (attempts >= MAX_ATTEMPTS && !game_over) {
            char msg[64];
            snprintf(msg, sizeof(msg), "Вы проиграли! Число было %d\n", number);
            write(connfd, msg, strlen(msg));
            write(connfd, "Практическая работа 4.\n", strlen("Практическая работа 4.\n"));
            log_message(client_label, "Клиент проиграл", COLOR_RED);
            game_over = 1;
        }
    }

    log_message(client_label, "Клиент отключился", COLOR_RED);
    close(connfd);
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <port>\n", argv[0]);
        exit(1);
    }

    logfile = fopen("server.log", "a");
    if (!logfile) {
        perror("logfile");
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
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(atoi(argv[1]));

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind error");
        exit(1);
    }

    if (listen(listenfd, 10) < 0) {
        perror("listen error");
        exit(1);
    }

    printf("Сервер слушает на порте %s...\n", argv[1]);

    while (1) {
        int *connfd = malloc(sizeof(int));
        *connfd = accept(listenfd, NULL, NULL);
        if (*connfd < 0) {
            perror("accept");
            free(connfd);
            continue;
        }
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, connfd);
        pthread_detach(tid);
    }

    fclose(logfile);
}
