#ifndef __MYSDFAT_H
#define __MYSDFAT_H


#include<stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum
{
    FAT12,
    FAT16,
    FAT32
} FATtype;

typedef struct
{
    uint32_t Cluster;
    uint8_t sectorIndex;
    uint8_t entryIndex;
    uint8_t LFN_EntCnt;
} fileEntInf_t;

typedef struct
{
    uint32_t FSI_LeadSig;
    uint8_t FSI_Reserved1[480];
    uint32_t FSI_StrucSig;
    uint32_t FSI_Free_Count;
    uint32_t FSI_Nxt_Free;
    uint8_t FSI_Reserved2[12];
    uint32_t FSI_TrailSig;

} FSInfo_t;

typedef struct
{
    char DIR_Name[8];
    char DIR_ext[3];
    uint8_t DIR_attr;
    uint8_t DIR_NTRes;
    uint8_t DIR_CrtTimeTenth;
    uint16_t DIR_CrtTime;
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;
    uint16_t DIR_FstClusHI;
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
    uint32_t entryIndex;
    fileEntInf_t fileEntInf;

} myFile;

bool mySdFat_init();

bool listDir(const char *path);

bool readFile(const char *path, const char *fileName);

myFile fileOpen(const char *path, const char *filename);

uint8_t fileLfnEntCnt(myFile *pFile);

void fileClose(myFile *pFile);

uint8_t readByte(myFile* pFile);

myFile createDirectory(const char *path, const char *dirName);

bool fileWrite(myFile *pFile, const char *data);

bool fileDelete(const char *path, const char *filename);

myFile nextFile(myFile *pFile);

myFile pathExists(const char *path);

uint32_t startCluster(myFile *pFile);

bool isValidFile(myFile *pFile);

bool isEndOfDir(myFile *pFile);

void fileReset(myFile *pFile);

uint32_t fileSize(myFile  *pFile);

extern char fileName[128];

typedef struct
{
    uint32_t fatSecNum;
    uint16_t fatEntOffset;
} fatEntLoc_t;

typedef fileEntInf_t freeEntInf_t;

typedef struct
{
    uint8_t LDIR_Ord;
    char LDIR_Name1[10];
    uint8_t LDIR_Attr;
    uint8_t LDIR_Type;
    uint8_t LDIR_Chksum;
    char LDIR_Name2[12];
    uint16_t LDIR_FstClusLO;
    char LDIR_Name3[4];
} LFN_entry_t;

typedef struct
{
    uint16_t BPB_BytesPerSec;
    uint8_t BPB_SecPerClus;
    uint16_t BPB_RsvdSecCnt;
    uint8_t BPB_NumFATs;
    uint16_t BPB_RootEntCnt;
    uint32_t BPB_TotSec32;
    uint32_t BPB_FATSz32;
    uint32_t BPB_RootClus;
    uint16_t BPB_FSInfo;
    char BS_VolLab[11];
} bootSecParams_t;

#endif
