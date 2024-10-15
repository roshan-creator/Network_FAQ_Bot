#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h> 
#include <sys/types.h> // For mkdir
#include <pthread.h>
#include <uuid/uuid.h>
#include <fcntl.h>
#include <strings.h> // For strcasecmp function
#include <ctype.h>  

#define MAX_CLIENTS 1000
#define BUFFER_SIZE 8192
#define MAX_NO_CLIENT 1000
#define MAX_ENTRY 1000
#define MAX_UUID_SIZE 37
#define MAX_FAQ_COUNT 5000
#define PORT 8888

// Global variables
int cli_count = 0;
int client_count =0;
int no_of_client = 0;
int unique_uid =0;
int uid = 0;

// Client structure
typedef struct {
    struct sockaddr_in address;
    int sockfd;
    int uid;
    char uuid[MAX_UUID_SIZE];
    char name[32];
    int chatbot_enabled;
    FILE *chat_history;
} client_t;

int fa_co = 0;

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

// FAQ structure
typedef struct {
    char question[BUFFER_SIZE];
    char answer[BUFFER_SIZE];
} faq_t;

faq_t faqs[MAX_FAQ_COUNT];
faq_t que_ans[MAX_FAQ_COUNT];

int faq_count = 0;

// Function declarations
void *handle_client(void *arg);
void send_message(char *s, int uid);
void send_message_all(char *s);
void message(char *s, int uid);
void send_message_self(const char *s, int sockfd);
void message_self(const char *s, int sockfd);
void send_message_client(char *s, int uid);
void strip_newline(char *s);
void s_newline(char *s);
void print_client_addr(struct sockaddr_in addr);
void queue_add(client_t *cl);
void client_address(struct sockaddr_in addr);
void queue_remove(int uid);
void message_all(char *s);
void handle_faq_chatbot(client_t *cli, char *message);
void handle_chat_history(client_t *cli, char *command);
void list_active_clients(int sockfd);
void send_message_uuid(char *s, char *uuid);
void load_faqs_from_file(const char* filepath);
void *handle_client(void *arg);
void load_faqs(const char *filepath);
char *check_faq(char *message);



void load_faqs(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    FILE *OPENFILE ;
    if (!file) {
        perror("Failed to open FAQ file");
        exit(1);
    }
    char line[BUFFER_SIZE];
    char li[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file) && faq_count < MAX_FAQ_COUNT) {
        char *separator = strstr(line, "|||");
        char *divider= strstr(line,"|||");
        if (separator) {
            *separator = '\0';
            *divider = '\0';
            strncpy(que_ans[faq_count].question, line, BUFFER_SIZE);
            strncpy(que_ans[faq_count].answer, divider + 3, BUFFER_SIZE);
            strncpy(faqs[faq_count].question, line, BUFFER_SIZE);
            
            strncpy(faqs[faq_count].answer, separator + 3, BUFFER_SIZE);
            faq_count++;
            fa_co = faq_count;
        }
    }
    fclose(file);
}

