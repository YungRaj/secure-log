#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "log.h"
#include "crypto.h"
#include "linkedlist.h"

log_* log_open(char *path, char *token)
{
	log_ *log;

	log = malloc(sizeof(log_));
	memset(log, 0x0, sizeof(log_));
	log->log = fopen(path, "ab+");

	if(!log->log)
	{
		free(log);

		return NULL;
	}

	fseek(log->log, 0, SEEK_END);
	log->size = ftell(log->log);
	log->current_offset = 0;
	fseek(log->log, 0, SEEK_SET);

	log->token = token;

	if(log->size)
	{
		log->encrypted = malloc(log->size - sizeof(struct _log_header));
		fseek(log->log, sizeof(struct _log_header), SEEK_SET);
		fread(log->encrypted, 1, log->size - sizeof(struct _log_header), log->log);

		struct _log_header header;
		fseek(log->log, 0, SEEK_SET);
		fread(&header, 1, sizeof(struct _log_header), log->log);

		if(header.magic != LOG_MAGIC)
		{
			fclose(log->log);
			free(log);

			return NULL;
		}
		
		memcpy(&log->header, &header, sizeof(struct _log_header));

		log->encrypted_size = log->size - sizeof(struct _log_header);
	}

	log->header.magic = LOG_MAGIC;

	return log;
}

bool log_verify(log_ *log)
{
	bool verified = false;

	unsigned char hash[SHA256_DIGEST_LENGTH];

	SHA256_CTX sha256;

	if(log->size == 0)
		return true;

    SHA256_Init(&sha256);
    SHA256_Update(&sha256, log->decrypted, log->decrypted_size);
    SHA256_Final(hash, &sha256);

    verified = (memcmp(hash, &log->header.hash, SHA256_DIGEST_LENGTH) == 0);

	return verified;
}

bool log_decrypt(log_ *log)
{
	unsigned char *plaintext;
	int plaintext_length;

	if(log->size == 0)
		return true;

	plaintext = malloc(log->size - sizeof(struct _log_header));

	plaintext_length = crypto_decrypt(log->encrypted,
									  log->encrypted_size,
									  (unsigned char*) &log->header.key,
									  (unsigned char*) &log->header.initial_vector,
									  plaintext);

	if(!plaintext_length)
		return false;

	free(log->encrypted);
	log->encrypted = NULL;
	log->encrypted_size = 0;

	log->decrypted = plaintext;
	log->decrypted_size = plaintext_length;

	return true;
}

bool log_encrypt(log_ *log)
{
	bool ok;

	unsigned char *ciphertext;
	int ciphertext_length;

	unsigned char key[32];
	unsigned char iv[32];

	unsigned char *key_data;
	unsigned int salt[] = {12345 , 54321};

	if(log->size == 0)
		return false;

	key_data = crypto_generate_key(log->token);

	ok = crypto_init(key_data,
			 		 AES_KEY_LENGTH,
			 		 (unsigned char*) salt,
             		 key,
             		 iv);
	
	memcpy(&log->header.key, key, AES_KEY_LENGTH);
	memcpy(&log->header.initial_vector, iv, AES_KEY_LENGTH);

	ciphertext = malloc(log->size - sizeof(struct _log_header));

	ciphertext_length = crypto_encrypt(log->decrypted,
									   log->decrypted_size,
									   key,
									   iv,
									   ciphertext);

	memcpy(&log->header.key, key, AES_KEY_LENGTH);
	memcpy(&log->header.initial_vector, iv, AES_KEY_LENGTH);

	free(log->decrypted);
	log->decrypted = NULL;
	log->decrypted_size = 0;
	
	log->encrypted = ciphertext;
	log->encrypted_size = ciphertext_length;

	return true;
}

bool log_sign(log_ *log)
{
	bool verified = false;

	unsigned char hash[SHA256_DIGEST_LENGTH];

	SHA256_CTX sha256;

    SHA256_Init(&sha256);
    SHA256_Update(&sha256, log->decrypted, log->decrypted_size);
    SHA256_Final(hash, &sha256);

    memcpy(&log->header.hash, hash, SHA256_DIGEST_LENGTH);

    return true;
}

bool log_close(log_ *log, char *path, bool wb)
{
	bool ok = false;

	int bytes_written;

	if(wb)
	{
		log->log = freopen(NULL,"wb+",log->log);

		bytes_written = fwrite(&log->header, 1, sizeof(struct _log_header), log->log);

		if(bytes_written != sizeof(struct _log_header))
			goto done;

		bytes_written = fwrite(log->encrypted, 1, log->encrypted_size, log->log);

		if(bytes_written != log->encrypted_size)
			goto done;

		ok = true;
	} else
		ok = true;

	goto done;

done:
	fclose(log->log);
	free(log->encrypted);
	free(log);

	return ok;
}

