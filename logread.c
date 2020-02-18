#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <getopt.h>

#include "log.h"

// -T <timestamp> -K <token> (-D <doctor-name> | -N <nurse-name>) (-A | -L) [-R <room-id>] -F <log>

void print_usage()
{
    fprintf(stdout, "log-K <token> -R (-D <name> | -N <name>) -F <log>\n");
}

static struct option options[] =
{
    {"token",    required_argument, 0, 'K'},
    {"doctor-name",    required_argument, 0, 'D'},
    {"nurse-name", required_argument, 0, 'N'},
    {"log", required_argument, 0, 'F'},
    {"rooms", no_argument, 0, 'R'},
    {"status", no_argument, 0, 'S'},
    {0, 0, 0, 0}
};


int main(int argc, char *argv[])
{
    log_ *log;

    bool status;
    bool rooms;

    bool nurse, doctor;

    char *token;
    char *name;

    char *log_file;

    status = rooms = nurse = doctor = false;
    token = name = log_file = NULL;

    int c;

    do
    {
        int opt_index = 0;

        c = getopt_long(argc, argv, "K:D:N:F:RS", options, &opt_index);

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
            case 'K':
                token = optarg;

                break;
            case 'D':
                doctor = true;
                name = optarg;

                break;
            case 'N':
                nurse = true;
                name = optarg;

                break;
            case 'F':
                log_file = optarg;

                break;

            case 'R':
                rooms = true;

                break;

            case 'S':
                status = true;

                break;

            case '?':
                break;

            default:
                exit(0);
        }

    } while(c != -1);

    bool ok;

    if(!log_file || access(log_file, 0) != 0)
    {
        print_usage();

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

    log_event *most_recent = log_most_recent_event(log, name);

    if(most_recent &&
       most_recent->event.doctor != doctor &&
       most_recent->event.nurse != nurse)
    {
        fprintf(stderr, "Invalid staff member provided for staff member named %s\n", name);

        exit(-1);
    }

    most_recent = log_most_recent_event(log, NULL);

    if(most_recent && strcmp(token, most_recent->token) != 0)
    {
        ok = log_close(log, log_file, false);
        CHECK_OK(ok, log_close);

        fprintf(stderr, "Invalid token!\n");

        exit(-1);
    }

    free(most_recent);

    if(rooms && name)
    {
        log_get_rooms(log, name);
    } else if(status)
    {
        ok = log_print(log);
        CHECK_OK(ok, log_print);
    }

    ok = log_close(log, log_file, false);
    CHECK_OK(ok, log_close);
    
}
