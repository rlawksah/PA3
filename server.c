
#include "http-server.h"
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>

#define MaxChats 100000
#define MaxUsername 15
#define MaxMessage 255

typedef struct 
{
    char username[MaxUsername + 1];
    char message[MaxMessage + 1];
} Reaction;

typedef struct 
{
    int id;
    char timestamp[20];
    char username[MaxUsername + 1];
    char message[MaxMessage + 1];
    Reaction reactions[100];
    int reaction_count;
} Chat;

Chat* chatsLog[MaxChats]; 
int chat_count = 0;

void add_chat(const char* username, const char* message) 
{
    if (chat_count >= MaxChats) 
    {
        printf("Chat limit reached\n");
        return;
    }

    if (strlen(username) > MaxUsername || strlen(message) > MaxMessage) 
    {
        printf("Invalid username or message length\n");
        return;
    }

    Chat *new_chat = (Chat*)malloc(sizeof(Chat));
    if (new_chat == NULL) 
    { 
        printf("Memory allocation failed\n");
        return;
    }

    new_chat->id = chat_count + 1;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(new_chat->timestamp, sizeof(new_chat->timestamp), "%Y-%m-%d %H:%M", t);

    strncpy(new_chat->username, username, MaxUsername);
    strncpy(new_chat->message, message, MaxMessage);
    new_chat->reaction_count = 0;
}
    
uint8_t add_reaction(const char* username, const char* message, const char* id_str) 
{
    int chat_id = atoi(id_str);
    if (chat_id <= 0 || chat_id > chat_count) 
    {
        return 1; 
    }

    if (strlen(username) > MaxUsername || strlen(message) > MaxMessage) 
    {
        return 1; 
    }

    Chat *chat = chatsLog[chat_id - 1];
    

    
    Reaction *reaction = &chat->reactions[chat->reaction_count];
    strncpy(reaction->username, username, MaxUsername);
    strncpy(reaction->message, message, MaxMessage);

    chat->reaction_count++;
    return 0;  
}

void reset() {
    for (int i = 0; i < chat_count; i++) 
    {
        if (chatsLog[i] != NULL) 
        {
            free(chatsLog[i]);
        }
    }
    chat_count = 0;
}

void respond_with_chats(int client_sock) 
{
    char response[BUFFER_SIZE];
    for (int i = 0; i < chat_count; i++) 
    {
        Chat *chat = chatsLog[i];
        snprintf(response, sizeof(response), "[#%d %s] %s: %s\n", chat->id, chat->timestamp, chat->username, chat->message);
        write(client_sock, response, strlen(response));
    }
}


void handle_post(char* path, int client_sock) 
{
    char *username_ptr = strstr(path, "user=");
    char *message_ptr = strstr(path, "&message=");

    if (!username_ptr || !message_ptr) 
    {
        send(client_sock, "400 Bad Request: Missing user or message\n", strlen("400 Bad Request: Missing user or message\n"), 0);
        return;
    }

    username_ptr += 5;
    message_ptr += 9;

    char username[MaxUsername + 1];
    char message[MaxMessage + 1];

    strncpy(username, username_ptr, MaxUsername);
    username[MaxUsername] = '\0';

    strncpy(message, message_ptr, MaxMessage);
    message[MaxMessage] = '\0';

    if (strlen(username) == 0 || strlen(message) == 0) 
    {
        send(client_sock, "400 Bad Request: Username or message cannot be empty\n", strlen("400 Bad Request: Username or message cannot be empty\n"), 0);
        return;
    }

    add_chat(username, message);
    respond_with_chats(client_sock);
}

void handle_reaction(char* path, int client_sock) 
{
    char *username_ptr = strstr(path, "user=");
    char *message_ptr = strstr(path, "&message=");
    char *id_ptr = strstr(path, "&id=");

    if (!username_ptr || !message_ptr || !id_ptr) 
    {
        send(client_sock, "400 Bad Request: Missing user, message, or id\n", strlen("400 Bad Request: Missing user, message, or id\n"), 0);
        return;
    }

    username_ptr += 5;  
    message_ptr += 9;   
    id_ptr += 4;        

    char username[MaxUsername + 1];
    char message[MaxMessage + 1];
    int chat_id = atoi(id_ptr);

    strncpy(username, username_ptr, MaxUsername);
    username[MaxUsername] = '\0';

    strncpy(message, message_ptr, MaxMessage);
    message[MaxMessage] = '\0';

    if (strlen(username) == 0 || strlen(message) == 0 || chat_id <= 0) 
    {
        send(client_sock, "400 Bad Request: Invalid username, message, or id\n", strlen("400 Bad Request: Invalid username, message, or id\n"), 0);
        return;
    }

    if (add_reaction(username, message, id_ptr) != 0) 
    {
        send(client_sock, "400 Bad Request: Unable to add reaction\n", strlen("400 Bad Request: Unable to add reaction\n"), 0);
        return;
    }

    respond_with_chats(client_sock);
}


void Parsing(char* request, int client_sock) 
{
    if (strncmp(request, "GET /chats", 10) == 0) 
    {
        respond_with_chats(client_sock);
    } else if (strncmp(request, "GET /post", 9) == 0) 
    {
        handle_post(request, client_sock);
    } else if (strncmp(request, "GET /react", 10) == 0) 
    {
        handle_reaction(request, client_sock);
    } else 
    {
        send(client_sock, "404 Not Found\n\n", 24, 0);
    }
}

int main(int argc, char *argv[]) 
{
    int port = 0;
    if (argc > 1) 
    {
        port = atoi(argv[1]);
    }
    start_server(Parsing, port);
    return 0;
}
