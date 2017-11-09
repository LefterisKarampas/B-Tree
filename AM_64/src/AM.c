#include "AM.h"
#include "bf.h"
#include "Global_Struct.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

int AM_errno = AME_OK;

open_files **Open_Files = NULL;
scan_files **Scan_Files = NULL;


/*Initialize the global structures and BF*/
void AM_Init() {
  BF_Init(LRU);                                                              //Initialize the BF
  int i;
  Open_Files = (open_files **)malloc(sizeof(open_files *)*MAXOPENFILES);     //Create array of pointers to open_files
  for(i=0;i<MAXOPENFILES;i++){
    Open_Files[i] = NULL;
  }
  Scan_Files = (scan_files **)malloc(sizeof(scan_files *)*MAXSCANS);         //Create array of pointers to scan_files
  for(i=0;i<MAXSCANS;i++){
    Scan_Files[i] = NULL;                                                    //Initialize each pointert to NULL
  }
	return;
}


/*Create a new file based on B+ Tree and initialize the first block with the metadata */
int AM_CreateIndex(char *fileName, 
	               char attrType1, 
	               int attrLength1, 
	               char attrType2, 
	               int attrLength2) {
  int fd;
  BF_Block *block;
  BF_Block_Init(&block);                                                          //Initialize Block
  if((AM_errno = BF_CreateFile(fileName)) != BF_OK){                                 //Create a new Block File
    BF_Block_Destroy(&block);
    return AM_errno;                                                              //Return error
  }

  if((AM_errno = BF_OpenFile(fileName, &fd)) != BF_OK){                              //Open the file
    BF_Block_Destroy(&block);
    return AM_errno;                                                                //Return error
  }

  if((AM_errno = BF_AllocateBlock(fd, block)) != BF_OK){                            //Allocate a new block
    BF_CloseFile(fd);
    BF_Block_Destroy(&block);
    return AM_errno;                                                                //Return error
  } 
  char* data;
  data = BF_Block_GetData(block);
  int m = 0;
  memcpy(data, "B+", sizeof(char)*3);                                               //Initialize the new file as B+ file
  m+=3;
  data[3] = attrType1;                                                              //Store the type of key  
  m += 1;
  memcpy(data+m,(char *)&attrLength1,sizeof(int));                                   //Store the length of key 
  m+=sizeof(int);
  data[m] = attrType2;                                                              //Store the type second field
  m+=1;
  memcpy(data+m,(char *)&attrType2,sizeof(int));                                     //Store the length of second field 
  m+=sizeof(int);           
  memcpy(data+m,"-1",sizeof(int));                                                  //Set root as not exists
  BF_Block_SetDirty(block);                                                         //Set block Dirty
  BF_UnpinBlock(block);                                                             //Unpin Block
  BF_CloseFile(fd);                                                                 //Close file
  BF_Block_Destroy(&block);
  return AME_OK;
}


int AM_DestroyIndex(char *fileName) {

  int i;
  for (i=0;i<MAXOPENFILES;i++)		
  {
	if (Open_Files[i]!=NULL)
	{
		if (strcmp(fileName,Open_Files[i]->filename))	//check for filename
			continue;
		return AM_errno;				//file open - Error
	}
  }
  unlink(fileName);						//delete file	
  return AME_OK;
}


int AM_OpenIndex (char *fileName) {
  int fd;
  char *data;
  BF_Block *block;
  BF_Block_Init(&block);
  if((AM_errno = BF_OpenFile(fileName, &fd)) != BF_OK)
  {
  	BF_Block_Destroy(&block);
	return AM_errno;
  }
  BF_GetBlock(fd,0,block);
  data = BF_Block_GetData(block);
  if (strcmp(data,"B+"))
  {
	BF_UnpinBlock(block);
	BF_Block_Destroy(&block);
	return AM_errno;
  }
  int i;
  for (i=0;i<MAXOPENFILES;i++)
  {
	if (Open_Files[i] == NULL)	//find empty cell ,create struct and fill it
	{
		int m=3;
		Open_Files[i] = malloc(sizeof(open_files));		
		Open_Files[i]->fd = fd;
		memcpy(&(Open_Files[i]->filename),&fileName,strlen(fileName)+1);
		memcpy(&(Open_Files[i]->attrType1),&data[m],sizeof(char));
		m+=1;
		memcpy(&(Open_Files[i]->attrLength1),&data[m],sizeof(int));
		m+=sizeof(int);
		memcpy(&(Open_Files[i]->attrType2),&data[m],sizeof(char));
		m+=1;
		memcpy(&(Open_Files[i]->attrLength2),&data[m],sizeof(int));
		m+=sizeof(int);
		memcpy(&(Open_Files[i]->root_number),&data[m],sizeof(int));
		BF_UnpinBlock(block);          
		BF_CloseFile(fd);                                                                 
  		BF_Block_Destroy(&block);
		return i;			//return cell's position
	} 	
  }
  return AME_OK;
}


int AM_CloseIndex (int fileDesc) {
  int i;
  for (i=0;i<MAXSCANS;i++)	
  {
	if (Scan_Files[i]!=NULL)
	{
		if (Scan_Files[i]->fd == fileDesc)
			return AM_errno;
	}
  }
  for (i=0;i<MAXOPENFILES;i++)		//pws diagrafw to arxeio ?
  {
	if (Open_Files[i]!=NULL)
	{
		if (Open_Files[i]->fd == fileDesc)
		{
			free(Open_Files[i]);
			Open_Files[i]=NULL;	//den xerw an xreiazetai auto
		}
	}
  }
  if((AM_errno = BF_CloseFile(fileDesc)) != BF_OK)
	return AM_errno;
  return AME_OK;
}


int AM_InsertEntry(int fileDesc, void *value1, void *value2) {
  return AME_OK;
}


int AM_OpenIndexScan(int fileDesc, int op, void *value) {
  return AME_OK;
}


void *AM_FindNextEntry(int scanDesc) {
	
}


int AM_CloseIndexScan(int scanDesc) {
  return AME_OK;
}


void AM_PrintError(char *errString) {
  fprintf(stderr,"%s",errString);
  switch(AM_errno){
    case 1:
      break;
    case 2:
      break;
    case 3:
      break;
    default:
      ;
  }
}

/*Destroy all the global structures and the BF*/
void AM_Close() {
  int i;
  if(Open_Files != NULL){
    for(i=0;i<MAXOPENFILES;i++){
      if(Open_Files[i] != NULL){
        free(Open_Files[i]->filename);
        free(Open_Files[i]);
      }
    }
    free(Open_Files);
  }
  if(Scan_Files != NULL){
    for(i=0;i<MAXSCANS;i++){
      if(Scan_Files[i] != NULL){
        free(Scan_Files[i]);
      }
    }
    free(Scan_Files);
  }
  BF_Close();
}
