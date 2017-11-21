#ifndef _LIST_H_
#define _LIST_H_

typedef struct node{
	int block;
	struct node * next;
}Node;

typedef Node * List;

List * List_Create();
int List_Push(List *,int);
int List_Pop(List *);
void List_Destroy(List *);
void List_Print(List *);

#endif