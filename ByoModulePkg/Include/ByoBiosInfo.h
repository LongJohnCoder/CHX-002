
#ifndef __BYO_BIOS_INFO_H__
#define __BYO_BIOS_INFO_H__

#include <Uefi.h>

#define BIOS_BLOCK_TYPE_FV_SEC            1
#define BIOS_BLOCK_TYPE_FV_BB             2
#define BIOS_BLOCK_TYPE_FV_MAIN           3
#define BIOS_BLOCK_TYPE_FV_MICROCODE      4
#define BIOS_BLOCK_TYPE_FV_NVRAM          5
#define BIOS_BLOCK_TYPE_FV_LOGO           6
#define BIOS_BLOCK_TYPE_ROM_HOLE          0x80
#define BIOS_BLOCK_TYPE_ROM_HOLE_SMBIOS   0x81



#define BYO_BIOS_INFO_SIGNATURE         SIGNATURE_32('$', 'B', 'I', '$')
#define BYO_BIOS_INFO_SIGNATURE2        SIGNATURE_32('_', 'B', 'Y', 'O')
#define BYO_BIOS_INFO_VERSION           1


typedef struct {
  UINT8   Type;
  UINT32  Base;
  UINT32  Size;
  UINT32  Sign;             // for instance
} BIOS_IMAGE_BLOCK_INFO;

typedef struct {
  UINT32                 Signature;
  UINT32                 Length;
  UINT32                 Version;
  UINT32                 Reserved;  
  UINT32                 Signature2;
  UINT32                 BiosBaseAddr;
  UINT32                 BiosSize;
  UINT32                 InfoCount;
} BYO_BIOS_INFO_HEADER;
 
typedef struct {
  BYO_BIOS_INFO_HEADER   Header;
  BIOS_IMAGE_BLOCK_INFO  Info[1];
} BYO_BIOS_INFO;


#endif


