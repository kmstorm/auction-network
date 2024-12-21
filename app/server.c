#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>
#include "../model/message.h"
#include "../model/user.h"
#include "../model/auction_room.h"
#include "../model/item.h"
#include "../model/countdown_timer.h"

#define PORT 8080
#define MAX_CLIENTS 30

int main()
{
    int server_sock, client_socks[MAX_CLIENTS] = {0};
    struct sockaddr_in server_addr, client_addr;
    fd_set read_fds;
    socklen_t addr_len = sizeof(client_addr);

    printf("Log_SERVER: Loading users from file\n");
    load_users_from_file();
    printf("Log_SERVER: Loading rooms from file\n");
    load_rooms_from_file();
    printf("Log_SERVER: Loading items from file\n");
    load_items_from_file();

    printf("Log_SERVER: Creating server socket\n");
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == 0)
    {
        perror("Log_SERVER: Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    printf("Log_SERVER: Binding server socket\n");
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Log_SERVER: Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Log_SERVER: Listening on port %d\n", PORT);
    if (listen(server_sock, 3) < 0)
    {
        perror("Log_SERVER: Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("Log_SERVER: Server listening on port %d\n", PORT);

    while (1)
    {
        FD_ZERO(&read_fds);
        FD_SET(server_sock, &read_fds);
        int max_sd = server_sock;

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (client_socks[i] > 0)
                FD_SET(client_socks[i], &read_fds);
            if (client_socks[i] > max_sd)
                max_sd = client_socks[i];
        }

        printf("Log_SERVER: Waiting for activity\n");
        int activity = select(max_sd + 1, &read_fds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR))
        {
            printf("Log_SERVER: Select error\n");
        }

        // New connection
        if (FD_ISSET(server_sock, &read_fds))
        {
            int new_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
            if (new_sock < 0)
            {
                perror("Log_SERVER: Accept failed");
                exit(EXIT_FAILURE);
            }
            printf("Log_SERVER: New connection accepted\n");
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (client_socks[i] == 0)
                {
                    client_socks[i] = new_sock;
                    printf("Log_SERVER: Added client socket to list: %d\n", new_sock);
                    break;
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            int sock = client_socks[i];
            if (FD_ISSET(sock, &read_fds))
            {
                Message message;
                memset(&message, 0, sizeof(Message)); // Clear the message buffer
                int valread = recv(sock, &message, sizeof(Message), 0);
                if (valread > 0)
                {
                    char username[50], password[50];
                    printf("Log_SERVER: Received message of type %d\n", message.message_type);
                    switch (message.message_type)
                    {
                    case 0: // REGISTER_REQUEST
                        if (parse_credentials(&message, username, password) && register_user(username, password))
                        {
                            build_message(&message, 0, "Registration successful");
                        }
                        else
                        {
                            build_message(&message, 0, "Registration failed");
                        }
                        send(sock, &message, sizeof(Message), 0);
                        break;
                    case 1: // LOGIN_REQUEST
                    {
                        int user_id;
                        if (parse_credentials(&message, username, password) && login_user(username, password, &user_id))
                        {
                            char response_payload[BUFFER_SIZE];
                            snprintf(response_payload, sizeof(response_payload), "Login successful|%d", user_id);
                            build_message(&message, 1, response_payload);
                            printf("Log_SERVER: Login successful for user %s with ID %d\n", username, user_id);
                        }
                        else
                        {
                            build_message(&message, 1, "Login failed");
                            printf("Log_SERVER: Login failed for user %s\n", username);
                        }
                        send(sock, &message, sizeof(Message), 0);
                        break;
                    }
                    case 2: // LOGOUT_REQUEST
                        logout_user(username);
                        build_message(&message, 2, "Logout successful");
                        send(sock, &message, sizeof(Message), 0);
                        break;
                    case 3: // CREATE_ROOM_REQUEST
                    {
                        // Call auction room create function
                        char room_name[50], description[200], start_time[20];
                        int duration;
                        sscanf(message.payload, "%49[^|]|%199[^|]|%19[^|]|%d", room_name, description, start_time, &duration);
                        if (create_room(1, room_name, description, start_time, duration)) // 1 indicates admin
                        {
                            build_message(&message, 3, "Room created successfully");
                        }
                        else
                        {
                            build_message(&message, 3, "Room creation failed");
                        }
                        send(sock, &message, sizeof(Message), 0);
                        break;
                    }
                    case 4: // DELETE_ROOM_REQUEST
                    {
                        int room_id;
                        sscanf(message.payload, "%d", &room_id);
                        if (delete_room(1, room_id)) // 1 indicates admin
                        {
                            build_message(&message, 4, "Room deleted successfully");
                        }
                        else
                        {
                            build_message(&message, 4, "Room deletion failed");
                        }
                        send(sock, &message, sizeof(Message), 0);
                        break;
                    }
                    case 5: // CREATE_ITEM_REQUEST
                    {
                        // Parse the message for room_id, item details
                        int room_id;
                        char name[50], description[200];
                        float starting_price;
                        sscanf(message.payload, "%d|%49[^|]|%199[^|]|%f", &room_id, name, description, &starting_price);

                        if (create_item(1, room_id, name, description, starting_price)) // Admin id 1
                        {
                            build_message(&message, 5, "Item created successfully");
                        }
                        else
                        {
                            build_message(&message, 5, "Item creation failed");
                        }
                        send(sock, &message, sizeof(Message), 0);
                        break;
                    }
                    case 6: // DELETE_ITEM_REQUEST
                    {
                        int room_id, item_id;
                        sscanf(message.payload, "%d|%d", &room_id, &item_id);

                        if (delete_item(1, room_id, item_id))
                        {
                            build_message(&message, 6, "Item deleted successfully");
                        }
                        else
                        {
                            build_message(&message, 6, "Item deletion failed");
                        }
                        send(sock, &message, sizeof(Message), 0);
                        break;
                    }
                    case 7: // LIST_ITEMS_REQUEST
                    {
                        int room_id;
                        sscanf(message.payload, "%d", &room_id); // Get the room ID from the message

                        list_items(room_id);

                        build_message(&message, 7, "Items listed successfully.");
                        send(sock, &message, sizeof(Message), 0);
                        break;
                    }
                    case 8: // IS_ADMIN_REQUEST
                    {
                        printf("Log_SERVER: IS_ADMIN_REQUEST received for user %s\n", username);
                        if (is_admin(username))
                        {
                            build_message(&message, 8, "Admin");
                            printf("Log_SERVER: User %s is an admin\n", username);
                        }
                        else
                        {
                            build_message(&message, 8, "Not Admin");
                            printf("Log_SERVER: User %s is not an admin\n", username);
                        }
                        send(sock, &message, sizeof(Message), 0);
                        break;
                    }
                    case 9: // JOIN_ROOM_REQUEST
                    {
                        int user_id, room_id;
                        sscanf(message.payload, "%d|%d", &user_id, &room_id);
                        printf("Log_SERVER: JOIN_ROOM_REQUEST received for user_id=%d, room_id=%d\n", user_id, room_id);
                        if (join_room(user_id, room_id))
                        {
                            build_message(&message, 9, "Joined room successfully");
                            send(sock, &message, sizeof(Message), 0);

                            // Start the countdown timer after sending the response
                            AuctionRoom *room = get_room_by_id(room_id);
                            if (room != NULL)
                            {
                                if (has_room_started(room_id))
                                {
                                    countdown_room_duration(sock, room->start_time, room->duration);
                                }
                                else
                                {
                                    countdown_to_start_time(sock, room->start_time);
                                    countdown_room_duration(sock, room->start_time, room->duration);
                                }
                            }
                        }
                        else
                        {
                            build_message(&message, 9, "Failed to join room");
                            send(sock, &message, sizeof(Message), 0);
                        }
                        break;
                    }
                    case 10: // LEAVE_ROOM_REQUEST
                    {
                        int user_id, room_id;
                        sscanf(message.payload, "%d|%d", &user_id, &room_id);
                        printf("Log_SERVER: LEAVE_ROOM_REQUEST received for user_id=%d, room_id=%d\n", user_id, room_id);
                        if (leave_room(user_id, room_id))
                        {
                            build_message(&message, 10, "Left room successfully");
                        }
                        else
                        {
                            build_message(&message, 10, "Failed to leave room");
                        }
                        send(sock, &message, sizeof(Message), 0);
                        break;
                    }
                    case 11: // LIST_ROOMS_REQUEST
                    {
                        printf("Log_SERVER: LIST_ROOMS_REQUEST received\n");
                        char room_list[BUFFER_SIZE] = "";
                        list_rooms(room_list, sizeof(room_list));
                        build_message(&message, 11, room_list);
                        send(sock, &message, sizeof(Message), 0);
                        break;
                    }
                    default:
                        printf("Log_SERVER: Unknown message type %d\n", message.message_type);
                    }
                }
                else if (valread == 0)
                {
                    // Client disconnected
                    printf("Log_SERVER: Client disconnected\n");
                    close(sock);
                    client_socks[i] = 0;
                }
                else
                {
                    perror("Log_SERVER: recv failed");
                }
            }
        }
    }

    return 0;
}
