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
	for(i=-250;i<250;i++){
		v1 = i;
	 	v2 = 1;
		AM_InsertEntry(p,(void *)&v1,(void*)&v2);
	}

	AM_Print(p);



	if(AM_CloseIndex(p) != AME_OK){
		fprintf(stderr,"%s\n","CLOSE_INDEX");
		exit(1);
	}
	AM_DestroyIndex(name);
	AM_Close();
	return 0;
}