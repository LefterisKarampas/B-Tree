#include "AM.h"
#include "bf.h"
#include "Global_Struct.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "List.h"

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
  memcpy(data+m,(char *)&attrType2,sizeof(int));                                     //Store the length of second field 
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
      m+=sizeof(int);
      memcpy(&(Open_Files[i]->attrType2),&data[m],sizeof(char));
      m+=1;
      memcpy(&(Open_Files[i]->attrLength2),&data[m],sizeof(int));
      m+=sizeof(int);
      memcpy(&(Open_Files[i]->root_number),&data[m],sizeof(int));
      BF_UnpinBlock(block);          
      BF_CloseFile(fd);                                                                 
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
        free(Open_Files[i]);
        Open_Files[i]=NULL;                                                     
  }

  if((AM_errno = BF_CloseFile(fileDesc)) != BF_OK)
   return AM_errno;
  return AME_OK;
}


int AM_InsertEntry(int fileDesc, void *value1, void *value2) {
  BF_Block *block;
  BF_Block_Init(&block);                                                            //Initialize Block
  if(Open_Files[fileDesc]->root_number == -1){
    Initialize_Root(fileDesc,value1,-1,-1);                                         //Create B+ Tree's root
  }
  int block_number;
  int prev = Open_Files[fd]->root_number;
  //Get the data block
  List * list = List_Create();                                                      //Create a list(Stack) for block's number
  while((block_number = traverse(fileDesc,prev,value1)) != -1){
    prev = block_number;
    List_Push(list,block_number);                                        //Store the blocks number, in case of splitting
  }
  prev = List_Pop(list);                                                            //Get the last visited block
  //Insert new entry and sort it 
  while(sort(fileDesc,prev,value1,value2) < 0){                                     //If there is not free space
    Split_Data * split_data = split(fileDesc,prev,value1);                          //SPLIT
    value2 = split_data->value;                                                     //value2 -> new pointer to index block 
    int x;
    x = List_Pop(list);
    if(x == -1){                                                                    //Split root
      Initialize_Root(fileDesc,split_data->value,prev,split_data->pointer);
      break;
    }
    prev = x;
  }
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
  int prev = Open_Files[fd]->root_number;
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





Split_Data * split(int fileDesc,int block_num,void *value1,void *value2){
  BF_Block *block;
  BF_Block_Init(&block);
  BF_Block *block2;
  BF_Block_Init(&block2);
  char* data;
  BF_GetBlock(fd,block_num,block);                                                  //Read block
  data = BF_Block_GetData(block);
  if((AM_errno = BF_AllocateBlock(Open_Files[fileDesc]->fd, block2)) != BF_OK){     //Allocate a new block
    BF_CloseFile(Open_Files[fileDesc]->fd);
    BF_Block_Destroy(&block2);
    return AM_errno;                                                                //Return error
  } 
  int new_block;
  BF_GetBlockCounter(Open_Files[fileDesc]->fd,&new_block);
  new_block -=1;
  char* data2;
  data2 = BF_Block_GetData(block2);
  data2[0] = data[0];
  int *counter = &data[1];
  //Split data block
  int mid,mod,size,m;
  int attrLength2;
  int attrLength1 = Open_Files[fileDesc]->attrLength1;

  //Split data block
  if(data[0] == 'd'){
    attrLength2 = Open_Files[fileDesc]->attrLength2;
    m = sizeof(char) + 3*sizeof(int);
    size = attrLength1 + attrLength2;
    mid = ((BF_BLOCK_SIZE-m)/size + 1)/2 + 0.5;                                     //Find the number of the entries that move to new block
    memcpy(&data[BF_BLOCK_SIZE-sizeof(int)],&new_block,sizeof(int));                //Update pointer to next
    memcpy(&data2[sizeof(char)+sizeof(int)],&block_num,sizeof(int));                //Update pointer to prev
  }
  //Split index block
  else{
    attrLength2 = sizeof(int);
    m = sizeof(char) + 2*sizeof(int);
    size = attrLength1 + attrLength2;
    mid = ((BF_BLOCK_SIZE-m)/size + 1.5)/2 + 0.5;                                   //Find the number of the entries that move to new block
  }
  mod = (*counter) +1 - mid;
  memcpy(&data2[1],&mid,sizeof(int));                                               //Update the counter to new block
  memcpy(&data[1],&mod,sizeof(int));                                                //Update the counter to old block
  int i;
  int j = mid-1;
  int flag = 0;
  //From the end until the mid, move the entries to new block from the end
  for(i = *counter-1;i<mid;i--){
    //If new value is less than current, move the new to other block
    if(!flag && compare(value,&data[m+j*size],Open_Files[fileDesc]->attrType1,attrLength1)){
      memcpy(&data2[m+j*size],value1,Open_Files[fileDesc]->attrLength1);                        //Copy new key value
      memcpy(&data2[m+j*size+attrLength1],value2,attrLength2);                                 
      flag = 1;
    }
    //If new value is bigger than current, move current to other block
    else{
      memcpy(&data2[m+j*size],&data[m+i*size],attrLength1);                                     //Copy current key value
      memcpy(&data2[m+j*size+attrLength1],&data[m+i*size+attrLength1],attrLength2);
    }
  }
  Split_Data * split_data = (Split_Data *)malloc(sizeof(Split_Data));       
  split_data->pointer = new_block;
  memcpy(split_data->value,&data2[m],attrLength1);
  BF_Block_SetDirty(block);                                                         //Set old block Dirty
  BF_UnpinBlock(block);                                                             //Unpin old Block
  BF_Block_Destroy(&block);
  BF_Block_SetDirty(block2);                                                         //Set new block Dirty
  BF_UnpinBlock(block2);                                                             //Unpin new Block
  BF_Block_Destroy(&block2);
  return split_data;                                                                 //Return value for split and new pointer
}





//Compare function for all possible types, return 0 if x1 < x2 else return 1
int compare(void * value1,void *value2,char attrType,int attrLength){
  if(attrType == 'c'){                                                    //If type is char *
    char x1[attrLength1];
    char x2[attrLength1];
    strncpy(x1,(char *)value1,attrLength);                                //Copy the string in temp_var
    x1[attrLength1-1] = '\0';
    strncpy(x2,(char *)value2,attrLength);                                //Copy the string in temp_var
    x2[attrLength1-1] = '\0';
    return (strcmp(x1,x2) < 0? 0:1);    //Return 0 if x1 < x2 else return 1
  }
  else{
    if(attrType == 'f'){                                                  //If type is float
      if((*(float *)value1) < *((float *)value2)){
        return 0;                                                         //Return 0 if x1 < x2
      }
    }
    else if(*((int *)value1) < *((int *)value2)){                         //If type is int
      return 0;                                                           //Return 0 if x1 <X2
    }
  }
  return 1;                                                               //Return 1 if x1>=x2
}





//Sort entries in the block and return 0 if new entry fits in block
//othewise return -1 for full block, can use it both data block and index block
int sort(int index,int block_num,void *value1,void *value2){
  BF_Block *block;
  BF_Block_Init(&block);
  char* data;
  BF_GetBlock(fd,block_num,block);                                        //Read block
  data = BF_Block_GetData(block);
  int attrLength1 = Open_Files[index]->attrLength1;                       //Hold attr1 length
  int attrLength2;                                                        //Hold attr2 length
  int * counter = (int *)&(data[1]);                                      //Get the counter of entries
  char id = data[0];
  if(id == 'd'){
    attrLength2 = Open_Files[index]->attrLength2;
    if(*counter >= ((BF_BLOCK_SIZE - 3*sizeof(int)+sizeof(char))/(attrLength1+attrLength2)))
      return -1;
  }
  else{
    attrLength2 = sizeof(int);
    if(*counter >= ((BF_BLOCK_SIZE - 2*sizeof(int)+sizeof(char))/(attrLength1+sizeof(int))))
      return -1;
  }
  int i;
  int m = sizeof(char)+2*sizeof(int);                                       
  int flag = 0;
  //From the last entry to the first of block, find the position to insert the new entry
  for(i=*counter-1;i>=0;i++){
    void * temp;
    temp = &data[m+(attrLength1+attrLength2)*i];
    //Compare the new entry with the current, if new < current then shift right the current
    if(!compare(value1,temp,Open_Files[index]->attrType1,attrLength1)){ 
      memcpy(&data[m+(attrLength1+attrLength2)*(i+1)],(char *)temp,attrLength1);
      temp = &data[m+(attrLength1+attrLength2)*i+attrLength1];
      memcpy(&data[m+(attrLength1+attrLength2)*(i+1)+attrLength1],(char *)temp,attrLength2);
    }
    //Else insert the new entry, right of the current
    else{
      memcpy(&data[m+(attrLength1+attrLength2)*(i+1)],(char *)value1,attrLength1);
      memcpy(&data[m+(attrLength1+attrLength2)*(i+1)+attrLength1],(char *)value2,attrLength2);
      flag = 1;
      break;
    }
  }
  //If new entry is the min key then insert it as first
  if(flag = 0){
    memcpy(&data[m],(char *)value1,attrLength1);
    memcpy(&data[m+attrLength1],(char *)value2,attrLength2);
  }
  *counter = *(counter) + 1;
  memcpy(&data[1],counter,sizeof(int));                                             //Increase by 1 the counter
  BF_Block_SetDirty(block);                                                         //Set block Dirty
  BF_UnpinBlock(block);                                                             //Unpin Block
  BF_Block_Destroy(block);
  return 0;
}


//Create and Initialize the root of B-Tree,
//Update the metadata with the new root and the OpenFiles structure
int Initialize_Root(int fileDesc,void * value1,int p0,int p1){
  BF_Block *block;
  BF_Block_Init(&block);                                                          //Initialize Block
  if((AM_errno = BF_AllocateBlock(Open_Files[fileDesc]->fd, block)) != BF_OK){    //Allocate a new block
    BF_Block_Destroy(&block);
    return AM_errno;                                                              //Return error
  }
  int block_num;
  BF_GetBlockCounter(Open_Files[fileDesc]->fd,&block_num)
  block_num -= 1;
  Open_Files[fileDesc]->root_number = block_num;                                  //Update root from Open files structure
  data = BF_Block_GetData(block);                                                 //Read block
  data[0] = 'i';                                                                  //Initialize it as index block
  int k = 1;
  int m = 1;
  memcpy(&data[m],&k,sizeof(int));                                                //Update counter of index block 
  m += sizeof(int);
  memcpy(&data[m],&p0,sizeof(int));                                                //Update pointer to NULL
  m+=sizeof(int);
  memcpy(&data[m],value1,Open_Files[fileDesc]->attrLength1);                      //Insert entry to root
  m +=  Open_Files[fileDesc]->attrLength1;
  memcpy(&data[m],&p1,sizeof(int));
  BF_Block_SetDirty(block);
  BF_UnpinBlock(block);
  //Update the metadata block with the new root block
  BF_GetBlock(Open_Files[fileDesc]->fd,0,block);                                  //Read block
  data = BF_Block_GetData(block); 
  memcpy(&data[13],&block_num,sizeof(int));                                       //Update root tree
  BF_Block_SetDirty(block);
  BF_UnpinBlock(block);
  BF_Block_Destroy(&block);
}
