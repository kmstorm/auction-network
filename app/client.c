// client.c

#include<stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fcntl.h> 

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
    CREATE_ITEM_REQUEST = 5,
    DELETE_ITEM_REQUEST = 6,
    LIST_ITEMS_REQUEST = 7,
    IS_ADMIN_REQUEST = 8,
    JOIN_ROOM_REQUEST = 9,
    LEAVE_ROOM_REQUEST = 10,
    LIST_ROOMS_REQUEST = 11,
    BID_REQUEST = 12,
    BID_RESPONSE =13
};

void send_register_request(int sock, const char *username, const char *password);
void send_login_request(int sock, const char *username, const char *password);
void send_logout_request(int sock, const char *username);
void send_create_room_request(int sock, const char *room_name, const char *description, const char *start_time, int duration);
void send_delete_room_request(int sock, int room_id);
void send_create_item_request(int sock, int room_id, const char *name, const char *description, float starting_price);
void send_delete_item_request(int sock, int room_id, int item_id);
void send_list_items_request(int sock, int room_id);
void send_is_admin_request(int sock, const char *username);
int is_admin_response(int sock);
void send_join_room_request(int sock, int user_id, int room_id);
void send_leave_room_request(int sock, int user_id, int room_id);
void send_list_rooms_request(int sock);
void send_bid_request(int sock, int user_id, int room_id, float bid_amount);
void* handle_timer_response(void* arg);
void* handle_user_input(void* arg);

int main()
{
    int sock;
    struct sockaddr_in server_addr;
    char username[50], password[50];
    int choice;
    int logged_in = 0;
    int is_admin = 0;
    int room_id;
    int inside_room = 0;
    int user_id = 0; // Add user_id variable

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
                    if (strncmp(response.payload, "Login successful", 16) == 0)
                    {
                        logged_in = 1; // Mark user as logged in
                        sscanf(response.payload, "Login successful|%d", &user_id); // Parse user ID

                        send_is_admin_request(sock, username);

                        is_admin = is_admin_response(sock); // Update admin status based on server response
                    }
                    else
                    {
                        printf("Login failed\n");
                    }
                }
                break;

            default:
                printf("Invalid choice. Please try again.\n");
            }
        }
        else if (inside_room)
        {
            // Menu when inside a room
            printf("\nRoom Menu:\n");
            printf("1. Leave Room\n");
            printf("Enter your choice: ");
            scanf("%d", &choice);

            Message response;

            switch (choice)
            {
            case 1:
                send_leave_room_request(sock, user_id, room_id);
                if (recv(sock, &response, sizeof(Message), 0) > 0)
                {
                    printf("Server response: %s\n", response.payload);
                    if (strcmp(response.payload, "Left room successfully") == 0)
                    {
                        inside_room = 0; // Mark user as outside the room
                    }
                }
                break;

            default:
                printf("Invalid choice. Please try again.\n");
            }
        }
        else
        {
            // Main menu after login
            printf("\nMenu:\n");
            printf("1. Logout\n");
            printf("2. Exit\n");
            printf("3. List Room\n");
            printf("4. Join Room\n");

            if (is_admin)
            {
                printf("5. Create Room\n");
                printf("6. Delete Room\n");
                printf("7. Add item\n");
                printf("8. Delete item\n");
                printf("9. List items\n");
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
                    logged_in = 0;
                }
                break;

            case 2:
                close(sock);
                return 0;

            case 3:
                // List rooms
                send_list_rooms_request(sock);
                if (recv(sock, &response, sizeof(Message), 0) > 0)
                {
                    printf("Server response:\n%s", response.payload);
                }
                break;

            case 4: // Join room
                printf("Enter room ID to join: ");
                scanf("%d", &room_id);
                send_join_room_request(sock, user_id, room_id);

                if (recv(sock, &response, sizeof(Message), 0) > 0)
                {
                    printf("Server response: %s\n", response.payload);
                    if (strcmp(response.payload, "Joined room successfully") == 0)
                    {
                        inside_room = 1;

                        pthread_t timer_thread, input_thread;

                        // Thread 1: Nhận thông tin từ server
                        if (pthread_create(&timer_thread, NULL, handle_timer_response, &sock) != 0)
                        {
                            perror("Failed to create timer thread");
                            break;
                        }

                        // Thread 2: Xử lý input người dùng
                        if (pthread_create(&input_thread, NULL, handle_user_input, &sock) != 0)
                        {
                            perror("Failed to create input thread");
                            break;
                        }

                        // Chờ cả hai thread hoàn thành
                        pthread_join(timer_thread, NULL);
                        pthread_join(input_thread, NULL);
                    }
                }
                break;


            case 5: // Create room
                if (is_admin)
                {
                    char room_name[50], description[200], start_time[20];
                    int duration;
                    printf("Enter room name: ");
                    scanf("%s", room_name);
                    printf("Enter description: ");
                    scanf("%s", description);
                    printf("Enter start time: ");
                    scanf("%s", start_time);
                    printf("Enter duration (in minutes): ");
                    scanf("%d", &duration);
                    send_create_room_request(sock, room_name, description, start_time, duration);
                    if (recv(sock, &response, sizeof(Message), 0) > 0)
                    {
                        printf("Server response: %s\n", response.payload);
                    }
                }
                else
                {
                    printf("Admin privileges required.\n");
                }
                break;

            case 6: // Delete room
                if (is_admin)
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
                else
                {
                    printf("Admin privileges required.\n");
                }
                break;

            case 7: // Add item
                if (is_admin)
                {
                    int room_id;
                    char name[50], description[200];
                    float starting_price;
                    printf("Enter room ID: ");
                    scanf("%d", &room_id);
                    printf("Enter item name: ");
                    scanf("%s", name);
                    printf("Enter item description: ");
                    scanf("%s", description);
                    printf("Enter starting price: ");
                    scanf("%f", &starting_price);
                    send_create_item_request(sock, room_id, name, description, starting_price);
                    if (recv(sock, &response, sizeof(Message), 0) > 0)
                    {
                        printf("Server response: %s\n", response.payload);
                    }
                }
                else
                {
                    printf("Admin privileges required.\n");
                }
                break;

            case 8: // Delete item
                if (is_admin)
                {
                    int room_id, item_id;
                    printf("Enter room ID: ");
                    scanf("%d", &room_id);
                    printf("Enter item ID to delete: ");
                    scanf("%d", &item_id);
                    send_delete_item_request(sock, room_id, item_id);
                    if (recv(sock, &response, sizeof(Message), 0) > 0)
                    {
                        printf("Server response: %s\n", response.payload);
                    }
                }
                else
                {
                    printf("Admin privileges required.\n");
                }
                break;

            case 9: // List items
                if (is_admin)
                {
                    int room_id;
                    printf("Enter room ID: ");
                    scanf("%d", &room_id);
                    send_list_items_request(sock, room_id);
                    if (recv(sock, &response, sizeof(Message), 0) > 0)
                    {
                        printf("Server response: %s\n", response.payload);
                    }
                }
                else
                {
                    printf("Admin privileges required.\n");
                }
                break;

            default:
                printf("Invalid choice. Please try again.\n");
            }
        }
    }

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