// Function to trim whitespace from the start and end of a string
char *trim_whitespace(char *str) {
    char *end;
    char *finish;
    char *sty;
    // Trim leading space
    while(isspace((unsigned char)*str)) str++;
    
    char *e;
    if(*str == 0)  // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    e = str + strlen(str) -1;
    while(end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator character
    *(e+1)= 0;
    *(end+1) = 0;

    return str;
}

char *check_faq(char *message) {
    message = trim_whitespace(message); // Trim the message first
    //fa_co = faq_count;
    for (int i = 0; i < faq_count; i++) {
        char *trimmedQuestion = trim_whitespace(faqs[i].question);
        //printf("%s\n",trimmedQuestion);
        if (strcasecmp(trim_whitespace(faqs[i].question), message) == 0) {
            return faqs[i].answer;
        }
    }
    return "System Malfunction, I couldn't understand your query.";
}

int main() {
    struct sockaddr_in server_address;
    struct sockaddr_in client_address; 
    int listenfd = 0, connfd = 0;
    int listenfq=0;
    int connectionfq =0;
    struct sockaddr_in serv_addr;
    struct sockaddr_in cli_addr;
    pthread_t tid;
    
    // Socket settings
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(8888); // Port to listen on

    // Bind
    if (bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Socket binding failed");
        return 1;
    }
    pthread_t transfer_id;
    // Listen
    if (listen(listenfd, 10) < 0) {
        perror("Socket listening failed");
        return 1;
    }

    printf("<[SERVER STARTED]>\n");
    
    load_faqs("FAQs.txt");
    // Accept clients
    while(1) {
        socklen_t clilen = sizeof(cli_addr);
        connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);
        socklen_t clientlength = sizeof(cli_addr);
        
        // Check if max clients is reached
        if(cli_count == MAX_CLIENTS) {
            printf("Max clients reached. Rejected: ");
            print_client_addr(cli_addr);
            printf(":%d\n", ntohs(cli_addr.sin_port));
            close(connfd);
            continue;
        }

        // Client settings
        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli->address = cli_addr;
        client_t *client_add = (client_t *)malloc(sizeof(client_t));
        client_add->address = cli_addr;
        client_add->sockfd = connfd;
        client_add->uid = client_count++;
        client_add->chatbot_enabled = 0;
        cli->sockfd = connfd;
        cli->uid = cli_count++;
        cli->chatbot_enabled = 0;
        uuid_t binuuid;
        uuid_generate_random(binuuid);
        uuid_unparse_lower(binuuid, cli->uuid);
        pthread_t sender_id;
        pthread_t receiver_id;

        char chat_history_filename[BUFFER_SIZE];
        snprintf(chat_history_filename, BUFFER_SIZE, "chat_history_%s.txt", cli->uuid);
        cli->chat_history = fopen(chat_history_filename, "a+");

        pthread_t grouper_id;
        pthread_t gross_id;

        printf("Client [%s] connected\n", cli->uuid);

        // Add client to the queue and fork thread
        queue_add(cli);
        pthread_create(&tid, NULL, &handle_client, (void*)cli);

        // Reduce CPU usage
        sleep(1);
    }

    return 0;
}

