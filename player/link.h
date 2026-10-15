#ifndef LINK_H
#define LINK_H

struct Node
{
	char music_name[128];
	struct Node *next;
	struct Node *prior;
};
typedef struct Node Node;

int InitLink();
void ClearLink();
void FindNextMusic(const char *cur, int mode, char *next);
int InsertLink(Node *h, const char *name);
void PriorMusic(const char *cur, int mode, char *prior);
void NextMusic(const char *cur, int mode, char *next);

#endif
