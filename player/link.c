#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include <string.h>
#include "link.h"
#include "player.h"
#include <time.h>

extern struct Node *head;

int InitLink()
{
	head = (Node *)malloc(sizeof(Node) * 1);
	if (NULL == head)
	{
		return FAILURE;
	}

	head->next = head;
	head->prior = head;

	return SUCCESS;
}

void ClearLink()
{
	if (NULL == head)
	{
		return;
	}

	Node *p = head->next;
	while (p != head)
	{
		Node *n = p->next;
		free(p);
		p = n;
	}
	head->next = head;
	head->prior = head;
}

int InsertLink(Node *h, const char *name)
{
	if (NULL == h || NULL == name)
	{
		return FAILURE;
	}

	Node *end = h->prior;

	Node *n = (Node *)malloc(sizeof(Node) * 1);
	if (NULL == n)
	{
		return FAILURE;
	}

	n->next = h;
	strncpy(n->music_name, name, sizeof(n->music_name) - 1);
	n->music_name[sizeof(n->music_name) - 1] = '\0';
	end->next = n;
	n->prior = end;
	h->prior = n;

	return SUCCESS;
}

//找不到当前曲名时不要在循环链表里空转
static Node *find_named(const char *cur)
{
	if (NULL == head || head->next == head || NULL == cur)
	{
		return NULL;
	}

	Node *p = head->next;
	Node *start = p;
	do
	{
		if (strcmp(p->music_name, cur) == 0)
		{
			return p;
		}
		p = p->next;
		if (p == head)
		{
			p = p->next;
		}
	} while (p != start);

	return NULL;
}

void FindNextMusic(const char *cur, int mode, char *next)
{
	if (head->next == head)
	{
		next[0] = '\0';
		return;
	}

	if (mode == CIRCLE)
	{
		strcpy(next, cur);
		return;
	}
	else if (mode == SEQUENCEMODE)
	{
		Node *p = find_named(cur);
		if (NULL == p)
		{
			strcpy(next, head->next->music_name);
			return;
		}

		if (p->next == head)
		{
			strcpy(next, head->next->music_name);
		}
		else
		{
			strcpy(next, p->next->music_name);
		}
		return;
	}
	else
	{
		Node *p = head->next;
		srand(time(NULL));
		int num = rand() % 100;

		int i;
		for (i = 0; i < num; i++)
		{
			p = p->next;
		}

		if (p == head)
		{
			strcpy(next, head->next->music_name);
		}
		else
		{
			strcpy(next, p->music_name);
		}

		return;
	}
}

void PriorMusic(const char *cur, int mode, char *prior)
{
	if (head->next == head)
	{
		prior[0] = '\0';
		return;
	}

	if (mode == SEQUENCEMODE || mode == CIRCLE)
	{
		Node *p = find_named(cur);
		if (NULL == p)
		{
			strcpy(prior, head->prior->music_name);
			return;
		}

		if (p == head->next)
		{
			strcpy(prior, head->prior->music_name);
		}
		else
		{
			strcpy(prior, p->prior->music_name);
		}
		return;
	}
	else
	{
		Node *p = head->next;
		srand(time(NULL));
		int num = rand() % 100;

		int i;
		for (i = 0; i < num; i++)
		{
			p = p->next;
		}

		if (p == head)
		{
			strcpy(prior, p->next->music_name);
		}
		else
		{
			strcpy(prior, p->music_name);
		}

		return;
	}
}

void NextMusic(const char *cur, int mode, char *next)
{
	if (head->next == head)
	{
		next[0] = '\0';
		return;
	}

	if (mode == SEQUENCEMODE || mode == CIRCLE)
	{
		Node *p = find_named(cur);
		if (NULL == p)
		{
			strcpy(next, head->next->music_name);
			return;
		}

		if (p->next == head)
		{
			strcpy(next, head->next->music_name);
		}
		else
		{
			strcpy(next, p->next->music_name);
		}
		return;
	}
	else
	{
		Node *p = head->next;
		srand(time(NULL));
		int num = rand() % 100;

		int i;
		for (i = 0; i < num; i++)
		{
			p = p->next;
		}

		if (p == head)
		{
			strcpy(next, p->next->music_name);
		}
		else
		{
			strcpy(next, p->music_name);
		}

		return;
	}
}
