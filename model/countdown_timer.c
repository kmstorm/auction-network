#include "countdown_timer.h"
#include "message.h"
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

void countdown_room_duration(int sock, const char *start_time, int duration)
{
    struct tm tm = {0};
    time_t start_epoch, end_time, current_time;
    double seconds;

    if (strptime(start_time, "%Y-%m-%dT%H:%M", &tm) == NULL) {
        fprintf(stderr, "Error parsing start_time: %s\n", start_time);
        return;
    }

    start_epoch = mktime(&tm);
    end_time = start_epoch + duration * 60;

    printf("LOG_TIMER: Start time is %ld\n", start_epoch);
    printf("LOG_TIMER: End time is %ld\n", end_time);

    while (1)
    {
        current_time = time(NULL);
        seconds = difftime(end_time, current_time);

        printf("LOG_TIMER: Current time: %ld, Seconds remaining: %.0f\n", current_time, seconds);

        if (seconds <= 0)
        {
            Message message;
            build_message(&message, 100, "Auction room has ended!\n");
            send(sock, &message, sizeof(Message), 0);
            printf("LOG_TIMER: Auction room has ended!\n");
            break;
        }

        int int_seconds = (int)seconds;
        char buffer[BUFFER_SIZE];
        snprintf(buffer, sizeof(buffer), "Time remaining: %02d:%02d:%02d\n",
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