void send_list_rooms_request(int sock)
{
    Message message;
    message.message_type = LIST_ROOMS_REQUEST;
    snprintf(message.payload, sizeof(message.payload), ""); // Correct the snprintf call
    send(sock, &message, sizeof(message), 0);
}

void send_join_room_request(int sock, int user_id, int room_id)
{
    Message message;
    message.message_type = JOIN_ROOM_REQUEST;
    snprintf(message.payload, sizeof(message.payload), "%d|%d", user_id, room_id);
    send(sock, &message, sizeof(message), 0);
}

void send_leave_room_request(int sock, int user_id, int room_id)
{
    Message message;
    message.message_type = LEAVE_ROOM_REQUEST;
    snprintf(message.payload, sizeof(message.payload), "%d|%d", user_id, room_id);
    send(sock, &message, sizeof(message), 0);
}

// Function to send create room request (only for admins)
void send_create_room_request(int sock, const char *room_name, const char *description, const char *start_time, int duration)
{
    Message message;
    message.message_type = CREATE_ROOM_REQUEST;
    snprintf(message.payload, sizeof(message.payload), "%s|%s|%s|%d", room_name, description, start_time, duration);
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

void send_create_item_request(int sock, int room_id, const char *name, const char *description, float starting_price)
{
    Message message;
    message.message_type = CREATE_ITEM_REQUEST;
    snprintf(message.payload, sizeof(message.payload), "%d|%s|%s|%f", room_id, name, description, starting_price);
    send(sock, &message, sizeof(message), 0);
}

void send_delete_item_request(int sock, int room_id, int item_id)
{
    Message message;
    message.message_type = DELETE_ITEM_REQUEST;
    snprintf(message.payload, sizeof(message.payload), "%d|%d", room_id, item_id);
    send(sock, &message, sizeof(message), 0);
}

void send_list_items_request(int sock, int room_id)
{
    Message message;
    message.message_type = LIST_ITEMS_REQUEST;
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
        return strcmp(response.payload, "Admin") == 0;
    }
    return 0;
}

void send_bid_request(int sock, int user_id, int room_id, float bid_amount)
{
    Message message;
    message.message_type = BID_REQUEST;
    snprintf(message.payload, sizeof(message.payload), "%d|%d|%.2f", user_id, room_id, bid_amount);
    send(sock, &message, sizeof(message), 0);
}

void* handle_timer_response(void* arg)
{
    int sock = *(int*)arg;
    Message response;

    // Đặt socket ở chế độ non-blocking
    fcntl(sock, F_SETFL, O_NONBLOCK);

    while (1)
    {
        // Nhận dữ liệu từ server nếu có
        if (recv(sock, &response, sizeof(Message), 0) > 0)
        {
            printf("%s\n", response.payload);

            // Nếu nhận thông báo kết thúc room
            if (strcmp(response.payload, "Auction room has ended!\n") == 0)
            {
                printf("Room has ended. Returning to menu.\n");
                break;
            }
        }
        usleep(100000); // Nghỉ 100ms để tránh chiếm giữ CPU
    }
    return NULL;
}


void* handle_user_input(void* arg)
{
    int sock = *(int*)arg;
    int room_id, user_id;
    char input[50];


    while (1)
    {
        printf("Enter your bid or type 'exit' to leave: ");
        scanf("%s", input);

        if (strcmp(input, "exit") == 0)
        {
            send_leave_room_request(sock, user_id, room_id);
            break;
        }
        else
        {
            float bid_amount = atof(input);
            send_bid_request(sock, user_id, room_id, bid_amount);
        }
    }
    return NULL;
}

