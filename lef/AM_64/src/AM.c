#include "AM.h"
#include "bf.h"
#include "Global_Struct.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "List.h"
#include "B_Tree.h"
static int loop = 0;
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
  //Check for correct attrType1 and attrLegth1
  if(attrType1 == 'i' && attrLength1 != 4){
    AM_errno = 2;
    return AM_errno;
  }
  else if(attrType1 == 'f' && attrLength1 != 4){
    AM_errno = 2;
    return AM_errno;
  }
  else if(attrType1 == 'c' && (attrLength1 < 1 || attrLength1 > 255)){
    AM_errno = 2;
    return AM_errno;
  }
  else if(attrType1 != 'c' && attrType1 != 'f' && attrType1 != 'i'){
    AM_errno = 2;
    return AM_errno;
  }

  //Check for correct attrType2 and attrLegth2
  if(attrType2 == 'i' && attrLength2 != 4){
    AM_errno = 2;
    return AM_errno;
  }
  else if(attrType2 == 'f' && attrLength2 != 4){
    AM_errno = 2;
    return AM_errno;
  }
  else if(attrType2 == 'c' && (attrLength2 < 1 || attrLength2 > 255)){
    AM_errno = 2;
    return AM_errno;
  }
  else if(attrType2 != 'c' && attrType2 != 'f' && attrType2 != 'i'){
    AM_errno = 2;
    return AM_errno;
  }

  int fd;
  BF_Block *block;
  BF_Block_Init(&block);                                                            //Initialize Block
  if((AM_errno = BF_CreateFile(fileName)) != BF_OK){                                //Create a new Block File
    BF_Block_Destroy(&block);
    return AM_errno;                                                                //Return error
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
  memcpy(data+m,(char *)&attrLength2,sizeof(int));                                     //Store the length of second field 
  m+=sizeof(int);        
  int root_num = -1;                                                          //default root number( -1 == NULL)
  memcpy(data+m,&root_num,sizeof(int));                                                  //Set root as not exists
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
      if (strcmp(fileName,Open_Files[i]->filename))                                   //check for filename
        continue;
      return AM_errno;                                                                //file open - Error
    }
  }
  unlink(fileName);                                                                  //delete file  
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
    if (Open_Files[i] == NULL)                                                      //find empty cell ,create struct and fill it
    {
      int m=3;
      Open_Files[i] = malloc(sizeof(open_files));   
      Open_Files[i]->fd = fd;
      memcpy(&(Open_Files[i]->filename),&fileName,strlen(fileName)+1);
      memcpy(&(Open_Files[i]->attrType1),&data[m],sizeof(char));
      m+=1;
      memcpy(&(Open_Files[i]->attrLength1),&data[m],sizeof(int));
      printf("%d\n",Open_Files[i]->attrLength1);
      m+=sizeof(int);
      memcpy(&(Open_Files[i]->attrType2),&data[m],sizeof(char));
      m+=1;
      memcpy(&(Open_Files[i]->attrLength2),&data[m],sizeof(int));
      printf("%d\n",Open_Files[i]->attrLength2);
      m+=sizeof(int);
      memcpy(&(Open_Files[i]->root_number),&data[m],sizeof(int));
      BF_UnpinBlock(block);                                                                       
      BF_Block_Destroy(&block);
      return i;                                                                     //return cell's position
    }   
  }
  return -1;
}



int AM_CloseIndex (int fileDesc) {
  int i;
  for (i=0;i<MAXSCANS;i++)                                                      //Search for active scans 
  {
    if (Scan_Files[i]!=NULL)
    {
      if (Scan_Files[i]->fd == fileDesc)                                        //If exists
        return AM_errno;                                                        //Return error
    }
  }

  for (i=0;i<MAXOPENFILES;i++)                                                  
  {
    if (Open_Files[i]!=NULL)
    {
      if (Open_Files[i]->fd == fileDesc)
      {
        break; 
      }   
    }                                                 
  }
  if(i<MAXOPENFILES){
    if((AM_errno = BF_CloseFile(Open_Files[i]->fd)) != BF_OK){
     return AM_errno;
    }
    free(Open_Files[i]);
    Open_Files[i]=NULL;
  }
  return AME_OK;
}


