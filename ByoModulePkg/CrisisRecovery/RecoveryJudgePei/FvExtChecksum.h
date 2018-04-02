/** @file
  This Header file contain the definition of the structures and 
  GUID used in FV extented file. 

**/

#ifndef __FV_EXT_CHECKSUM_H__
#define __FV_EXT_CHECKSUM_H__

#define FV_CHECK_ALIGNMENT 1024

//
// FV Extended Checksum
//
#pragma pack(1)
typedef struct {
  UINT16          Version;
  UINT32          Checksum;
} EFI_FIRMWARE_VOLUME_EXT_CHECKSUM;
#pragma pack()

extern EFI_GUID   gFvExtChecksumFileNameGuid;

#endif

