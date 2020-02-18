#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include <getopt.h>

#include "log.h"

// -T <timestamp> -K <token> (-D <doctor-name> | -N <nurse-name>) (-A | -L) [-R <room-id>] -F <log

void print_usage()
{
    fprintf(stdout, "-T <timestamp> -K <token> (-D <doctor-name> | -N <nurse-name>) (-A | -L) [-R <room-id>] -F <log> -B <batch>\n");
}

static struct option options[] =
{
    {"timestamp",  required_argument, 0, 'T'},
    {"token",    required_argument, 0, 'K'},
    {"doctor-name",    required_argument, 0, 'D'},
    {"nurse-name", required_argument, 0, 'N'},
    {"room-id", required_argument, 0, 'R'},
    {"log", required_argument, 0, 'F'},
    {"batch", required_argument, 0, 'B'},
    {"arrival", no_argument, 0, 'A'},
    {"departure", no_argument, 0, 'L'},
    {0, 0, 0, 0}
};


int main(int argc, char *argv[])
{
    log_ *log;

    int timestamp;
    
    char *token;
    char *name;
    char *date;

    bool doctor;
    bool nurse;

    int room_id;

    char *log_file;
    char *batch_file;

    bool arrival;
    bool departure;

    timestamp = 0;
    token = name = date = NULL;
    room_id = -1;
    log_file = batch_file = NULL;
    doctor = nurse = arrival = departure = false;

    int c;

    do
    {
        int c;
        int opt_index = 0;

        c = getopt_long(argc, argv, "T:K:D:N:R:F:B:AL", options, &opt_index);

        if(c == -1)
            break;

        switch(c)
        {
            case 0:
                if(options[opt_index].flag != 0)
                    break;

                printf("option %s", options[opt_index].name);

                if(optarg)
                    printf(" with argument %s", optarg);

                printf("\n");

                break;
            case 'T':
                date = optarg;

                int year, month, day, hour, min, sec = 0;
                struct tm breakdown = {0};

                sscanf(date, "%d.%d.%d-%2d:%2d:%2d", &month, &day, &year, &hour, &min, &sec);

                breakdown.tm_year = year - 1900;
                breakdown.tm_mon = month - 1;
                breakdown.tm_mday = day;
                breakdown.tm_hour = hour;
                breakdown.tm_min = min;
                breakdown.tm_sec = sec;

                timestamp = mktime(&breakdown);

                break;

            case 'K':
                token = optarg;

                break;
            case 'D':
                name = optarg;
                doctor = true;

                break;
            case 'N':
                name = optarg;
                nurse = true;

                break;

            case 'R':
                room_id = atoi(optarg);

                break;

            case 'F':
                log_file = optarg;

                break;

            case 'B':
                batch_file = optarg;

                break;

            case 'A':
                arrival = true;

                break;

            case 'L':
                departure = true;

                break;

            case '?':
                break;

            default:
                exit(0);
        }

    } while(c != -1);

    bool ok;

    if(batch_file)
    {
        FILE *batch = fopen(batch_file, "r");
        
        char *line = NULL;
        size_t len;
        size_t read;

        if(!batch)
        {
            fprintf(stderr, "Could not open batch file named %s\n", batch_file);

            exit(-1);
        }

        while ((read = getline(&line, &len, batch)) != -1)
        {
            
        }
    }

    if(!log_file)
    {
        print_usage();

        exit(-1);
    }

    if(nurse && doctor)
    {
        fprintf(stderr, "Can't be a doctor and nurse both %s!\n", name);

        exit(-1);
    }

    if(!arrival && !departure)
    {
        fprintf(stderr, "No event!\n");

        exit(-1);
    }

    log = log_open(log_file, token);

    if(!log)
    {
        fprintf(stderr, "Could not open log file named %s!\n", log_file);

        exit(-1);
    }

    ok = log_decrypt(log);
    CHECK_OK(ok, log_decrypt)
    ok = log_verify(log);
    CHECK_OK(ok, log_verify);
    ok = log_check(log);
    CHECK_OK(ok, log_check);

    log_event *e = malloc(sizeof(log_event));
    e->event.magic = LOG_EVENT_MAGIC;
    e->event.timestamp = timestamp;
    e->event.doctor = !nurse && doctor;
    e->event.nurse = !doctor && nurse;
    e->event.arrival = !departure && arrival;
    e->event.departure = !arrival && departure;
    e->event.room_id = room_id;
    e->token = token;
    e->name = name;

    log_event *most_recent = log_most_recent_event(log, name);

    bool in_hospital = false;
    int current_room = -1;

    if(most_recent)
    {
        if(most_recent->event.arrival)
        {
            in_hospital = true;
            
            if(most_recent->event.room_id != -1)
                current_room = most_recent->event.room_id;

        } else if(most_recent->event.departure && most_recent->event.room_id != -1)
        {
            in_hospital = true;
        }
    }

    if(most_recent &&
       most_recent->event.doctor != doctor &&
       most_recent->event.nurse != nurse)
    {
        ok = log_close(log, log_file, false);
        CHECK_OK(ok, log_close);

        fprintf(stderr, "Invalid staff member provided for staff member named %s\n", name);

        exit(-1);
    } else if((departure && !in_hospital) ||
              (departure && current_room != -1 && current_room != room_id) ||
              (departure && current_room == -1 && room_id != -1) ||
              (arrival && !in_hospital && room_id != -1) ||
              (arrival && current_room != -1 && room_id == -1) ||
              (arrival && current_room != -1 && room_id != -1))
    {
        ok = log_close(log, log_file, false);
        CHECK_OK(ok, log_close);

        fprintf(stderr, "Invalid event!\n");

        exit(-1);
    }

    most_recent = log_most_recent_event(log, NULL);

    if(most_recent && timestamp <= most_recent->event.timestamp)
    {
        ok = log_close(log, log_file, false);
        CHECK_OK(ok, log_close);

        fprintf(stderr, "Invalid event!\n");

        exit(-1);
    } else if(most_recent && strcmp(token, most_recent->token) != 0)
    {
        ok = log_close(log, log_file, false);
        CHECK_OK(ok, log_close);

        fprintf(stderr, "Invalid token!\n");

        exit(-1);
    }

    free(most_recent);

    ok = log_append(log, e);
    CHECK_OK(ok, log_append);

    ok = log_sign(log);
    CHECK_OK(ok, log_append);
    ok = log_encrypt(log);
    CHECK_OK(ok, log_encrypt);
    ok = log_close(log, log_file, true);
    CHECK_OK(ok, log_close);
    
}