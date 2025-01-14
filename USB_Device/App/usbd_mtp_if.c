/**
  ******************************************************************************
  * @file    usbd_mtp_if.c
  * @author  MCD Application Team
  * @brief   Source file for USBD MTP file list_files.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usbd_mtp_if.h"

/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/*
static FILE MyFile;
static FATFS SDFatFs;
static char SDPath[4];
static FolderLevel Fold_Lvl;
static FOLD_INFTypeDef FoldStruct;
static FILE_INFTypeDef FileStruct;
static SD_Object_TypeDef sd_object;
*/
extern USBD_HandleTypeDef USBD_Device;

uint32_t idx[200];
uint32_t parent;
/* static char path[255]; */
uint32_t sc_buff[MTP_IF_SCRATCH_BUFF_SZE / 4U];
uint32_t sc_len = 0U;
uint32_t pckt_cnt = 1U;
uint32_t foldsize;


/* my storage */
#define MAX_OBJECTS 2
#define MAX_SIZE 1024*6
//object handles
MTP_ObjectInfoTypeDef object[MAX_OBJECTS];
//memory_block

uint8_t buffer[MAX_OBJECTS][MAX_SIZE];
uint8_t bufferLen[MAX_OBJECTS];

uint32_t emptyObjects =MAX_OBJECTS;// for empty memroy all objects are free
uint32_t emptySpace = MAX_OBJECTS*MAX_SIZE;
//array ID to handle



/*define text.txt file with hello */

uint8_t fileContent[] = "Hello";
#define FILE_CONTENT_LEN 6
uint16_t fileName[] = {0x74,0x65,0x78,0x74,0x2E,0x74,0x78,0x74,0};      //"text.txt"
#define FILE_NAME_LEN  9

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static uint8_t  USBD_MTP_Itf_Init(void);
static uint8_t  USBD_MTP_Itf_DeInit(void);
static uint32_t USBD_MTP_Itf_ReadData(uint32_t Param1, uint8_t *buff, MTP_DataLengthTypeDef *data_length);
static uint16_t USBD_MTP_Itf_Create_NewObject(MTP_ObjectInfoTypeDef ObjectInfo, uint32_t objhandle);

static uint32_t USBD_MTP_Itf_GetIdx(uint32_t Param3, uint32_t *obj_handle);
static uint32_t USBD_MTP_Itf_GetParentObject(uint32_t Param);
static uint16_t USBD_MTP_Itf_GetObjectFormat(uint32_t Param);
static uint8_t  USBD_MTP_Itf_GetObjectName_len(uint32_t Param);
static void     USBD_MTP_Itf_GetObjectName(uint32_t Param, uint8_t obj_len, uint16_t *buf);
static uint32_t USBD_MTP_Itf_GetObjectSize(uint32_t Param);
static uint64_t USBD_MTP_Itf_GetMaxCapability(void);
static uint64_t USBD_MTP_Itf_GetFreeSpaceInBytes(void);
static uint32_t USBD_MTP_Itf_GetNewIndex(uint16_t objformat);
static void     USBD_MTP_Itf_WriteData(uint16_t len, uint8_t *buff);
static uint32_t USBD_MTP_Itf_GetContainerLength(uint32_t Param1);
static uint16_t USBD_MTP_Itf_DeleteObject(uint32_t Param1);

static void     USBD_MTP_Itf_Cancel(uint32_t Phase);
/* static uint32_t USBD_MTP_Get_idx_to_delete(uint32_t Param, uint8_t *tab); */

USBD_MTP_ItfTypeDef USBD_MTP_fops =
{
  USBD_MTP_Itf_Init,
  USBD_MTP_Itf_DeInit,
  USBD_MTP_Itf_ReadData,
  USBD_MTP_Itf_Create_NewObject,
  USBD_MTP_Itf_GetIdx,
  USBD_MTP_Itf_GetParentObject,
  USBD_MTP_Itf_GetObjectFormat,
  USBD_MTP_Itf_GetObjectName_len,
  USBD_MTP_Itf_GetObjectName,
  USBD_MTP_Itf_GetObjectSize,
  USBD_MTP_Itf_GetMaxCapability,
  USBD_MTP_Itf_GetFreeSpaceInBytes,
  USBD_MTP_Itf_GetNewIndex,
  USBD_MTP_Itf_WriteData,
  USBD_MTP_Itf_GetContainerLength,
  USBD_MTP_Itf_DeleteObject,
  USBD_MTP_Itf_Cancel,
  sc_buff,
  MTP_IF_SCRATCH_BUFF_SZE,
};

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  USBD_MTP_Itf_Init
  *         Initialize the file system Layer
  * @param  None
  * @retval status value
  */
