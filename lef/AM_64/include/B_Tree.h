#ifndef B_TREE_H_
#define B_TREE_H_
#include "bf.h"

typedef struct split_d{
	int *pointer;
	void *value;
}Split_Data;

int Initialize_Root(int,void *,int,int);
int compare(void *,void *,char,int);
int sort(int,int,void *,void *);
Split_Data * split(int ,BF_Block *,char *,int ,void *,void *);
int traverse(int,int,void*,int *);
int op_function(void * ,void *,char ,int ,int );

#endif