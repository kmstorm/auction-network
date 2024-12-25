#include "countdown_timer.h"
#include "message.h"
#include "item.h"
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>

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
        if (seconds > 60)
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
    Item *item = find_item(room_id); // Tìm vật phẩm hiện tại
    if (!item)
    {
        printf("LOG_TIMER: No items available in Room ID %d\n", room_id);
        Message message;
        build_message(&message, 100, "No items available in this room\n");
        send(sock, &message, sizeof(Message), 0);
        return;
    }

    time_t current_time, end_time;
    end_time = time(NULL) + duration * 60; // Thời gian kết thúc ban đầu

    printf("LOG_TIMER: Starting countdown for Item ID %d: %s\n", item->id, item->name);

    while (1)
    {
        current_time = time(NULL);
        double seconds = difftime(end_time, current_time);

        if (seconds <= 0)
        {
            printf("LOG_TIMER: Auction ended for Item ID %d\n", item->id);
            item->status = 1; // Đánh dấu vật phẩm đã được bán
            save_all_items_to_file();

            // Gửi thông báo kết thúc vật phẩm
            Message message;
            build_message(&message, 100, "Auction ended for this item\n");
            send(sock, &message, sizeof(Message), 0);
            break;
        }

        // Nhận giá đấu mới từ client
        Message bid_message;
        if (recv(sock, &bid_message, sizeof(Message), MSG_DONTWAIT) > 0)
        {
            if (bid_message.message_type == 14) {
                break;
            }
            int user_id, bid_room_id;
            float bid_amount;

            // Parse giá đấu
            if (sscanf(bid_message.payload, "%d|%d|%f", &user_id, &bid_room_id, &bid_amount) == 3)
            {
                printf("LOG_TIMER: Received bid from User ID %d with amount %.2f\n", user_id, bid_amount);

                if (bid_room_id == room_id && bid_amount > item->current_price)
                {
                    item->current_price = bid_amount;
                    item->highest_bidder = user_id;

                    // Reset countdown nếu trong 30 giây cuối
                    if (seconds <= 30)
                    {
                        end_time = time(NULL) + 30;
                        printf("LOG_TIMER: Countdown reset to 30 seconds due to new bid\n");
                    }

                    save_all_items_to_file();

                    // Gửi phản hồi đến client
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

        // Hiển thị thông tin thời gian và giá
        char buffer[BUFFER_SIZE];
        snprintf(buffer, sizeof(buffer),
                 "Item ID: %d | Name: %s | Current Price: %.2f | Time Left: %.0f seconds\n",
                 item->id, item->name, item->current_price, seconds);
        Message message;
        build_message(&message, 100, buffer);
        send(sock, &message, sizeof(Message), 0);
        printf("LOG_TIMER: %s\n", buffer);

        // Ngủ tùy thuộc vào thời gian còn lại
        if (seconds <= 30)
        {
            sleep(1); // Thông báo mỗi giây trong 30 giây cuối
        }
        else
        {
            sleep(5); // Thông báo mỗi 5 giây trước 30 giây cuối
        }
    }
}