bool log_append(log_ *log, log_event *event)
{
	unsigned char *tmp;

	unsigned char *new_log;
	uint64_t new_size;

	if(!event)
		return false;

	if(!log->decrypted && log->size)
		return false;

	new_size = log->decrypted_size + sizeof(struct _log_event) + strlen(event->name) + strlen(event->token) + 2;
	new_log = malloc(new_size);

	tmp = new_log;

	memcpy(tmp, log->decrypted, log->decrypted_size);
	tmp += log->decrypted_size;
	memcpy(tmp, (struct _log_event*)event, sizeof(struct _log_event));
	tmp += sizeof(struct _log_event);
	strncpy((char*)tmp, event->name, strlen(event->name) + 1);
	tmp += strlen(event->name) + 1;
	strncpy((char*)tmp, event->token, strlen(event->token) + 1);

	free(log->decrypted);
	log->decrypted = new_log;
	log->decrypted_size = new_size;
	log->size = new_size + sizeof(struct _log_header);

	return true;
}

bool log_check(log_ *log)
{
	unsigned char *tmp;
	char *previous_token = NULL;

	if(log->size == 0)
		return true;

	tmp = log->decrypted;

	while(tmp < log->decrypted + log->decrypted_size)
	{
		struct _log_event *event = (struct _log_event*) tmp;
		char *name = (char*)tmp + sizeof(struct _log_event);
		char *token = (char*)name + strlen(name) + 1;

		if(event->magic != LOG_EVENT_MAGIC)
			return false;

		if(previous_token && 
		   strcmp(previous_token, token) != 0)
			return false;

		previous_token = token;
		tmp = (unsigned char*)token + strlen(token) + 1;
	}

	return true;
}

log_event* log_most_recent_event(log_ *log, char *staff_name)
{
	log_event *event;
	struct _log_event *most_recent;
	unsigned char *tmp;

	if(log->size == 0)
		return NULL;

	most_recent = NULL;

	tmp = log->decrypted;

	int timestamp = 0;

	while(tmp < log->decrypted + log->decrypted_size)
	{
		struct _log_event *event = (struct _log_event*) tmp;
		char *name = (char*)tmp + sizeof(struct _log_event);
		char *token = (char*)name + strlen(name) + 1;

		if(staff_name != NULL &&
		   strcmp(staff_name, name) == 0)
		{
			most_recent = event;
		} else if(staff_name == NULL &&
				  event->timestamp > timestamp)
		{
			most_recent = event;
		}

		tmp = (unsigned char*) token + strlen(token) + 1;
	}

	if(!most_recent)
		return NULL;

	event = malloc(sizeof(log_event));
	memset(event, 0x0, sizeof(log_event));

	memcpy(&event->event, most_recent, sizeof(struct _log_event));
	event->name = (char*)most_recent + sizeof(struct _log_event);
	event->token = (char*)event->name + strlen(event->name) + 1;

	return event;
}

void log_get_staff(struct node *head, bool doctor)
{
	struct node *current;

	current = head;

	if(head == NULL)
		return;

	if(doctor)
		printf("doctors = ");
	else
		printf("nurses = ");

	while(current)
	{
		if((current->staff.doctor ^ doctor) == false)
		{
			printf("%s ", current->staff.name);
			
			if(current->staff.room_id != -1)
				printf("%d ", current->staff.room_id);
		}

		current = current->next;
	}

	printf("\n");
}

bool log_print(log_ *log)
{
	struct node *list;
	unsigned char *tmp;

	tmp = log->decrypted;
	list = NULL;

	while(tmp < log->decrypted + log->decrypted_size)
	{
		struct _log_event *event = (struct _log_event*)tmp;
		char *name = (char*)tmp + sizeof(struct _log_event);
		char *token = (char*)name + strlen(name) + 1;

		log_event *e = log_most_recent_event(log, name);

		if(!list_exists(list, e->name))
		{
			struct staff staff = {e->event.doctor, e->name, e->event.room_id};

			list = list_add(list, &staff);
		}

		tmp = (unsigned char*)token + strlen(token) + 1;
	}

	log_get_staff(list, true);
	log_get_staff(list, false);

	list_remove_all(list);

	return true;
}

bool log_get_rooms(log_ *log, char *staff_name)
{
	unsigned char *tmp;

	tmp = log->decrypted;

	fprintf(stdout, "{");

	while(tmp < log->decrypted + log->decrypted_size)
	{
		struct _log_event *event = (struct _log_event*)tmp;
		char *name = (char*)tmp + sizeof(struct _log_event);
		char *token = (char*)name + strlen(name) + 1;

		if(strcmp(staff_name, name) == 0)
		{
			fprintf(stdout, "%d ", event->room_id);
		}

		tmp = (unsigned char*)token + strlen(token) + 1;
	}

	fprintf(stdout, "}\n");

	return true;
}