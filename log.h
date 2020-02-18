#ifndef LOG_H__
#define LOG_H__

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "crypto.h"

#define LOG_MAGIC 0xBADBEEF
#define LOG_EVENT_MAGIC 0xFEEDBEEF

#define CHECK_OK(x, y) if(!x) { fprintf(stderr, "%s error!\n", #y); exit(-1); }

struct _log_header
{
	uint64_t magic;
	unsigned char hash[32];
	unsigned char key[32];
	unsigned char initial_vector[32];
};

typedef struct
{
	struct _log_header header;
	char *token;
	
	FILE *log;
	unsigned char *decrypted;
	uint64_t decrypted_size;

	unsigned char *encrypted;
	uint64_t encrypted_size;

	uint64_t size;
	uint64_t current_offset;
} log_;

struct _log_event
{
	uint64_t magic;

	int timestamp;

	bool doctor;
	bool nurse;

	bool arrival;
	bool departure;

	int room_id;
};

typedef struct
{
	struct _log_event event;

	char *name;
	char *token;
} log_event;

log_* log_open(char *path, char *token);
bool log_verify(log_ *log);
bool log_decrypt(log_ *log);

bool log_encrypt(log_ *log);
bool log_sign(log_ *log);
bool log_close(log_ *log, char *path, bool wb);

bool log_check(log_ *log);
log_event* log_most_recent_event(log_ *log, char *name);

bool log_append(log_ *log, log_event *event);

bool log_print(log_ *log);
bool log_get_rooms(log_ *log, char *name);


#endif