static uint8_t USBD_MTP_Itf_Init(void)
{

  /* add one object */
uint32_t newID = 0;
  object[newID].Storage_id=newID+1;
  object[newID].ObjectFormat=0;
  object[newID].ProtectionStatus=0;
  object[newID].ObjectCompressedSize=FILE_CONTENT_LEN;
  object[newID].ThumbFormat=0;
  object[newID].ThumbCompressedSize=0;
  object[newID].ThumbPixWidth=0;
  object[newID].ThumbPixHeight=0;
  object[newID].ImagePixWidth=0;
  object[newID].ImagePixHeight=0;
  object[newID].ImageBitDepth=0;
  object[newID].ParentObject=0x00000000;
  object[newID].AssociationType=0xC000;//MTP type file or folder?
  object[newID].AssociationDesc=0;
  object[newID].SequenceNumber=0;
  object[newID].Filename_len=FILE_NAME_LEN;
  object[newID].CaptureDate=0;
  object[newID].ModificationDate=0;
  object[newID].Keywords=0;
  emptySpace = emptySpace-MAX_SIZE; //decrease max size
  bufferLen[newID]=FILE_CONTENT_LEN;
  memcpy(buffer[newID],fileContent,FILE_CONTENT_LEN);
  memcpy(object[newID].Filename,fileName,FILE_NAME_LEN*2);
  return 0;
}

/**
  * @brief  USBD_MTP_Itf_DeInit
  *         Uninitialize the file system Layer
  * @param  None
  * @retval status value
  */
static uint8_t USBD_MTP_Itf_DeInit(void)
{
  return 0;
}

/**
  * @brief  USBD_MTP_Itf_GetIdx
  *         Get all object handle
  * @param  Param3: current object handle
  * @param  obj_handle: all objects handle (subfolders/files) in current object
  * @retval number of object handle in current object
  */
static uint32_t USBD_MTP_Itf_GetIdx(uint32_t Param3, uint32_t *obj_handle)
{
  uint32_t count = 0U;
  //content of root dir, we not use folders
  if((Param3==0)||(Param3==0xFFFFFFFF)){
    uint32_t i=0;
    //go over all bojects
    for(i=0;i<MAX_OBJECTS;i++){
      //if obejct is stored id is not 0
      if(object[i].Storage_id!=0){
        //return handle from object with Storage_id is located in  obj_handle[Storage_id-1]
        obj_handle[count]=object[i].Storage_id;
        count++;
      }
    }

  }

  return count;
}

/**
  * @brief  USBD_MTP_Itf_GetParentObject
  *         Get parent object
  * @param  Param: object handle (object index)
  * @retval parent object
  */
static uint32_t USBD_MTP_Itf_GetParentObject(uint32_t Param)
{
  uint32_t parentobj = 0U;
  //get parent obejct except root dir
  if((Param< MAX_OBJECTS+1)&& (Param!=0)){
    parentobj= object[Param-1].ParentObject;
  }

  return parentobj;
}

/**
  * @brief  USBD_MTP_Itf_GetObjectFormat
  *         Get object format
  * @param  Param: object handle (object index)
  * @retval object format
  */
static uint16_t USBD_MTP_Itf_GetObjectFormat(uint32_t Param)
{
  uint16_t objformat = 0U;
  if((Param< MAX_OBJECTS+1)&& (Param!=0)){
    objformat= object[Param-1].AssociationType;
  }


  return objformat;
}

/**
  * @brief  USBD_MTP_Itf_GetObjectName_len
  *         Get object name length
  * @param  Param: object handle (object index)
  * @retval object name length
  */
static uint8_t USBD_MTP_Itf_GetObjectName_len(uint32_t Param)
{
  uint8_t obj_len = 0U;
  if((Param< MAX_OBJECTS+1)&& (Param!=0)){
    obj_len= object[Param-1].Filename_len;
  }


  return obj_len;
}

/**
  * @brief  USBD_MTP_Itf_GetObjectName
  *         Get object name
  * @param  Param: object handle (object index)
  * @param  obj_len: length of object name
  * @param  buf: pointer to object name
  * @retval object size in SD card
  */
static void USBD_MTP_Itf_GetObjectName(uint32_t Param, uint8_t obj_len, uint16_t *buf)
{
  if((Param< MAX_OBJECTS+1)&& (Param!=0)){ 
    memcpy(buf,object[Param-1].Filename,obj_len*2);
  }

  return;
}

/**
  * @brief  USBD_MTP_Itf_GetObjectSize
  *         Get size of current object
  * @param  Param: object handle (object index)
  * @retval object size in SD card
  */
static uint32_t USBD_MTP_Itf_GetObjectSize(uint32_t Param)
{
  uint32_t ObjCompSize = 0U;
  if((Param< MAX_OBJECTS+1)&& (Param!=0)){ 
    ObjCompSize=object[Param-1].ObjectCompressedSize;
  }

  return ObjCompSize;
}

/**
  * @brief  USBD_MTP_Itf_Create_NewObject
  *         Create new object in SD card and store necessary information for future use
  * @param  ObjectInfo: object information to use
  * @param  objhandle: object handle (object index)
  * @retval None
  */
