// client.c

#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

#include <../model/message.h>
#include <../model/user.h>

// Enum to define message types
enum MessageType {
    REGISTER_REQUEST = 0,
    LOGIN_REQUEST = 1,
    LOGOUT_REQUEST = 2,
    CREATE_ROOM_REQUEST = 3,
    DELETE_ROOM_REQUEST = 4,
    IS_ADMIN_REQUEST = 5
};

void send_register_request(int sock, const char *username, const char *password);
void send_login_request(int sock, const char *username, const char *password);
void send_logout_request(int sock, const char *username);
void send_create_room_request(int sock, const char *room_name, const char *description, const char *start_time, const char *end_time);
void send_delete_room_request(int sock, int room_id);
void send_is_admin_request(int sock, const char *username);
int is_admin_response(int sock);

int main()
{
    int sock;
    struct sockaddr_in server_addr;
    char username[50], password[50];
    int choice;
    int logged_in = 0;
    int is_admin = 0;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection failed");
        return 1;
    }

    printf("Connected to the server\n");

    // Main loop for client interaction
    while (1)
    {
        // Display different menus based on the login status
        if (!logged_in)
        {
            printf("\nMenu:\n");
            printf("1. Register\n");
            printf("2. Login\n");
            printf("Enter your choice: ");
            scanf("%d", &choice);

            Message response;

            switch (choice)
            {
            case 1:
                printf("Enter username: ");
                scanf("%s", username);
                printf("Enter password: ");
                scanf("%s", password);
                send_register_request(sock, username, password);
                if (recv(sock, &response, sizeof(Message), 0) > 0)
                {
                    printf("Server response: %s\n", response.payload);
                }
                break;

            case 2:
                printf("Enter username: ");
                scanf("%s", username);
                printf("Enter password: ");
                scanf("%s", password);
                send_login_request(sock, username, password);
                if (recv(sock, &response, sizeof(Message), 0) > 0)
                {
                    printf("Server response: %s\n", response.payload);
                    if (strcmp(response.payload, "Login successful") == 0)
                    {
                        logged_in = 1; // Mark user as logged in

                        send_is_admin_request(sock, username);

                        is_admin = is_admin_response(sock); // Update admin status based on server response
                    }
                }
                break;

            default:
                printf("Invalid choice. Please try again.\n");
            }
        }
        else
        {
            // After login, show logged-in menu
            printf("\nMenu:\n");
            printf("1. Logout\n");
            printf("2. Exit\n");

            // Check if the logged-in user is an admin
            if (is_admin)
            {
                printf("3. Create Room\n");
                printf("4. Delete Room\n");
            }

            printf("Enter your choice: ");
            scanf("%d", &choice);

            Message response;

            switch (choice)
            {
            case 1:
                send_logout_request(sock, username);
                if (recv(sock, &response, sizeof(Message), 0) > 0)
                {
                    printf("Server response: %s\n", response.payload);
                    logged_in = 0; // Mark user as logged out
                }
                break;

            case 2:
                close(sock);
                return 0;

            case 3:
                if (is_admin)
                {
                    {
                        char room_name[50], description[200], start_time[20], end_time[20];
                        printf("Enter room name: ");
                        scanf("%s", room_name);
                        printf("Enter description: ");
                        scanf("%s", description);
                        printf("Enter start time: ");
                        scanf("%s", start_time);
                        printf("Enter end time: ");
                        scanf("%s", end_time);
                        send_create_room_request(sock, room_name, description, start_time, end_time);
                        if (recv(sock, &response, sizeof(Message), 0) > 0)
                        {
                            printf("Server response: %s\n", response.payload);
                        }
                    }
                }
                break;

            case 4:
                if (is_admin)
                {
                    {
                        int room_id;
                        printf("Enter room ID to delete: ");
                        scanf("%d", &room_id);
                        send_delete_room_request(sock, room_id);
                        if (recv(sock, &response, sizeof(Message), 0) > 0)
                        {
                            printf("Server response: %s\n", response.payload);
                        }
                    }
                }
                break;

            default:
                printf("Invalid choice. Please try again.\n");
            }
        }
    }

    close(sock);
    return 0;
}

void send_register_request(int sock, const char *username, const char *password)
{
    Message message;
    message.message_type = REGISTER_REQUEST;
    snprintf(message.payload, sizeof(message.payload), "%s|%s", username, password);
    send(sock, &message, sizeof(message), 0);
}

void send_login_request(int sock, const char *username, const char *password)
{
    Message message;
    message.message_type = LOGIN_REQUEST;
    snprintf(message.payload, sizeof(message.payload), "%s|%s", username, password);
    send(sock, &message, sizeof(message), 0);
}

void send_logout_request(int sock, const char *username)
{
    Message message;
    message.message_type = LOGOUT_REQUEST;
    snprintf(message.payload, sizeof(message.payload), "%s", username);
    send(sock, &message, sizeof(message), 0);
}

// Function to send create room request (only for admins)
void send_create_room_request(int sock, const char *room_name, const char *description, const char *start_time, const char *end_time)
{
    Message message;
    message.message_type = CREATE_ROOM_REQUEST;
    snprintf(message.payload, sizeof(message.payload), "%s|%s|%s|%s", room_name, description, start_time, end_time);
    send(sock, &message, sizeof(message), 0);
}

// Function to send delete room request (only for admins)
void send_delete_room_request(int sock, int room_id)
{
    Message message;
    message.message_type = DELETE_ROOM_REQUEST;
    snprintf(message.payload, sizeof(message.payload), "%d", room_id);
    send(sock, &message, sizeof(message), 0);
}

// Function to check for admin role
void send_is_admin_request(int sock, const char *username)
{
    Message message;
    message.message_type = IS_ADMIN_REQUEST;
    snprintf(message.payload, sizeof(message.payload), "%s", username);
    send(sock, &message, sizeof(message), 0);
}

// Function to process the admin status response from the server

int is_admin_response(int sock)

{

    Message response;

    if (recv(sock, &response, sizeof(Message), 0) > 0)

    {

        if (strcmp(response.payload, "Admin") == 0)

        {

            return 1; // User is an admin
        }
    }

    return 0; // User is not an admin
}