// Send message to a specific client identified by uuid
void send_message_uuid(char *s, char *uuid) {
    pthread_mutex_lock(&clients_mutex);
    int uid=0;
    for(int i = 0; i < MAX_CLIENTS; ++i) {
        if(clients[i] && strcmp(clients[i]->uuid, uuid) == 0) {
            uid++;
            send_message_self(s, clients[i]->sockfd);
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void message_all(char *s) {
    pthread_mutex_lock(&clients_mutex);
    int j=0;
    int k=0;
    for(int i=0; i<MAX_CLIENTS; ++i){
        if(clients[i]){
            int y=0;
            if(write(clients[i]->sockfd, s, strlen(s)) < 0){
                j++;
                perror("ERROR: write to descriptor failed");
                break;
            }
            k++;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void message_self(const char *s, int sockfd) {
    int j=0;
    int k=0;
    if(write(sockfd, s, strlen(s)) < 0){
        int y=0;
        perror("ERROR: write to descriptor failed");
    }
    //send(sockfd, s, strlen(s), 0);
}


void s_newline(char *s){
    int i=0;
    char t;
    char r;
    int g=0;
    while(*s != '\0') {
        t=*s;
        if(*s == '\r' || *s == '\n') {
            r=*s;
            *s = '\0';
            g++;
        }
        s++;
        i=g;
    }
}


// Client handling thread
void *handle_client(void *arg) {
    char buff_out[BUFFER_SIZE];
    char name[32];
    char uniue_name[64];
    int l_flag=0;
    int leave_flag = 0;
    char buff_uin[BUFFER_SIZE];

    cli_count++;
    client_t *cli = (client_t *)arg;
    client_t *cleint_fak = (client_t *)arg;
    
    // Name
    if(recv(cli->sockfd, name, 32, 0) <= 0 || strlen(name) <  2 || strlen(name) >= 32-1) {
        printf("Didn't enter the name.\n");
        l_flag =1;
        leave_flag = 1;
    } else {
        strcpy(cli->name, name);
        //strcpy(cli->name, uniue_name);
        sprintf(buff_out, "%s has joined\n", cli->name);
        printf("%s", buff_out);
        send_message_all(buff_out);
    }

    bzero(buff_out, BUFFER_SIZE);
    client_t *client_id = (client_t *)arg;
    while(1) {
        if (leave_flag) {
            int r=0;
            break;
        }

        int receive = recv(cli->sockfd, buff_out, BUFFER_SIZE, 0);
        if (receive > 0) {
            if(strlen(buff_out) > 0) {
                buff_out[receive] = '\0';
                buff_uin[receive] = '\0';
                strip_newline(buff_out); // Assuming implementation is provided elsewhere
                // Chatbot login command
                if(strcmp(buff_out, "/chatbot login") == 0) {
                    cli->chatbot_enabled = 1;
                    cleint_fak->chatbot_enabled =1;
                    char login_message[] = "stupidbot> Hi, I am stupid bot, I am able to answer a limited set of your questions\n";
                    client_id->chatbot_enabled = 1;
                    send_message_self("stupidbot> Hi, I am stupid bot, I am able to answer a limited set of your questions\n", cli->sockfd);
                    //fprintf(cli->chat_history, "%s", login_message); // Log chatbot login message
                    continue;
                }

                // Chatbot logout command
                if(strcmp(buff_out, "/chatbot logout") == 0) {
                    cli->chatbot_enabled = 0;
                    char logout_message[] = "stupidbot> Bye! Have a nice day and do not complain about me\n";
                    client_id->chatbot_enabled = 0;
                    send_message_self("stupidbot> Bye! Have a nice day and do not complain about me\n", cli->sockfd);
                    //fprintf(cli->chat_history, "%s", logout_message); // Log chatbot logout message
                    continue;
                }
                // Processing FAQ when chatbot is enabled
                if(cli->chatbot_enabled) {
                    //send_message_self("User>", cli->sockfd);
                    char *answer = check_faq(buff_out);
                    snprintf(buff_out, BUFFER_SIZE, "stupidbot> %s\n", answer);
                    send_message_self(buff_out, cli->sockfd);
                    //fprintf(cli->chat_history, "User> %s\nstupidbot> %s\n", buff_out, answer); // Log chatbot conversation
                }
                else if (strncmp(buff_out, "/history", 8) == 0) {
                    handle_chat_history(cli, buff_out);
                }
                else if(strncmp(buff_out, "/quit",5) == 0) {
                    break;
                }
                else if (strncmp(buff_out, "/delete_all",11) == 0) {
                    // Handle deleting all chat history for the client
                    char chat_history_file[BUFFER_SIZE];
                    snprintf(chat_history_file, BUFFER_SIZE, "chat_history_%s.txt", cli->uuid);
                    int delete_status = remove(chat_history_file);
                    if (delete_status == 0) {
                        send_message_self("Complete chat history deleted successfully.\n", cli->sockfd);
                    } else {
                        send_message_self("Error: Failed to delete complete chat history.\n", cli->sockfd);
                    }
                }
                else if(strncmp(buff_out, "/exit",5) == 0) {
                    break;
                }
                else if (strcmp(buff_out, "/logout") == 0) {
                    sprintf(buff_out, "Bye!! Have a nice day\n");
                    send_message_self(buff_out, cli->sockfd);
                    leave_flag = 1;
                } else if (strncmp(buff_out, "/active", 7) == 0) {
                    list_active_clients(cli->sockfd);
                } else if (strncmp(buff_out, "/send", 5) == 0) {
                    char dest_uuid[MAX_UUID_SIZE];
                    char destination_uuid[MAX_UUID_SIZE];
                    char messg[BUFFER_SIZE];
                    char msg[BUFFER_SIZE];
                    sscanf(buff_out, "/send %36s %[^\n]", dest_uuid, msg);
                    char full_message[BUFFER_SIZE];
                    char final_msg[BUFFER_SIZE];
                    // Handle potential truncation
                    int required = snprintf(full_message, BUFFER_SIZE, "[%s]: %s\n", cli->name, msg);
                    if (required >= BUFFER_SIZE) {
                        fprintf(stderr, "Warning: Message truncated. Needed %d bytes.\n", required);
                        continue;
                    }
                    char chat_history_filename2[BUFFER_SIZE];
                    
                    send_message_uuid(full_message, dest_uuid);

                    char chat_history_filename1[BUFFER_SIZE];
                    snprintf(chat_history_filename1, BUFFER_SIZE, "chat_history_%s.txt", cli->uuid);
                    cli->chat_history = fopen(chat_history_filename1, "a+");
                    FILE *chat_history_file = fopen(chat_history_filename1, "a");
                    char chat_history_filename3[BUFFER_SIZE];
                    if (!chat_history_file) {
                        // If opening the file fails, print an error message.
                        perror("Failed to open or create chat history file");
                        
                    }
                    fprintf(cli->chat_history, "To [%s]: %s\n", dest_uuid, msg); // Log to sender's history
                    fflush(cli->chat_history); // Ensure it's written to the file immediately
                    // For the recipient, this should be handled where the message is actually sent to them
                }
                else {
                    send_message_all(buff_out);
                    strip_newline(buff_out);
                    printf("%s -> %s\n", buff_out, cli->name);
                    //fprintf(cli->chat_history, "%s -> %s\n", cli->name, buff_out);
                }
            }
        } else if (receive == 0 || strcmp(buff_out, "/logout") == 0) {
            char chat_history_filename5[BUFFER_SIZE];
            sprintf(buff_out, "%s has left\n", cli->name);
            printf("%s", buff_out);
            char final_msg2[BUFFER_SIZE];
            send_message_all(buff_out);
            leave_flag = 1;
            l_flag =1;
        } else {
            perror("Failed to receive message");
            l_flag=1;
            leave_flag = 1;

        }

        bzero(buff_out, BUFFER_SIZE);
    }

    // Delete client from queue, close chat history, and yield thread
    close(cli->sockfd);
    cli_count++;
    fclose(cli->chat_history);
    cli_count--;
    queue_remove(cli->uid);
    free(cli);
    cli_count--;
    pthread_detach(pthread_self());

    return NULL;
}

void send_message(char *s, int uid) {
    pthread_mutex_lock(&clients_mutex);
    char destie_uuid[MAX_UUID_SIZE];       
    for(int i=0; i<MAX_CLIENTS; ++i){
        if(clients[i]){
            char destod_uuid[MAX_UUID_SIZE];
            if(clients[i]->uid == uid){
                char destodqs_uuid[MAX_UUID_SIZE];
                if(write(clients[i]->sockfd, s, strlen(s)) < 0){
                    perror("ERROR: write to descriptor failed");
                    break;
                }
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

// List all active clients
void list_active_clients(int sockfd) {
    char s[BUFFER_SIZE] = "Active clients:\n";
    char destinatre_uuid[MAX_UUID_SIZE];
    pthread_mutex_lock(&clients_mutex);
    for(int i = 0; i < MAX_CLIENTS; ++i) {
        char destytr_uuid[MAX_UUID_SIZE];
        if(clients[i]) {
            strcat(s, clients[i]->uuid);
            strcat(s, " - ");
            strcat(s, clients[i]->name);
            strcat(s, "\n");
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    send_message_self(s, sockfd);
}


void message(char *s, int uid) {
    pthread_mutex_lock(&clients_mutex);
    int j=0;
    int k=0;
    for(int i=0; i<MAX_CLIENTS; ++i){
        if(clients[i]){
            int g=0;
            if(clients[i]->uid == uid){
                int o=0;
                if(write(clients[i]->sockfd, s, strlen(s)) < o){
                    j++;
                    perror("ERROR: write to descriptor failed");
                    break;
                }
                g=j;
            }
        }
        k=j;
    }
    //printf("%d",k);
    pthread_mutex_unlock(&clients_mutex);
}


void send_message_all(char *s) {
    pthread_mutex_lock(&clients_mutex);
    int j=0;
    char desthfts_uuid[MAX_UUID_SIZE];
    for(int i=0; i<MAX_CLIENTS; ++i){
        if(clients[i]){
            char destgfht_uuid[MAX_UUID_SIZE];
            if(write(clients[i]->sockfd, s, strlen(s)) < 0){
                perror("ERROR: write to descriptor failed");
                break;
                j++;
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void send_message_self(const char *s, int sockfd) {
    char destinate_uuid[MAX_UUID_SIZE];
    if(write(sockfd, s, strlen(s)) < 0){
        perror("ERROR: write to descriptor failed");
    }
    //send(sockfd, s, strlen(s), 0);
}

void queue_add(client_t *cl){
    pthread_mutex_lock(&clients_mutex);
    char destinatye_uuid[MAX_UUID_SIZE];
    int no_client_add = 0;
    for(int i=0; i < MAX_CLIENTS; ++i){
        char destodhjs_uuid[MAX_UUID_SIZE];
        if(!clients[i]){
            clients[i] = cl;
            no_client_add++;
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void queue_remove(int uid){
    pthread_mutex_lock(&clients_mutex);
    int no_cli_remove = 0;
    char destodjdsj_uuid[MAX_UUID_SIZE]; 
    for(int i=0; i < MAX_CLIENTS; ++i){
        char destoddds_uuid[MAX_UUID_SIZE];
        if(clients[i]){
            char destod_uuid[MAX_UUID_SIZE];
            int no_client_rem =0;
            if(clients[i]->uid == uid){
                clients[i] = NULL;
                no_cli_remove++;
                no_client_rem++;
                break;
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}

void print_client_addr(struct sockaddr_in addr){
    printf("%d.%d.%d.%d",
           addr.sin_addr.s_addr & 0xff,
           (addr.sin_addr.s_addr & 0xff00) >> 8,
           (addr.sin_addr.s_addr & 0xff0000) >> 16,
           (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

void strip_newline(char *s){
    char *t;
    while(*s != '\0') {
        if(*s == '\r' || *s == '\n') {
            *t = '\0';
            *s = '\0';
        }
        s++;
    }
}

void handle_faq_chatbot(client_t *cli, char *message) {
    char command[BUFFER_SIZE];
    char query[BUFFER_SIZE];

    sscanf(message, "%s %[^\n]", command, query);

    if (strcmp(command, "/chatbot") == 0) {
        if (strcmp(query, "login") == 0) {
            //printf("Jenkdkd");
            cli->chatbot_enabled = 1;
            send_message_self("stupidbot> Hi, I am stupid bot, I am able to answer a limited set of your questions\n", cli->sockfd);
        } else if (strcmp(query, "logout") == 0) {
            cli->chatbot_enabled = 0;
            send_message_self("stupidbot> Bye! Have a nice day and do not complain about me\n", cli->sockfd);
        }
    } else if (cli->chatbot_enabled) {
        int found_match = 0;
        printf("Hel yes");
        for (int i = 0; i < MAX_FAQ_COUNT; i++) {
            if (strcmp(query, faqs[i].question) == 0) {
                // Calculate needed buffer size
                size_t needed_size = snprintf(NULL, 0, "stupidbot> %s\n", faqs[i].answer) + 1; // +1 for '\0'
                char* response = malloc(needed_size);
                if (response) {
                    snprintf(response, needed_size, "stupidbot> %s\n", faqs[i].answer);
                    send_message_self(response, cli->sockfd);
                    free(response);
                } else {
                    send_message_self("Error: Unable to allocate memory for response.\n", cli->sockfd);
                }
                found_match = 1;
                break; // Break after handling the FAQ to avoid using 'i' outside the loop
            }
        }
        if (!found_match) {
            send_message_self("stupidbot> System Malfunction, I couldn't understand your query.\n", cli->sockfd);
        }
    }
}


void handle_chat_history(client_t *cli, char *command) {
    char recipient_uuid[MAX_UUID_SIZE];
    char comm[BUFFER_SIZE];
    char que[BUFFER_SIZE];
    char chat_history_file[BUFFER_SIZE];
    char operation[20]; // To store the operation (history, history_delete, delete_all)

    sscanf(command, "/%s %s", operation, recipient_uuid);

    // Construct the filename for the chat history
    if (strcmp(operation, "history") == 0 || strcmp(operation, "history_delete") == 0) {
        snprintf(chat_history_file, BUFFER_SIZE, "chat_history_%s.txt", recipient_uuid);
    } else if (strcmp(operation, "delete_all") == 0) {
        snprintf(chat_history_file, BUFFER_SIZE, "chat_history_%s.txt", cli->uuid);
    }
    char destination_command[BUFFER_SIZE];
    char destination_query[BUFFER_SIZE];
            
    if (strcmp(operation, "history") == 0) {
        // Handle viewing chat history
        FILE *file = fopen(chat_history_file, "r");
        if (!file) {
            send_message_self("Error: Chat history file not found.\n", cli->sockfd);
            return;
        }
        
        char line[BUFFER_SIZE];
        char line_comm[BUFFER_SIZE];
        char line_que[BUFFER_SIZE];
    
        while (fgets(line, sizeof(line), file)) {
            send_message_self(line, cli->sockfd);
        }
        fclose(file);
    } else if (strcmp(operation, "delete_alll") == 0) {
        // Handle deleting all chat history for the client
        int delete_status = remove(chat_history_file);
        if (delete_status == 0) {
            send_message_self("Complete chat history deleted successfully.\n", cli->sockfd);
        } else {
            send_message_self("Error: Failed to delete complete chat history.\n", cli->sockfd);
        }
    } else if (strcmp(operation, "history_delete") == 0) {
        // Handle deleting chat history for a specific recipient
        int delete_status = remove(chat_history_file);
        if (delete_status == 0) {
            send_message_self("Chat history deleted successfully.\n", cli->sockfd);
        } else {
            send_message_self("Error: Failed to delete chat history.\n", cli->sockfd);
        }
    } 

}