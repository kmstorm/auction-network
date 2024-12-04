#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include "../model/message.h"
#include "../model/user.h"
#include "../model/auction_room.h"

#define PORT 8080
#define MAX_CLIENTS 30

int main()
{
    int server_sock, client_socks[MAX_CLIENTS] = {0};
    struct sockaddr_in server_addr, client_addr;
    fd_set read_fds;
    socklen_t addr_len = sizeof(client_addr);

    load_users_from_file();
    load_rooms_from_file();

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_sock, 3);
    printf("Server listening on port %d\n", PORT);

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

        select(max_sd + 1, &read_fds, NULL, NULL, NULL);

        // New connection
        if (FD_ISSET(server_sock, &read_fds))
        {
            int new_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
            printf("New connection accepted\n");
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (client_socks[i] == 0)
                {
                    client_socks[i] = new_sock;
                    printf("Added client socket to list: %d\n", new_sock);
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
                if (recv(sock, &message, sizeof(Message), 0) > 0)
                {
                    char username[50], password[50];
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
                        break;
                    case 1: // LOGIN_REQUEST
                        if (parse_credentials(&message, username, password) && login_user(username, password))
                        {
                            build_message(&message, 1, "Login successful");
                        }
                        else
                        {
                            build_message(&message, 1, "Login failed");
                        }
                        break;
                    case 2: // LOGOUT_REQUEST
                        logout_user(username);
                        build_message(&message, 2, "Logout successful");
                        break;
                    case 3:                     // CREATE_ROOM_REQUEST
                        if (is_admin(username)) // Only allow if user is an admin
                        {
                            // Call auction room create function
                            char room_name[50], description[200], start_time[20], end_time[20];
                            sscanf(message.payload, "%s|%s|%s|%s", room_name, description, start_time, end_time);
                            if (create_room(1, room_name, description, start_time, end_time)) // 1 indicates admin
                            {
                                build_message(&message, 3, "Room created successfully");
                            }
                            else
                            {
                                build_message(&message, 3, "Room creation failed");
                            }
                        }
                        else
                        {
                            build_message(&message, 3, "Admin privileges required");
                        }
                        break;
                    case 4:                     // DELETE_ROOM_REQUEST
                        if (is_admin(username)) // Only allow if user is an admin
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
                        }
                        else
                        {
                            build_message(&message, 4, "Admin privileges required");
                        }
                        break;
                    case 5: // CREATE_ITEM_REQUEST
                        if (is_admin(username))
                        {
                            // Parse the message for room_id, item details
                            int room_id;
                            char name[50], description[200];
                            float starting_price;
                            sscanf(message.payload, "%d|%s|%s|%f", &room_id, name, description, &starting_price);

                            if (create_item_in_room(1, room_id, name, description, starting_price)) // Admin id 1
                            {
                                build_message(&message, 5, "Item created successfully");
                            }
                            else
                            {
                                build_message(&message, 5, "Item creation failed");
                            }
                        }
                        else
                        {
                            build_message(&message, 5, "Admin privileges required");
                        }
                        break;
                    case 6: //DELETE_ITEM_REQUEST
                        if (is_admin(username))
                        {
                            int room_id, item_id;
                            sscanf(message.payload, "%d|%d", &room_id, &item_id);

                            if (delete_item_from_room(1, room_id, item_id))
                            {
                                build_message(&message, 6, "Item deleted successfully");
                            }
                            else
                            {
                                build_message(&message, 6, "Item deletion failed");
                            }
                        }
                        else
                        {
                            build_message(&message, 6, "Admin privileges required");
                        }
                        break;
                    case 7: // LIST_ITEMS_REQUEST
                        if (is_admin(username))  
                        {
                            int room_id;
                            sscanf(message.payload, "%d", &room_id);  // Get the room ID from the message

                            list_room_items(room_id); 

                            build_message(&message, 7, "Items listed successfully.");
                            send(sock, &message, sizeof(Message), 0);
                        }
                        else
                        {
                            build_message(&message, 7, "Admin privileges required");
                            send(sock, &message, sizeof(Message), 0);
                        }
                        break;
                    case 8: // IS_ADMIN_REQUEST
                        if (is_admin(username))
                        {
                            build_message(&message, 7, "Admin");
                        }
                        else
                        {
                            build_message(&message, 7, "Not Admin");
                        }
                        break;
                    }
                    send(sock, &message, sizeof(Message), 0);
                }
                else
                {
                    close(sock);
                    client_socks[i] = 0;
                }
            }
        }
    }

    return 0;
}
