#include "countdown_timer.h"
#include "message.h"
#include "auction_room.h"
#include "item.h"
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/select.h> // Include this header for select and related macros
#include <pthread.h>

void* countdown_thread(void* arg)
{
    CountdownArgs* args = (CountdownArgs*)arg;
    countdown_room_duration(args->sock, args->room_id, args->duration);
    free(args);
    return NULL;
}

void countdown_to_start_time(int sock, const char *start_time)
{
    struct tm tm = {0};
    time_t start_epoch, current_time;
    double seconds;

    if (strptime(start_time, "%Y-%m-%dT%H:%M", &tm) == NULL) {
        fprintf(stderr, "Error parsing start_time: %s\n", start_time);
        return;
    }

    start_epoch = mktime(&tm);

    printf("LOG_TIMER: Start time is %ld\n", start_epoch);

    while (1)
    {
        current_time = time(NULL);
        seconds = difftime(start_epoch, current_time);

        printf("LOG_TIMER: Current time: %ld, Seconds remaining: %.0f\n", current_time, seconds);

        if (seconds <= 0)
        {
            Message message;
            build_message(&message, 100, "Auction room has started!\n");
            send(sock, &message, sizeof(Message), 0);
            printf("LOG_TIMER: Auction room has started!\n");
            break;
        }

        int int_seconds = (int)seconds;
        char buffer[BUFFER_SIZE];
        snprintf(buffer, sizeof(buffer), "Time to start: %02d:%02d:%02d\n",
            int_seconds / 3600,
            (int_seconds / 60) % 60,
            int_seconds % 60);
        Message message;
        build_message(&message, 100, buffer);
        send(sock, &message, sizeof(Message), 0);
        printf("LOG_TIMER: Sent message: %s\n", buffer);

        // Calculate the time taken for processing
        time_t after_processing = time(NULL);
        double processing_time = difftime(after_processing, current_time);

        // Adjust sleep duration to maintain synchronization
        if (seconds >= 30)
        {
            sleep(5 - (int)processing_time);
        }
        else
        {
            sleep(1 - (int)processing_time);
        }
    }
}

void countdown_room_duration(int sock, int room_id, int duration)
{
    struct tm tm = {0};
    time_t start_epoch, end_time, current_time;
    double seconds;

    // Get the room details
    AuctionRoom *room = get_room_by_id(room_id);
    if (!room)
    {
        printf("LOG_TIMER: Room ID %d not found\n", room_id);
        Message message;
        build_message(&message, 100, "Room not found\n");
        send(sock, &message, sizeof(Message), 0);
        return;
    }

    // Parse the start time
    if (strptime(room->start_time, "%Y-%m-%dT%H:%M", &tm) == NULL)
    {
        fprintf(stderr, "Error parsing start_time: %s\n", room->start_time);
        return;
    }

    Item *item = find_item(room_id); // Find the current item
    if (!item)
    {
        printf("LOG_TIMER: No items available in Room ID %d\n", room_id);
        Message message;
        build_message(&message, 100, "No items available in this room\n");
        send(sock, &message, sizeof(Message), 0);
        return;
    }

    start_epoch = mktime(&tm);
    end_time = start_epoch + duration * 60;

    printf("LOG_TIMER: Starting countdown for Item ID %d: %s\n", item->id, item->name);

    // Set the socket to non-blocking mode
    fcntl(sock, F_SETFL, O_NONBLOCK);

    while (1)
    {
        current_time = time(NULL);
        seconds = difftime(end_time, current_time);
        printf("LOG_TIMER: Current time: %ld, Seconds remaining: %.0f\n", current_time, seconds);

        if (seconds <= 0)
        {
            printf("LOG_TIMER: Auction ended for Item ID %d\n", item->id);
            item->status = 1; // Mark the item as sold
            save_all_items_to_file();

            // Send auction end notification
            Message message;
            build_message(&message, 100, "Auction ended for this item\n");
            send(sock, &message, sizeof(Message), 0);
            break;
        }

        // Use select to wait for incoming messages without blocking
        fd_set read_fds;
        struct timeval timeout;
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int activity = select(sock + 1, &read_fds, NULL, NULL, &timeout);
        if (activity > 0 && FD_ISSET(sock, &read_fds))
        {
            // Receive new bid from client
            Message bid_message;
            if (recv(sock, &bid_message, sizeof(Message), 0) > 0)
            {
                int user_id, bid_room_id;
                float bid_amount;

                // Parse the bid
                if (sscanf(bid_message.payload, "%d|%d|%f", &user_id, &bid_room_id, &bid_amount) == 3)
                {
                    printf("LOG_TIMER: Received bid from User ID %d with amount %.2f\n", user_id, bid_amount);

                    if (bid_room_id == room_id && bid_amount > item->current_price)
                    {
                        item->current_price = bid_amount;
                        item->highest_bidder = user_id;

                        // Reset countdown if within the last 30 seconds
                        if (seconds <= 30)
                        {
                            end_time = time(NULL) + 30;
                            printf("LOG_TIMER: Countdown reset to 30 seconds due to new bid\n");
                        }

                        save_all_items_to_file();

                        // Send response to client
                        char buffer[BUFFER_SIZE];
                        snprintf(buffer, sizeof(buffer),
                                 "Bid accepted: User ID %d is now the highest bidder with %.2f\n",
                                 user_id, bid_amount);
                        build_message(&bid_message, 101, buffer); // 101: Custom message type for bid response
                        send(sock, &bid_message, sizeof(Message), 0);
                    }
                    else
                    {
                        printf("LOG_TIMER: Bid rejected: %.2f is less than current price %.2f\n",
                               bid_amount, item->current_price);
                    }
                }
            }
        }

        // Display time and price information
        char buffer[BUFFER_SIZE];
        snprintf(buffer, sizeof(buffer),
                 "Item ID: %d | Name: %s | Current Price: %.2f | Time Left: %.0f seconds\n",
                 item->id, item->name, item->current_price, seconds);
        Message message;
        build_message(&message, 100, buffer);
        send(sock, &message, sizeof(Message), 0);
        printf("LOG_TIMER: %s\n", buffer);

        // Calculate the time taken for processing
        time_t after_processing = time(NULL);
        double processing_time = difftime(after_processing, current_time);

        // Adjust sleep duration to maintain synchronization
        if (seconds >= 30)
        {
            sleep(5 - (int)processing_time);
        }
        else
        {
            sleep(1 - (int)processing_time);
        }
    }
}
