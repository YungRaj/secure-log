#ifndef LINKEDLIST_H__
#define LINKEDLIST_H__

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

struct staff
{
	bool doctor;

	char *name;
	int room_id;
};

struct node
{
	struct node *next;
	struct node *prev;

	struct staff staff;
};

bool list_exists(struct node *head, char *name);
struct node* list_add(struct node *head, struct staff *staff);
struct node* list_remove(struct node *head, struct staff *staff);
void list_remove_all(struct node *head);

#endif