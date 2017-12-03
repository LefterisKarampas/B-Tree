#ifndef _LIST_H_
#define _LIST_H_

typedef struct node{
	int block;					//Id Number of block
	struct node * next;			//Pointer to next node	
}Node;

typedef Node * List;

//Create a list and return the pointer to head
List * List_Create();

//Insert as first a new value
int List_Push(List *,int);

//Pop the first value of the list
int List_Pop(List *);

//Remove all nodes and destroy the list
void List_Destroy(List *);

//Print list's values
void List_Print(List *);

#endif