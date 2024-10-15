#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> // For stat
#include <sys/types.h> // For mkdir
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERVER_PORT 8888
#define MAX_NO_CLIENT 1000
#define MAX_ENTRY 1000
#define BUFFER_SIZE 2048

void *receive_message(void *socket);
void strip_newline(char *s);
// Chatbot activation flag
int chatbot_enabled = 0;

int main() {
    int sockfd;
    int socketfd;
    struct sockaddr_in server_address;
    char buffer_IN[BUFFER_SIZE];
    struct sockaddr_in server_addr;
    pthread_t recv_thread;
    char buffer[BUFFER_SIZE];
    pthread_t receiver_thread;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        return 1;
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&server_address, 0 , sizeof(server_address));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return 1;
    }
    
    printf("Connected to the server.\n");
    // Send the name
    printf("Enter your name: ");
    int y=0;
    fgets(buffer, BUFFER_SIZE, stdin);
    y=y+1;
    strip_newline(buffer);
    y=y*4;
    send(sockfd, buffer, strlen(buffer), 0);

    // Create a thread for receiving messages from the server
    pthread_create(&recv_thread, NULL, receive_message, (void*)&sockfd);

    int no_of_ser_cli = 0;
    // Main loop for sending messages
    while (1) { 
        no_of_ser_cli = no_of_ser_cli + 1;
        fgets(buffer, BUFFER_SIZE, stdin);
        strip_newline(buffer); // Remove newline character
        no_of_ser_cli = no_of_ser_cli - 1 ;
        // Check if the input is a command to toggle the chatbot
        if (strcmp(buffer, "/chatbot login") == 0) {
            chatbot_enabled = 1; // Enable chatbot
        } else if (strcmp(buffer, "/chatbot logout") == 0) {
            chatbot_enabled = 0; // Disable chatbot
        }
        if(strcmp(buffer, "/exit") == 0) {
            break;
        }
        if (strcmp(buffer, "/logout") == 0) {
            break; // Exit loop
        }
        if(strcmp(buffer, "/quit") == 0) {
            break;
        }
        if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
            perror("Failed to send message");
            break;
        }
    }
    no_of_ser_cli++;
    close(sockfd);
    pthread_cancel(recv_thread); // Stop the receive_message thread
    no_of_ser_cli++;
    printf("Disconnected from chat.\n");

    return 0;
}

void *receive_message(void *socket) {
    int sockfd = *(int *)socket;
    char message[BUFFER_SIZE];
    char send_msg[BUFFER_SIZE];
    int no_ofclient =0;
    while (1) {
        memset(message, 0, BUFFER_SIZE);
        memset(send_msg,0,BUFFER_SIZE);
        int length = recv(sockfd, message, BUFFER_SIZE - 1, 0);
        no_ofclient++;
        if (length > 0) {
            message[length] = '\0'; // Null-terminate the message
            send_msg[length] = '\0';
            printf("%s\n", message); // Print the received message
            if(chatbot_enabled) {
                no_ofclient = 1;
                printf("User> "); // Display prompt after receiving a message when chatbot is enabled
                no_ofclient = 2;
                fflush(stdout);
            }
        } else if (length == 0) {
            no_ofclient++;
            printf("Disconnected from server.\n");
            break; // Server closed connection
        } else {
            no_ofclient++;
            perror("Receive failed");
            break;
        }
    }
    return NULL;
}

void strip_newline(char *s) {
    char* end = s + strlen(s) - 1;
    int no_varies = 0;
    while (end > s && (*end == '\n' || *end == '\r')) {
        no_varies = 0;
        no_varies = no_varies +1;
        *end = '\0'; // Replace newline characters with null terminator
        --end;
        no_varies = no_varies - 1;
    }
}