static uint16_t USBD_MTP_Itf_Create_NewObject(MTP_ObjectInfoTypeDef ObjectInfo, uint32_t objhandle)
{
  uint16_t rep_code = 0U;
  UNUSED(ObjectInfo);
  UNUSED(objhandle);

  return rep_code;
}

/**
  * @brief  USBD_MTP_Itf_GetMaxCapability
  *         Get max capability in SD card
  * @param  None
  * @retval max capability
  */
static uint64_t USBD_MTP_Itf_GetMaxCapability(void)
{
  uint64_t max_cap = 0U;//0 is read write

  return max_cap;
}

/**
  * @brief  USBD_MTP_Itf_GetFreeSpaceInBytes
  *         Get free space in bytes in SD card
  * @param  None
  * @retval free space in bytes
  */
static uint64_t USBD_MTP_Itf_GetFreeSpaceInBytes(void)
{
  uint64_t f_space_inbytes = emptySpace;

  return f_space_inbytes;
}

/**
  * @brief  USBD_MTP_Itf_GetNewIndex
  *         Create new object handle
  * @param  objformat: object format
  * @retval object handle
  */
static uint32_t USBD_MTP_Itf_GetNewIndex(uint16_t objformat)
{
  uint32_t n_index = 0U;
  UNUSED(objformat);

  return n_index;
}

/**
  * @brief  USBD_MTP_Itf_WriteData
  *         Write file data to SD card
  * @param  len: size of data to write
  * @param  buff: data to write in SD card
  * @retval None
  */
static void USBD_MTP_Itf_WriteData(uint16_t len, uint8_t *buff)
{
  UNUSED(len);
  UNUSED(buff);

  return;
}

/**
  * @brief  USBD_MTP_Itf_GetContainerLength
  *         Get length of generic container
  * @param  Param1: object handle
  * @retval length of generic container
  */
static uint32_t USBD_MTP_Itf_GetContainerLength(uint32_t Param1)
{
  uint32_t length = 0U;
  if((Param1< MAX_OBJECTS+1)&& (Param1!=0)){ 
    length = bufferLen[Param1-1];
  }



  return length;
}

/**
  * @brief  USBD_MTP_Itf_DeleteObject
  *         delete object from SD card
  * @param  Param1: object handle (file/folder index)
  * @retval response code
  */
static uint16_t USBD_MTP_Itf_DeleteObject(uint32_t Param1)
{
  uint16_t rep_code = 0U;
  UNUSED(Param1);

  return rep_code;
}

/**
  * @brief  USBD_MTP_Get_idx_to_delete
  *         Get all files/foldres index to delete with descending order ( max depth)
  * @param  Param: object handle (file/folder index)
  * @param  tab: pointer to list of files/folders to delete
  * @retval Number of files/folders to delete
  */
/* static uint32_t USBD_MTP_Get_idx_to_delete(uint32_t Param, uint8_t *tab)
{
  uint32_t cnt = 0U;

  return cnt;
}
*/

/**
  * @brief  USBD_MTP_Itf_ReadData
  *         Read data from SD card
  * @param  Param1: object handle
  * @param  buff: pointer to data to be read
  * @param  temp_length: current data size read
  * @retval necessary information for next read/finish reading
  */
static uint32_t USBD_MTP_Itf_ReadData(uint32_t Param1, uint8_t *buff, MTP_DataLengthTypeDef *data_length)
{
  UNUSED(Param1);
  #define HEADER_SHIFT 12
  #define READ_MAX_SIZE (64 - HEADER_SHIFT)
  uint32_t temp_length = data_length->temp_length;
  uint32_t totallen = object[Param1-1].ObjectCompressedSize;;
  uint32_t copyLength;
  if((Param1< MAX_OBJECTS+1)&& (Param1!=0)){ 
    if((totallen - temp_length) < READ_MAX_SIZE){

      copyLength = (totallen - temp_length);
    }else{
      copyLength=MAX_OBJECTS;
    }
    //check if we need to keep space for header => temp_length=0 means this is first message with header 
    if(temp_length==0){

      memcpy(buff+HEADER_SHIFT,(uint8_t*)(buffer[Param1-1]+temp_length),copyLength);
    }else{
      memcpy(buff,(uint8_t*)(buffer[Param1-1]+temp_length),copyLength);
    }
    data_length->temp_length+=copyLength;
    data_length->readbytes = copyLength;
  }

  return 0U;
}

/**
  * @brief  USBD_MTP_Itf_Cancel
  *         Close opened folder/file while cancelling transaction
  * @param  MTP_ResponsePhase: MTP current state
  * @retval None
  */
static void USBD_MTP_Itf_Cancel(uint32_t Phase)
{
  UNUSED(Phase);

  /* Make sure to close open file while canceling transaction */

  return;
}
