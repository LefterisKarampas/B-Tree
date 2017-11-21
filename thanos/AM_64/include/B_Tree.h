#ifndef B_TREE_H_
#define B_TREE_H_

typedef struct split_d{
	int pointer;
	void *value;
}Split_Data;

int Initialize_Root(int,void *,int,int);
int compare(void *,void *,char,int);
int sort(int,int,void *,void *);
Split_Data * split(int ,int ,void *,void *);
int traverse(int,int,void*);

#endif