#include "AM.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defn.h"


int main(void){
	AM_Init();
	char name[40];
	strcpy(name,"lef");
	if(AM_CreateIndex(name,STRING,10,INTEGER,sizeof(int)) != AME_OK){
		AM_PrintError("AM_CreateIndex");
		exit(1);
	}
	int p;
	if((p = AM_OpenIndex(name) < 0)){
		AM_PrintError("AM_OpenIndex");
		exit(1);
	}
	/*int v1 = 10;
	int v2 = 10;
	int k = 1;
	AM_InsertEntry(p,(void *)&v1,(void*)&v2);
	for(int i=7300;i>-7000;i--){
		if(i % 2 == 0){
			;//k *=-1;
		}
		v1 = k*i;
		v2 = k*i;
		AM_InsertEntry(p,(void *)&v1,(void*)&v2);
	}*/


	char v1[10];
	sprintf(v1,"%d",10);
	int v2 = 10;
	AM_InsertEntry(p,(void *)&v1,(void*)&v2);
	 sprintf(v1,"%d",20);
	 v2 = 30;
	AM_InsertEntry(p,(void *)&v1,(void*)&v2);
	 sprintf(v1,"%d",30);
	 v2 = 20;
	AM_InsertEntry(p,(void *)&v1,(void*)&v2);
	sprintf(v1,"%d",50);
	 v2 = 50;
	AM_InsertEntry(p,(void *)&v1,(void*)&v2);
	int i;
	for(i=5000;i>-5000;i--){
		sprintf(v1,"%d",i);
	 	v2 = i;
		AM_InsertEntry(p,(void *)&v1,(void*)&v2);
		if(i==0){
			sprintf(v1,"%d",-251);
			v2 = -251;
			AM_InsertEntry(p,(void *)&v1,(void*)&v2);
		}
	}
	//exit(1);
	/*
	for(i=2500;i>1500;i--){
		v1 = i;
	 	v2 = i;
		AM_InsertEntry(p,(void *)&v1,(void*)&v2);
		if(i==0){
			v1 = -251;
			v2 = -251;
			AM_InsertEntry(p,(void *)&v1,(void*)&v2);
		}
	}
	for(i=3000;i>-18;i--){
		v1 = i;
	 	v2 = i;
		AM_InsertEntry(p,(void *)&v1,(void*)&v2);
		if(i==0){
			v1 = -251;
			v2 = -251;
			AM_InsertEntry(p,(void *)&v1,(void*)&v2);
		}
	}
	for(i=0;i<64;i++){
		v1 = 15;
		v2 = 15;
		AM_InsertEntry(p,(void *)&v1,(void*)&v2);
	}*/
	/*sprintf(v1,"%d",180);
	v2 = 180;
	AM_InsertEntry(p,(void *)&v1,(void*)&v2);
	//exit(1);
	AM_Print(p);
	exit(1);
	i = AM_OpenIndexScan(p,2,(void *)&v1);
	int * t;
	while((t = (int *)AM_FindNextEntry(i))){
		printf("EntryValue: %d\n",*t);
	}*/
	/*AM_OpenIndexScan(p,2,(void *)&v1);
	AM_OpenIndexScan(p,3,(void *)&v1);
	AM_OpenIndexScan(p,4,(void *)&v1);
	AM_OpenIndexScan(p,5,(void *)&v1);
	AM_OpenIndexScan(p,6,(void *)&v1);*/


	AM_Print(p);
	printf("END\n");
	/*if(AM_CloseIndex(p) != AME_OK){
		fprintf(stderr,"%s\n","CLOSE_INDEX");
		exit(1);
	}*/
	//AM_DestroyIndex(name);
	//AM_Close();
	return 0;
}