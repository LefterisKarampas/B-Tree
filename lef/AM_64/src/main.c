#include "AM.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defn.h"


int main(void){
	AM_Init();
	char name[40];
	strcpy(name,"lef");
	if(AM_CreateIndex(name,INTEGER,sizeof(int),INTEGER,sizeof(int)) != AME_OK){
		fprintf(stderr,"%s\n","CREATE_INDEX");
		exit(1);
	}
	int p;
	if((p = AM_OpenIndex(name) < 0)){
		fprintf(stderr,"%s\n","OPEN_INDEX");
		exit(1);
	}



	int v1 = 10;
	int v2 = 1;
	AM_InsertEntry(p,(void *)&v1,(void*)&v2);
	 v1 = 30;
	 v2 = 1;
	AM_InsertEntry(p,(void *)&v1,(void*)&v2);
	 v1 = 20;
	 v2 = 1;
	AM_InsertEntry(p,(void *)&v1,(void*)&v2);
	v1 = 50;
	 v2 = 1;
	AM_InsertEntry(p,(void *)&v1,(void*)&v2);
	int i;
	for(i=-1000;i<=1500;i++){
		v1 = i;
	 	v2 = i;
		AM_InsertEntry(p,(void *)&v1,(void*)&v2);
		if(i==0){
			v1 = -251;
			AM_InsertEntry(p,(void *)&v1,(void*)&v2);
		}
	}
	v1 = 180;
	v2 = 180;
	AM_InsertEntry(p,(void *)&v1,(void*)&v2);
	AM_Print(p);
	v1 = 180;

	i = AM_OpenIndexScan(p,2,(void *)&v1);
	int * t;
	while((t = (int *)AM_FindNextEntry(i))){
		printf("EntryValue: %d\n",*t);
	}
	/*AM_OpenIndexScan(p,2,(void *)&v1);
	AM_OpenIndexScan(p,3,(void *)&v1);
	AM_OpenIndexScan(p,4,(void *)&v1);
	AM_OpenIndexScan(p,5,(void *)&v1);
	AM_OpenIndexScan(p,6,(void *)&v1);*/



	if(AM_CloseIndex(p) != AME_OK){
		fprintf(stderr,"%s\n","CLOSE_INDEX");
		exit(1);
	}
	//AM_DestroyIndex(name);
	AM_Close();
	return 0;
}