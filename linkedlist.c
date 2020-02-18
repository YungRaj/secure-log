#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "linkedlist.h"

bool staff_equal(struct staff *s1, struct staff *s2)
{
	return s1->doctor == s2->doctor &&
		   strcmp(s1->name, s2->name) == 0;
}

struct node* get_tail(struct node *head)
{
	struct node *current = head;

	while(current->next)
		current = current->next;

	return current;
}

bool list_exists(struct node *head, char *name)
{
	struct node *current;

	current = head;

	while(current)
	{
		if(strcmp(current->staff.name, name) == 0)
			return true;

		current = current->next;
	}

	return false;
}

struct node* list_add(struct node *head, struct staff *staff)
{
	struct node *node;
	struct node *tail;

	node = malloc(sizeof(struct node));
	memset(node, 0x0, sizeof(struct node));
	memcpy(&node->staff, staff, sizeof(struct staff));

	if(head == NULL)
		return node;

	tail = get_tail(head);

	tail->next = node;
	node->prev = tail;

	return head;
}


struct node* list_remove(struct node *head, struct staff *staff)
{
	struct node *current;

	if(staff_equal(staff,&head->staff))
	{
		struct node *node = head->next;
		node->prev = NULL;

		free(head);

		return node;
	}

	current = head->next;

	while(current->next)
	{
		if(staff_equal(staff,&current->staff))
		{
			struct node *next = current->next;
			struct node *prev = current->prev;

			next->prev = prev;
			prev->next = next;

			free(current);

			return head;
		}

		current = current->next;
	}

	if(staff_equal(staff,&current->staff))
	{
		struct node *prev = current->prev;
		prev->next = NULL;

		free(current);
	}

	return head;
}

void list_remove_all(struct node *head)
{
	struct node *current;

	current = head;

	while(current->next)
	{
		struct node *next = current->next;
		
		current->next = NULL;
		current->prev = NULL;

		free(current);

		current = next;
	}

}

