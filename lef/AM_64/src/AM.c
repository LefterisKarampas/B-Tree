#include "AM.h"
#include "bf.h"
#include "Global_Struct.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "List.h"
#include "B_Tree.h"

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
}


/*Create a new file based on B+ Tree and initialize the first block with the metadata */
int AM_CreateIndex(char *fileName, 
                 char attrType1, 
                 int attrLength1, 
                 char attrType2, 
                 int attrLength2) {
  //Check for correct attrType1 and attrLegth1
  if(attrType1 == 'i' && attrLength1 != sizeof(int)){
    AM_errno = 2;
    return AM_errno;
  }
  else if(attrType1 == 'f' && attrLength1 != sizeof(float)){
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
  if(attrType2 == 'i' && attrLength2 != sizeof(int)){
    AM_errno = 2;
    return AM_errno;
  }
  else if(attrType2 == 'f' && attrLength2 != sizeof(float)){
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
  //Initialize Block
  BF_Block_Init(&block);

  //Create a new Block File
  if((AM_errno = BF_CreateFile(fileName)) != BF_OK){
    BF_Block_Destroy(&block);
    //Return error
    AM_errno = 3;
    return AM_errno;
  }

  //Open the file
  if((AM_errno = BF_OpenFile(fileName, &fd)) != BF_OK){
    BF_Block_Destroy(&block);
    //Return error
    AM_errno = 4;
    return AM_errno;  
  }

  //Allocate a new block
  if((AM_errno = BF_AllocateBlock(fd, block)) != BF_OK){
    BF_CloseFile(fd);
    BF_Block_Destroy(&block);
    //Return error
    AM_errno = 5;
    return AM_errno;
  } 

  //Initialize the new file as B+ file
  char* data;
  data = BF_Block_GetData(block);
  memset(data,0,BF_BLOCK_SIZE);
  int m = 0;
  memcpy(data, "B+", sizeof(char)*(strlen("B+")+1));
  m+= strlen("B+")+1;
  data[m] = attrType1;        //Store the type of key  
  m += 1;
  memcpy(data+m,(char *)&attrLength1,sizeof(int));    //Store the length of key 
  m+=sizeof(int);
  data[m] = attrType2;       //Store the type second field
  m+=1;
  memcpy(data+m,(char *)&attrLength2,sizeof(int));    //Store the length of second field 
  m+=sizeof(int);        
  int root_num = -1;                     //default root number( -1 == NULL)
  memcpy(data+m,&root_num,sizeof(int));  //Set root as not exists
  BF_Block_SetDirty(block);              //Set block Dirty
  BF_UnpinBlock(block);                  //Unpin Block
  BF_CloseFile(fd);                      //Close file
  BF_Block_Destroy(&block);
  return AME_OK;

}




int AM_DestroyIndex(char *fileName) {
  int i;
  //For each position in Open Files
  for (i=0;i<MAXOPENFILES;i++)    
  {
    //check if the position is not NULL
    if (Open_Files[i]!=NULL)
    {
      //check for filename
      if (strcmp(fileName,Open_Files[i]->filename)){
        continue;
      }
      else{
        AM_errno = 6;
        return 6;  //file open - Error
      }
    }
  }
  unlink(fileName);     //delete file  
  return AME_OK;
}




int AM_OpenIndex (char *fileName) {
  int fd;
  char *data;
  BF_Block *block;
  BF_Block_Init(&block);

  //Open the file
  if((AM_errno = BF_OpenFile(fileName, &fd)) != BF_OK)
  {
    BF_Block_Destroy(&block);
    AM_errno = 4;
    return AM_errno;
  }

  //Get the first block with the metadata info
  BF_GetBlock(fd,0,block);
  data = BF_Block_GetData(block);

  //If block is not a B+ Tree file, return error
  if (strcmp(data,"B+"))
  {
    BF_UnpinBlock(block);
    BF_Block_Destroy(&block);
    AM_errno = 7;
    return AM_errno;
  }

  int i;
  for (i=0;i<MAXOPENFILES;i++)
  {
    if (Open_Files[i] == NULL)       //find empty cell ,create struct and fill it
    {
      int m = strlen("B+")+1;
      Open_Files[i] = malloc(sizeof(open_files));   
      Open_Files[i]->fd = fd;
      //Store the filename
      Open_Files[i]->filename = (char *)malloc(sizeof(char)*(strlen(fileName)+1));
      strcpy(Open_Files[i]->filename,fileName);
      //Get the attrType1 
      memcpy(&(Open_Files[i]->attrType1),&data[m],sizeof(char));
      m+=sizeof(char);
      //Get the attrLength1
      memcpy(&(Open_Files[i]->attrLength1),&data[m],sizeof(int));
      m+=sizeof(int);
      //Get the attrType2
      memcpy(&(Open_Files[i]->attrType2),&data[m],sizeof(char));
      m+=sizeof(char);
      //Get the attrLength2
      memcpy(&(Open_Files[i]->attrLength2),&data[m],sizeof(int));
      m+=sizeof(int);
      //Store the B+ Tree's root (block number)
      memcpy(&(Open_Files[i]->root_number),&data[m],sizeof(int));
      BF_UnpinBlock(block);                                                                       
      BF_Block_Destroy(&block);
      return i;                 //return cell's position
    }   
  }
  //No empty position to open a file
  AM_errno = 8;
  BF_UnpinBlock(block);                                                                       
  BF_Block_Destroy(&block);
  return -1;
}



int AM_CloseIndex (int fileDesc) {
  int i;
  //Search for active scans 
  for (i=0;i<MAXSCANS;i++)
  {
    if (Scan_Files[i]!=NULL)
    {
      //If exists
      if (Scan_Files[i]->fd == fileDesc)
        AM_errno = 9;
        return AM_errno;   //Return error
    }
  }
  //If exists in open_files array, close file and delete index in array
  if(Open_Files[fileDesc] != NULL){
    if((AM_errno = BF_CloseFile(Open_Files[fileDesc]->fd)) != BF_OK){
      AM_errno = 10;
      return AM_errno;
    }
    free(Open_Files[fileDesc]->filename);
    free(Open_Files[fileDesc]);
    Open_Files[fileDesc]=NULL;
  }
  //Otherwise return error
  else{
    AM_errno = 11;
    return AM_errno;
  }
  return AME_OK;
}




int AM_InsertEntry(int fileDesc, void *value1, void *value2) {
  BF_Block *block;
  //Initialize Block
  BF_Block_Init(&block);
  //If root not exists 
  if(Open_Files[fileDesc]->root_number == -1){
    //Create B+ Tree's root
    Initialize_Root(fileDesc,value1,-1,-1);
  }
  //Else find the data block for insert the new value
  int block_number;
  int prev = Open_Files[fileDesc]->root_number;
  List * list = List_Create();  //Create a list(Stack) for block's number
  List_Push(list,prev);
  int op = -1;
  while((block_number = traverse(fileDesc,prev,value1,&op)) != -1){
    prev = block_number;
    List_Push(list,prev);  //Store the blocks number, in case of splitting
  }
  block_number = List_Pop(list);    //Get the last visited block
  BF_GetBlock(Open_Files[fileDesc]->fd,block_number,block);                    //Read block
  char * data = BF_Block_GetData(block);                               //If there is not free space

  int x;
  //Insert new entry and sort it
  while(sort(fileDesc,data,block,block_number,value1,value2) < 0){
    //Split block
    Split_Data * split_data = malloc(sizeof(Split_Data));
    split_data->pointer = malloc(sizeof(int));
    if(Open_Files[fileDesc]->attrType1 == 'i'){
      split_data->value = (int *)malloc(Open_Files[fileDesc]->attrLength1);
    }else if(Open_Files[fileDesc]->attrType1 == 'f'){
      split_data->value = (float *)malloc(Open_Files[fileDesc]->attrLength1);
    }
    else{
       split_data->value = (char *)malloc(Open_Files[fileDesc]->attrLength1);
    }
    split(fileDesc,block,data,block_number,value1,value2,&split_data);
    //value1 -> new value for insertion in index block
    memcpy(value1,split_data->value,Open_Files[fileDesc]->attrLength1);
    //value2 -> new pointer to index block 
    memcpy(value2,split_data->pointer,sizeof(int));
    //Get the parent block
    x = List_Pop(list);
    if(x == -1){
      //We have to Split root
      Initialize_Root(fileDesc,split_data->value,block_number,*split_data->pointer);
      break;
    }
    block_number = x;
    BF_UnpinBlock(block);
    BF_GetBlock(Open_Files[fileDesc]->fd,block_number,block);   //Read block
    data = BF_Block_GetData(block);
    free(split_data->pointer); 
    free(split_data->value);                                                  
    free(split_data);
  }
  List_Destroy(list);
  free(list);
  BF_UnpinBlock(block);      //Unpin Block
  BF_Block_Destroy(&block);
  return AME_OK;
}


//Find empty position to insert new scan in Scan global structure
int AM_OpenIndexScan(int fileDesc, int op, void *value) {
  //If scan structure is not initialized 
  if(Scan_Files == NULL){
    AM_errno = 2;
    return AM_errno; //Return error
  }
  int i;
  //Search for empty position
  for(i=0;i<MAXSCANS;i++){
    //If find exit from loop  
    if(Scan_Files[i] == NULL){
      break;                                               
    }
  }
  //If we don't find any empty position
  if(i == MAXSCANS){                                     
    AM_errno = 2;
    return AM_errno;  //Return error
  } 

  int block_number;
  int temp_op = op;
  int prev = Open_Files[fileDesc]->root_number;
  int id_block;

  while((block_number = Find_Scan(fileDesc,prev,value,&id_block,&temp_op)) != -1){
    prev = block_number;
  }
  //Find scan returns the id_block which we have to go 
  //for getting Entry and the record_number in block (temp_op)
  
  //Get scan record number miss
  Scan_Files[i] = (scan_files *)malloc(sizeof(scan_files));
  Scan_Files[i]->fd = fileDesc;
  //If record_number in block is NULL,there is not any entry
  //then set the block_id with NULL 
  if(temp_op == -1){
    memcpy(&(Scan_Files[i]->id_block),&temp_op,sizeof(int));
  }
  else{
    memcpy(&(Scan_Files[i]->id_block),&id_block,sizeof(int));
  }
  memcpy(&(Scan_Files[i]->record_number),&temp_op,sizeof(int));
  memcpy(&(Scan_Files[i]->op),&op,sizeof(int));

  if(Open_Files[fileDesc]->attrType1 == 'i'){
    Scan_Files[i]->value = (int *)malloc(Open_Files[fileDesc]->attrLength1);
  }else if(Open_Files[fileDesc]->attrType1 == 'f'){
    Scan_Files[i]->value = (float *)malloc(Open_Files[fileDesc]->attrLength1);
  }
  else{
     Scan_Files[i]->value = (char *)malloc(sizeof(char)*Open_Files[fileDesc]->attrLength1);
  }
  memcpy(Scan_Files[i]->value,value,Open_Files[fileDesc]->attrLength1);
  return i;
}





void *AM_FindNextEntry(int scanDesc) {
  if(Scan_Files[scanDesc]->id_block == -1)             //ERROR
  {
    AM_errno = AME_EOF; 
    return NULL;
  }
  BF_Block *block;
  BF_Block_Init(&block); 
  BF_GetBlock(Open_Files[Scan_Files[scanDesc]->fd]->fd,Scan_Files[scanDesc]->id_block,block);
  char *data;
  data = BF_Block_GetData(block);
  int pos = Scan_Files[scanDesc]->record_number;
  int m=sizeof(char)+2*sizeof(int);
  int size;
  int attrLength1 = Open_Files[Scan_Files[scanDesc]->fd]->attrLength1; 
  int attrLength2 = Open_Files[Scan_Files[scanDesc]->fd]->attrLength2;
  size = attrLength1 + attrLength2;
  int counter,new_block;
  memcpy(&counter, &data[sizeof(char)], sizeof(int));
  void *value1 ;
  if(Open_Files[Scan_Files[scanDesc]->fd]->attrType2 == 'i'){
    value1 = (int *)malloc(sizeof(int));
  }
  else if(Open_Files[Scan_Files[scanDesc]->fd]->attrType2 == 'f'){
   value1 = (float *)malloc(sizeof(float));
  }
  else{
   value1 = (char *)malloc(sizeof(char)*Open_Files[Scan_Files[scanDesc]->fd]->attrLength2);
  } 
  if(value1 == NULL){
    fprintf(stderr, "Malloc error\n");
    exit(1);
  }
  memcpy(value1,&data[m+size*pos+attrLength1],attrLength2);
  int flag =0 ;
  pos++;
  while(pos<counter)
  {
    flag = 0;
    int result;
    switch(Scan_Files[scanDesc]->op){
      case 1:{
        result = op_function(Scan_Files[scanDesc]->value,&data[m+size*pos], Open_Files[Scan_Files[scanDesc]->fd]->attrType1,attrLength1, Scan_Files[scanDesc]->op);
        if(result){
          Scan_Files[scanDesc]->id_block = -1;
        }
        flag = 1;
        break;
      }
      case 2:{
        result = op_function(Scan_Files[scanDesc]->value,&data[m+size*pos], Open_Files[Scan_Files[scanDesc]->fd]->attrType1,attrLength1, Scan_Files[scanDesc]->op);
        if(!result){
          flag = 1;
        }
        break;
      }
      case 3:{
        result = op_function(Scan_Files[scanDesc]->value,&data[m+size*pos], Open_Files[Scan_Files[scanDesc]->fd]->attrType1,attrLength1, 4);
        if(result){
          Scan_Files[scanDesc]->id_block = -1;
        }
        flag = 1;
        break;
      }
      case 4:
      case 6:{
        flag =1;
        break;
      }
      case 5:{
        result = op_function(Scan_Files[scanDesc]->value,&data[m+size*pos], Open_Files[Scan_Files[scanDesc]->fd]->attrType1,attrLength1, 6);
        if(result){
          Scan_Files[scanDesc]->id_block = -1;
        }
        flag = 1;
        break;
      }
    }
    if(flag == 1){
      break;
    }
    pos++;
  }
  if(flag == 0)                           
  {
    memcpy(&new_block,&data[BF_BLOCK_SIZE-sizeof(int)],sizeof(int));
    if (new_block!= -1)
    {
      Scan_Files[scanDesc]->id_block = new_block;
      Scan_Files[scanDesc]->record_number = 0;
    }
    else
    {
      Scan_Files[scanDesc]->id_block = -1;
    }
  }
  else{
    Scan_Files[scanDesc]->record_number = pos;
  }
  BF_UnpinBlock(block);
  BF_Block_Destroy(&block);
  return value1;
}





int AM_CloseIndexScan(int scanDesc) {
  if(Scan_Files == NULL){
    AM_errno = 12;
    return AM_errno;
  }
  if(Scan_Files[scanDesc] != NULL){
    free(Scan_Files[scanDesc]->value);   //Delete the value from struct
    free(Scan_Files[scanDesc]);          //Delete the struct
    Scan_Files[scanDesc] = NULL;
  }
  else{
    AM_errno = 12;
    return AM_errno;
  }
  return AME_OK;
}




void AM_PrintError(char *errString) {
  fprintf(stderr,"%s",errString);
  switch(AM_errno){
    case 2:{
      fprintf(stderr,"Error: AM_CreateIndex, wrong AttrType and AttrLength\n");
      break;
    }
    case 3:{
      fprintf(stderr, "Error: Failed on BF_CreateFile\n");
      break;
    }
    case 4:{
      fprintf(stderr,"Error: Failed on BF_OpenFile\n");
      break;
    }
    case 5:{
      fprintf(stderr,"Error: Failed on BF_AllocateBlock\n");
      break;
    }
    case 6:{
      fprintf(stderr,"Error: Failed on AM_DestroyIndex, the file which we tried to remove it, is still Open/Active\n");
      break;
    }
    case 7:{
      fprintf(stderr,"Error: Failed on AM_OpenIndex, the file is not a B+ Tree File\n");
      break;
    }
    case 8:{
      fprintf(stderr,"Error: Failed on AM_OpenIndex, Array with Active files is full\n");
      break;
    }
    case 9:{
      fprintf(stderr,"Error: Failed on AM_CloseIndex, the file has active scans\n");
      break;
    }
    case 10:{
      fprintf(stderr,"Error: Failed on BF_Close\n");
      break;
    }
    case 11:{
      fprintf(stderr,"Error: Failed on AM_CloseIndex, the file is not open\n");
      break;
    }
    case 12:{
      fprintf(stderr,"Error: Failed on AM_CloseIndexScan,there is not an active scan\n");
      break;
    }
    case 13:{
      //fprintf(stderr,"Error: Failed on AM_CloseIndex, the file is not open\n");
      break;
    }
    case 14:{
      // fprintf(stderr,"Error: Failed on AM_CloseIndex, the file is not open\n");
      break;
    }
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
  BF_Block_Init(&block);  
  char *data;
  int i;
  int *counter = malloc(sizeof(int));                                          //Initialize Block
  int prev = Open_Files[fileDesc]->root_number;
  int size = Open_Files[fileDesc]->attrLength1 + Open_Files[fileDesc]->attrLength2;
  int attrLength2 = Open_Files[fileDesc]->attrLength2;
  BF_GetBlock(Open_Files[fileDesc]->fd,prev,block);
  data = BF_Block_GetData(block);
  printf("Root %d:\n",prev);
  memcpy(counter,&(data[1]),sizeof(int));
  if(Open_Files[fileDesc]->attrType1 == 'i'){
    int temp;
    for(i=0;i<*counter;i++){
      if(i == 0){
        memcpy(&temp,&(data[sizeof(char)+sizeof(int)]),sizeof(int));
        printf("\t%d\n",temp);
      }
      memcpy(&temp,&(data[sizeof(char)+2*sizeof(int)+(Open_Files[fileDesc]->attrLength1 +sizeof(int))*i]),Open_Files[fileDesc]->attrLength1);
      printf("%d. %d -> ",i,temp);
      memcpy(&temp,&(data[sizeof(char)+2*sizeof(int)+(Open_Files[fileDesc]->attrLength1 +sizeof(int))*i+Open_Files[fileDesc]->attrLength1]),sizeof(int));
      printf("%d\n",temp);
    }
  }
  else if(Open_Files[fileDesc]->attrType1 == 'c'){
    char temp1[Open_Files[fileDesc]->attrLength1];
    int temp;
    for(i=0;i<*counter;i++){
      if(i == 0){
        memcpy(&temp,&(data[sizeof(char)+sizeof(int)]),sizeof(int));
        printf("\t%d\n",temp);
      }
      memcpy(temp1,&(data[sizeof(char)+2*sizeof(int)+(Open_Files[fileDesc]->attrLength1 +sizeof(int))*i]),Open_Files[fileDesc]->attrLength1);
      temp1[50] = '\0';
      printf("%d. %s ",i,temp1);
      memcpy(&temp,&(data[sizeof(char)+2*sizeof(int)+(Open_Files[fileDesc]->attrLength1 +sizeof(int))*i+Open_Files[fileDesc]->attrLength1]),sizeof(int));
      printf("%d\n",temp);
    }
  }
  BF_UnpinBlock(block);
  //exit(1);
  int block_number;
  int op = -1;
  while((block_number = traverse(fileDesc,prev,NULL,&op)) != -1){
    prev = block_number;                                          
  }
  int m = 2*sizeof(int)+sizeof(char);
  while(prev != -1){
    BF_GetBlock(Open_Files[fileDesc]->fd,prev,block);
    data = BF_Block_GetData(block);
    printf("Block %d:\n",prev);
    memcpy(counter,&(data[1]),sizeof(int));
    if(Open_Files[fileDesc]->attrType1 == 'i'){
      for(i=0;i<*counter;i++){
        int temp;
        memcpy(&temp,&(data[sizeof(char)+2*sizeof(int)+2*sizeof(int)*i]),sizeof(int));
        printf("\t%d. %d - ",i,temp);
        memcpy(&temp,&(data[sizeof(char)+2*sizeof(int)+2*sizeof(int)*i+sizeof(int)]),sizeof(int));
        printf("%d\n",temp);
      }
    }
    else if(Open_Files[fileDesc]->attrType1 == 'c'){
      for(i=0;i<*counter;i++){
        int temp;
        char temp1[Open_Files[fileDesc]->attrLength1];
        strncpy(temp1,&(data[m+size*i]),Open_Files[fileDesc]->attrLength1);
        temp1[Open_Files[fileDesc]->attrLength1-1] = '\0';
        printf("\t%d. %s - ",i,temp1);
        memcpy(&temp,&(data[m+size*i+Open_Files[fileDesc]->attrLength1]),sizeof(int));
        printf("%d\n",temp);
      }
    }
    memcpy(&prev,&(data[BF_BLOCK_SIZE-sizeof(int)]),sizeof(int));
    printf("%d\n",prev);
    BF_UnpinBlock(block);
  }
  free(counter);
  BF_Block_Destroy(&block);
}
