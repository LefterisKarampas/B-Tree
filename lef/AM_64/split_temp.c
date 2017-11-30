Split_Data * split(int fileDesc,BF_Block * block,char *data,int block_num,void *value1,void *value2){
//   printf("IN\n");
//   BF_Block *block2;
//   BF_Block_Init(&block2);
//   //Allocate a new block
//   if((AM_errno = BF_AllocateBlock(Open_Files[fileDesc]->fd, block2)) != BF_OK){
//     BF_CloseFile(Open_Files[fileDesc]->fd);
//     BF_Block_Destroy(&block2);
//     AM_errno = 2;
//     return NULL;   //Return error
//   } 


//   int new_block;
//   BF_GetBlockCounter(Open_Files[fileDesc]->fd,&new_block);
//   new_block -=1;
//   char* data2;
//   data2 = BF_Block_GetData(block2);
//   data2[0] = data[0];
//   int *counter = (int *)&data[1];
//   //Split data block
//   int mid,mod,size,m;
//   int temp_next_pointer = -1;  //Pointer to next before split
//   int attrLength2;
//   int attrLength1 = Open_Files[fileDesc]->attrLength1;

//   mid = ((*counter)+1)/2 ;         //Find the number of the entries that move to new block
//   if((((*counter)+1) % 2 != 0)){
//     mid+=1;
//   }


//   m = sizeof(char) + 2*sizeof(int);
//   //Split data block
//   if(data[0] == 'd'){
//     attrLength2 = Open_Files[fileDesc]->attrLength2;
//     //Read the pointer to next before split
//     memcpy(&temp_next_pointer,&data[BF_BLOCK_SIZE-sizeof(int)],sizeof(int));
//     //Update pointer to next
//     memcpy(&data[BF_BLOCK_SIZE-sizeof(int)],&new_block,sizeof(int));   
//     //Update pointer to prev      
//     memcpy(&data2[sizeof(char)+sizeof(int)],&block_num,sizeof(int));        
//     //Update pointer to prev
//     memcpy(&data2[BF_BLOCK_SIZE-sizeof(int)],&temp_next_pointer,sizeof(int)); 
//   }
//   //Split index block
//   else{
//     attrLength2 = sizeof(int);
//   }


//   size = attrLength1 + attrLength2;
//   mod = (*counter)+1 - mid;
//   int i = (*counter)-1;
//   int j = mid -1;
//   int flag = 0;
//   int null_pointer = -1;

//   //From the end until the mid, move the entries to new block from the end
//   if(data[0] == 'd'){
//     while(j>=0){
//       //If new value is greater than current, move the new to other block
//       if(!flag && compare(value1,&(data[m+i*size]),Open_Files[fileDesc]->attrType1,attrLength1)){
//         //Copy new key value
//         memcpy(&data2[m+j*size],value1,Open_Files[fileDesc]->attrLength1);
//         memcpy(&data2[m+j*size+attrLength1],value2,attrLength2);                                 
//         flag = 1;
//       }
//       //If new value is less than current, move current to other block
//       else{
//         //Copy current key value
//         memcpy(&data2[m+j*size],&(data[m+i*size]),attrLength1);
//         memcpy(&data2[m+j*size+attrLength1],&data[m+i*size+attrLength1],attrLength2);
//         i--;
//       }
//       j--;
//     }
//     memcpy(&data2[1],&mid,sizeof(int));    //Update the counter to new block
//     memcpy(&data[1],&mod,sizeof(int));     //Update the counter to old block
//   }
//   else{
//     if(mid > mod){
//       mid--;
//     }
//     for(i = (*counter)-1;i>mid;){
//       //If new value is greater than current, move the new to other block
//       if(!flag && compare(value1,&(data[m+i*size]),Open_Files[fileDesc]->attrType1,attrLength1)){
//         //Copy new key value
//         memcpy(&data2[m+j*size],value1,Open_Files[fileDesc]->attrLength1);
//         memcpy(&data2[m+j*size+attrLength1],value2,attrLength2);                                 
//         flag = 1;
//       }
//       //If new value is less than current, move current to other block
//       else{
//         //Copy current key value
//         memcpy(&data2[m+j*size],&data[m+i*size],attrLength1);
//         memcpy(&data2[m+j*size+attrLength1],&data[m+i*size+attrLength1],attrLength2);
//         //Each key-pointer has moved to new block, set the old pointer to Null
//         memcpy(&data[m+i*size+attrLength1],&null_pointer,attrLength2);
//         i--;
//       }
//       j--;
//     }
//     memcpy(&data2[1],&mid,sizeof(int)); //Update the counter to new block
//     memcpy(&data[1],&mod,sizeof(int));  //Update the counter to old block
//   }

//   BF_Block_SetDirty(block);   

//   //If new value has not inserted yet (it belongs to old block), sort the old block
//   if(flag == 0){
//     sort(fileDesc,block_num,value1,value2);
//     memcpy(&data[sizeof(char)],&mod,sizeof(int));                          
//   }

//   Split_Data * split_data = (Split_Data *)malloc(sizeof(Split_Data));
//   if(Open_Files[fileDesc]->attrType1 == 'i'){
//     split_data->value = (int *)malloc(sizeof(int));
//   }
//   else if(Open_Files[fileDesc]->attrType1 == 'f'){
//    split_data->value = (float *)malloc(sizeof(float));
//   }
//   else{
//    split_data->value = (char *)malloc(sizeof(char)*Open_Files[fileDesc]->attrLength1);
//   }       

//   split_data->pointer = &new_block;
//   if(data[0] == 'd'){
//     memcpy(split_data->value,&(data2[sizeof(char)+2*sizeof(int)]),attrLength1);
//   }
//   else{
//     /*if(mid > mod){*/
//       //Get the key which has to go up
//       memcpy(split_data->value,&data[m+mod*size],attrLength1);
//       //Relocate the pointer
//       memcpy(&data2[sizeof(char)+sizeof(int)],&data[m+mod*size+attrLength1],sizeof(int));
//       memcpy(&data[m+mod*size+attrLength1],&null_pointer,attrLength2);
//     }
//     /*else{
//       memcpy(split_data->value,&data[m+(mod-1)*size],attrLength1);
//       //Relocate the pointer
//       memcpy(&data2[sizeof(char)+sizeof(int)],&data[m+(mod-1)*size+attrLength1],sizeof(int));
//       memcpy(&data[m+(mod-1)*size+attrLength1],&null_pointer,attrLength2);
//     }
//   }*/

//   BF_Block_SetDirty(block);   //Set old block Dirty
//   BF_Block_SetDirty(block2);  //Set new block Dirty
//   BF_UnpinBlock(block2);      //Unpin new Block
//   //Read block
//   if((data[0] == 'd') && (temp_next_pointer != -1)){
//     BF_GetBlock(Open_Files[fileDesc]->fd,temp_next_pointer,block2); 
//     data2 = BF_Block_GetData(block);
//     memcpy(&data2[sizeof(char)+sizeof(int)],&new_block,sizeof(int));
//     //Set new block Dirty
//     BF_Block_SetDirty(block2);
//     BF_UnpinBlock(block2); //Unpin new Block
//   }
//   BF_Block_Destroy(&block2);
//   return split_data;   //Return value for split and new pointer
//   printf("OUT\n");
// }