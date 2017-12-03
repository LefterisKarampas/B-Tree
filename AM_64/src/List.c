#include "../include/List.h"
#include <stdlib.h>
#include <stdio.h>

//Create a new list and return the pointer to it
List * List_Create(){
	List * list;
	list = malloc(sizeof(List));
	*list = NULL;
	return list;
}

//Destroy the List
void List_Destroy(List *list){
	Node * temp = *list;
	while(temp != NULL){
		*list = temp->next;
		free(temp);
		temp = *list;
	}
}


//Insert new object as first in List
int List_Push(List *list,int pointer){
	Node * temp = (Node *)malloc(sizeof(Node));
	if(temp == NULL){
		return -1;
	}
	temp->block = pointer;
	temp->next = *list;
	*list = temp;
	return 0;
}

//Pop the first element of the List
int List_Pop(List *list){
	Node * temp;
	if(*list != NULL){
		int p;
		temp = (*list)->next;
		p = (*list)->block;
		free(*list);
		*list = temp;
		return p;
	}
	return -1;
}

void List_Print(List *list){
	Node * temp = *list;
	while(temp){
		printf("%d->\n",temp->block);
		temp = temp->next;
	}
}
