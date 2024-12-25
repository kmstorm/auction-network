// client.c

#include <stdio.h>
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
    SEARCH_ITEMS_REQUEST = 13,
    BUY_NOW_REQUEST = 14,
};

typedef struct {
    int sock;
    int room_id;
    int user_id;
} ThreadArgs;

void send_register_request(int sock, const char *username, const char *password);
void send_login_request(int sock, const char *username, const char *password);
void send_logout_request(int sock, const char *username);
void send_create_room_request(int sock, const char *room_name, const char *description, const char *start_time, int duration);
void send_delete_room_request(int sock, int room_id);
void send_create_item_request(int sock, int room_id, const char *name, const char *description, float starting_price, float buy_now_price);
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
void send_search_items_request(int sock, const char *keyword, const char *start_time, const char *end_time);
void send_buy_now_request(int sock, int user_id, int room_id);

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
            printf("5. Search Items\n");

            if (is_admin)
            {
                printf("6. Create Room\n");
                printf("7. Delete Room\n");
                printf("8. Add item\n");
                printf("9. Delete item\n");
                printf("10. List items\n");
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
                        printf("To place a bid, type the amount.\n");
                        printf("Type 'max' to buy now.\n");
                        printf("Type 'exit' to leave the room.\n\n");

                        inside_room = 1;

                        pthread_t timer_thread, input_thread;

                        // Create a struct to hold the arguments
                        ThreadArgs args;
                        args.sock = sock;
                        args.room_id = room_id;
                        args.user_id = user_id;

                        // Thread 1: Receive timer updates from server
                        if (pthread_create(&timer_thread, NULL, handle_timer_response, &sock) != 0)
                        {
                            perror("Failed to create timer thread");
                            break;
                        }

                        // Thread 2: Handle user input
                        if (pthread_create(&input_thread, NULL, handle_user_input, &args) != 0)
                        {
                            perror("Failed to create input thread");
                            break;
                        }

                        // Wait for both threads to complete
                        pthread_join(timer_thread, NULL);
                        pthread_join(input_thread, NULL);
                    }
                }
                break;

            case 5: // Search items
            {
                char keyword[50] = "", start_time[20] = "", end_time[20] = "";
                int c;

                // Clear the input buffer
                while ((c = getchar()) != '\n' && c != EOF) {}

                printf("Enter keyword (or leave empty): ");
                fgets(keyword, sizeof(keyword), stdin);
                if (keyword[strlen(keyword) - 1] == '\n') keyword[strlen(keyword) - 1] = '\0';

                printf("Enter start time (YYYY-MM-DDTHH:MM) (or leave empty): ");
                fgets(start_time, sizeof(start_time), stdin);
                if (start_time[strlen(start_time) - 1] == '\n') start_time[strlen(start_time) - 1] = '\0';

                printf("Enter end time (YYYY-MM-DDTHH:MM) (or leave empty): ");
                fgets(end_time, sizeof(end_time), stdin);
                if (end_time[strlen(end_time) - 1] == '\n') end_time[strlen(end_time) - 1] = '\0';

                send_search_items_request(sock, keyword, start_time, end_time);
                if (recv(sock, &response, sizeof(Message), 0) > 0)
                {
                    printf("Server response: %s\n", response.payload);
                }
            }
            break;


            case 6: // Create room
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

            case 7: // Delete room
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

            case 8: // Add item
                if (is_admin)
                {
                    int room_id;
                    char name[50], description[200];
                    float starting_price, buy_now_price;
                    printf("Enter room ID: ");
                    scanf("%d", &room_id);
                    printf("Enter item name: ");
                    scanf("%s", name);
                    printf("Enter item description: ");
                    scanf("%s", description);
                    printf("Enter starting price: ");
                    scanf("%f", &starting_price);
                    printf("Enter buy now price: ");
                    scanf("%f", &buy_now_price);
                    send_create_item_request(sock, room_id, name, description, starting_price, buy_now_price);
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

            case 9: // Delete item
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

            case 10: // List items
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

void send_create_item_request(int sock, int room_id, const char *name, const char *description, float starting_price, float buy_now_price)
{
    Message message;
    message.message_type = CREATE_ITEM_REQUEST;
    printf("%.2f\n",buy_now_price);
    snprintf(message.payload, sizeof(message.payload), "%d|%s|%s|%.2f|%.2f", 
             room_id, name, description, starting_price, buy_now_price);
    send(sock, &message, sizeof(Message), 0);
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

void send_buy_now_request(int sock, int user_id, int room_id)
{
    Message message;
    message.message_type = BUY_NOW_REQUEST; // Sử dụng loại message mới cho "buy now"
    snprintf(message.payload, sizeof(message.payload), "%d|%d", user_id, room_id);
    send(sock, &message, sizeof(Message), 0);
}

void* handle_timer_response(void* arg)
{
    int sock = *(int*)arg;
    Message response;

    // Set the socket to non-blocking mode
    fcntl(sock, F_SETFL, O_NONBLOCK);

    while (1)
    {
        // Receive data from the server if available
        if (recv(sock, &response, sizeof(Message), 0) > 0)
        {
            printf("%s\n", response.payload);

            // If the auction room has ended, break the loop
            if (strcmp(response.payload, "Auction ended for this item\n") == 0)
            {
                printf("Auction has ended. Returning to menu.\n");
                break;
            }
        }
        usleep(100000); // Sleep for 100ms to avoid busy-waiting
    }
    return NULL;
}


void* handle_user_input(void* arg)
{
    ThreadArgs* args = (ThreadArgs*)arg;
    int sock = args->sock;
    int room_id = args->room_id;
    int user_id = args->user_id;
    char input[50];

    while (1)
    {
        // printf("Enter your bid, or type 'max' to directly buy, type 'exit' to leave: ");
        scanf("%s", input);

        if (strcmp(input, "exit") == 0)
        {
            send_leave_room_request(sock, user_id, room_id);
            break;
        }
        else if (strcmp(input, "max") == 0)
        {
            send_buy_now_request(sock, user_id, room_id);
        }
        else
        {
            float bid_amount = atof(input);
            if (bid_amount > 0) // Ensure bid amount is valid
            {
                printf("Sending bid request: User ID %d, Room ID %d, Bid Amount %.2f\n", user_id, room_id, bid_amount);
                send_bid_request(sock, user_id, room_id, bid_amount);
            }
            else
            {
                printf("Invalid bid amount. Please enter a valid number.\n");
            }
        }
    }
    return NULL;
}

void send_search_items_request(int sock, const char *keyword, const char *start_time, const char *end_time)
{
    Message message;
    char payload[BUFFER_SIZE];
    snprintf(payload, sizeof(payload), "%s|%s|%s", keyword, start_time, end_time);
    build_message(&message, SEARCH_ITEMS_REQUEST, payload);
    send(sock, &message, sizeof(Message), 0);
}