int AM_InsertEntry(int fileDesc, void *value1, void *value2) {
  BF_Block *block;
  BF_Block_Init(&block);                                                            //Initialize Block
  if(Open_Files[fileDesc]->root_number == -1){
    Initialize_Root(fileDesc,value1,-1,-1);                                         //Create B+ Tree's root
  }
  int block_number;
  int prev = Open_Files[fileDesc]->root_number;
  //Get the data block
  List * list = List_Create();                                                      //Create a list(Stack) for block's number
  loop++;
  while((block_number = traverse(fileDesc,prev,value1)) != -1){
    prev = block_number;
    List_Push(list,prev);                                                   //Store the blocks number, in case of splitting
  }
  prev = List_Pop(list);                                                            //Get the last visited block
  BF_GetBlock(Open_Files[fileDesc]->fd,prev,block);                                    //Read block
  char * data = BF_Block_GetData(block);
  //Insert new entry and sort it
  printf("Sort\n");
  while(sort(fileDesc,prev,value1,value2) < 0){                               //If there is not free space
    printf("Split %d\n",prev);
    Split_Data * split_data = split(fileDesc,block,data,prev,value1,value2);        //SPLIT
    value2 = split_data->pointer;                                                   //value2 -> new pointer to index block 
    value1 = split_data->value;                                                     //value1 -> new value for insertion in index block
    int x;
    x = List_Pop(list);
    if(x == -1){                                                                    //Split root
      Initialize_Root(fileDesc,split_data->value,prev,*((int*)split_data->pointer));
      free(split_data);
      break;
    }
    prev = x;
    BF_UnpinBlock(block);                                                             //Unpin Block
    BF_GetBlock(Open_Files[fileDesc]->fd,prev,block);                                    //Read block
    data = BF_Block_GetData(block);
    free(split_data);
  }
  List_Destroy(list);
  free(list);
  BF_UnpinBlock(block);                                                             //Unpin Block
  BF_Block_Destroy(&block);
  return AME_OK;
}


//Find empty position to insert new scan in Scan global structure
int AM_OpenIndexScan(int fileDesc, int op, void *value) {
  if(Scan_Files == NULL){                                  //If scan structure is not initialized 
    AM_errno = 2;
    return AM_errno;                                       //Return error
  }
  int i;
  for(i=0;i<MAXSCANS;i++){                                 //Search for empty position
    if(Scan_Files[i] == NULL){
      break;                                               //If find exit from loop
    }
  }
  if(i == MAXSCANS){                                       //If we don't find any empty position
    AM_errno = 2;
    return AM_errno;                                       //Return error
  } 
  scan_files * scan = (scan_files *)malloc(sizeof(scan_files));   //Allocate the scan info struct
  scan->fd = fileDesc;
  scan->op = op;
  scan->value = value;
  void *temp = NULL;
  //If we have op in {EQAUL,GREATER_THAN,GREATER_THAN_OR_EQUAL} find the data_block which
  //the value could exist
  if(op == EQUAL || op == GREATER_THAN || op == GREATER_THAN_OR_EQUAL){
    temp = value;
  }
  int block_number;
  int prev = Open_Files[fileDesc]->root_number;
  while((block_number = traverse(fileDesc,prev,temp)) != -1){
    prev = block_number;
  }
  scan->id_block = prev;
  //Get scan record number miss

  //Store scan info in scan structure
  Scan_Files[i] = scan;
  return i;
}






void *AM_FindNextEntry(int scanDesc) {
  ;
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

void AM_Print(int fileDesc){
  BF_Block *block;
  BF_Block_Init(&block);                                                            //Initialize Block
  int prev = Open_Files[fileDesc]->root_number;
  int block_number;
  while((block_number = traverse(fileDesc,prev,NULL)) != -1){
    prev = block_number;                                          
  }
  BF_GetBlock(Open_Files[fileDesc]->fd,prev,block);
  char *data = BF_Block_GetData(block);
  int i;
  int *counter = malloc(sizeof(int));
  printf("HERE\n");
  for(i=0;i<10;i++){
    int temp;
    memcpy(&temp,&(data[sizeof(char)+2*sizeof(int)]),sizeof(int));
    printf("%d < ",temp);
  }
}
