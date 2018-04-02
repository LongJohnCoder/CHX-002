/** @file

Copyright (c) 2006 - 2011, Byosoft Corporation.<BR> 
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in 
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced, 
stored in a retrieval system, or transmitted in any form or by any 
means without the express written consent of Byosoft Corporation.

File Name:
  MMCSDTransfer.c

Abstract: 
  SD card Model.

Revision History:
Bug 2026: Description of this bug.
TIME: 2011-05-16
$AUTHOR: Mac Peng
$REVIEWERS: Donald Guan
$SCOPE: SD card feature support.
$TECHNICAL: .
$END--------------------------------------------------------------------------

**/
/*++
  This file contains an 'Intel Peripheral Driver' and uniquely
  identified as Intel "Mobile Silicon Support Code" and is
  licensed for Intel CPUs and chipsets under the terms of your
  license agreement with Intel or your vendor.  This file may
  be modified by the user, subject to additional terms of the
  license agreement
--*/
/** @file

Copyright (c) 2007 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


File Name:

  MMCSDTransfer.c

Abstract:

  MMC/SD transfer specific functions

**/
#include "MediaDeviceDriver.h"

//Dongbo add for set backdoor, +S.
EFI_STATUS
SetSdhcBackDoorReg(
  IN   EFI_SD_HOST_IO_PROTOCOL    *This,
  IN   UINTN    Offset,
  IN   UINT8    RegVal
)
{
  UINT8   Value8;
  UINTN   MmioBar;

  MmioBar = This->Slot0RegMap;

  DEBUG ((EFI_D_ERROR, "SetSdhcBackDoorReg: MmioBar = 0x%08x.\n", MmioBar));

  if (Offset == 0x185) {
    // Enable backdoor enable bit.
    Value8 = *(UINT8 *)(MmioBar + 0x180);
    Value8 |= 1;
    *(UINT8 *)(MmioBar + 0x180) = Value8;
  }

  // Modify the backdoor register.
  *(UINT8 *)(MmioBar + Offset) = RegVal;

  // Dump
  gBS->Stall (1000); // Delay 1ms
  Value8 = *(UINT8 *)(MmioBar + Offset);
  DEBUG ((EFI_D_ERROR, "SetSdhcBackDoorReg: Set MMIO Rx%x = 0x%02x.\n", Offset, Value8));

  return EFI_SUCCESS;
}

//Dongbo add for set backdoor, +E.

/**
  Check card status, print the debug info and check the error.

  @param  Status                  Status got from card status register.

  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
**/
EFI_STATUS
CheckCardStatus (
  IN  UINT32    Status
  )
{
  CARD_STATUS    *CardStatus;
  CardStatus = (CARD_STATUS*)(&Status);

  if (CardStatus->ADDRESS_OUT_OF_RANGE) {
    DEBUG ((EFI_D_ERROR, "CardStatus: ADDRESS_OUT_OF_RANGE\n"));
  }

  if (CardStatus->ADDRESS_MISALIGN) {
    DEBUG ((EFI_D_ERROR, "CardStatus: ADDRESS_MISALIGN\n"));
  }

  if (CardStatus->BLOCK_LEN_ERROR) {
    DEBUG ((EFI_D_ERROR, "CardStatus: BLOCK_LEN_ERROR\n"));
  }

  if (CardStatus->ERASE_SEQ_ERROR) {
    DEBUG ((EFI_D_ERROR, "CardStatus: ERASE_SEQ_ERROR\n"));
  }

  if (CardStatus->ERASE_PARAM) {
    DEBUG ((EFI_D_ERROR, "CardStatus: ERASE_PARAM\n"));
  }

  if (CardStatus->WP_VIOLATION) {
    DEBUG ((EFI_D_ERROR, "CardStatus: WP_VIOLATION\n"));
  }

  if (CardStatus->CARD_IS_LOCKED) {
    DEBUG ((EFI_D_ERROR, "CardStatus: CARD_IS_LOCKED\n"));
  }

  if (CardStatus->LOCK_UNLOCK_FAILED) {
    DEBUG ((EFI_D_ERROR, "CardStatus: LOCK_UNLOCK_FAILED\n"));
  }

  if (CardStatus->COM_CRC_ERROR) {
    DEBUG ((EFI_D_ERROR, "CardStatus: COM_CRC_ERROR\n"));
  }

  if (CardStatus->ILLEGAL_COMMAND) {
    DEBUG ((EFI_D_ERROR, "CardStatus: ILLEGAL_COMMAND\n"));
  }

  if (CardStatus->CARD_ECC_FAILED) {
    DEBUG ((EFI_D_ERROR, "CardStatus: CARD_ECC_FAILED\n"));
  }

  if (CardStatus->CC_ERROR) {
    DEBUG ((EFI_D_ERROR, "CardStatus: CC_ERROR\n"));
  }

  if (CardStatus->ERROR) {
    DEBUG ((EFI_D_ERROR, "CardStatus: ERROR\n"));
  }

  if (CardStatus->UNDERRUN) {
    DEBUG ((EFI_D_ERROR, "CardStatus: UNDERRUN\n"));
  }

  if (CardStatus->OVERRUN) {
    DEBUG ((EFI_D_ERROR, "CardStatus: OVERRUN\n"));
  }

  if (CardStatus->CID_CSD_OVERWRITE) {
    DEBUG ((EFI_D_ERROR, "CardStatus: CID_CSD_OVERWRITE\n"));
  }

  if (CardStatus->WP_ERASE_SKIP) {
    DEBUG ((EFI_D_ERROR, "CardStatus: WP_ERASE_SKIP\n"));
  }

  if (CardStatus->ERASE_RESET) {
    DEBUG ((EFI_D_ERROR, "CardStatus: ERASE_RESET\n"));
  }

  if (CardStatus->SWITCH_ERROR) {
    DEBUG ((EFI_D_ERROR, "CardStatus: SWITCH_ERROR\n"));
  }

  if ((Status & 0xFCFFA080) != 0) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Send command by using Host IO protocol.

  @param  This                    Pointer to EFI_SD_HOST_IO_PROTOCOL.
  @param  CommandIndex            The command index to set the command index field of command register.
  @param  Argument                Command argument to set the argument field of command register.
  @param  DataType                TRANSFER_TYPE, indicates no data, data in or data out.
  @param  Buffer                  Contains the data read from / write to the device.
  @param  BufferSize              The size of the buffer.
  @param  ResponseType            RESPONSE_TYPE.
  @param  TimeOut                 Time out value in 1 ms unit.
  @param  ResponseData            Depending on the ResponseType, such as CSD or card status.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_UNSUPPORTED         Command unsuported.

**/
EFI_STATUS
SendCommand (
  IN   EFI_SD_HOST_IO_PROTOCOL    *This,
  IN   UINT16                     CommandIndex,
  IN   UINT32                     Argument,
  IN   TRANSFER_TYPE              DataType,
  IN   UINT8                      *Buffer, OPTIONAL
  IN   UINT32                     BufferSize,
  IN   RESPONSE_TYPE              ResponseType,
  IN   UINT32                     TimeOut,
  OUT  UINT32                     *ResponseData
  )
{

  EFI_STATUS    Status;

  Status = This->SendHCommand (
                   This,
                   CommandIndex,
                   Argument,
                   DataType,
                   Buffer,
                   BufferSize,
                   ResponseType,
                   TimeOut,
                   ResponseData
                   );
  if (!EFI_ERROR (Status)) {
    if (ResponseType == ResponseR1 || ResponseType == ResponseR1b) {
      ASSERT(ResponseData != NULL);
      Status = CheckCardStatus (*ResponseData);
    }
  } else {
    This->ResetSdHost (This, Reset_DAT_CMD);
  }

  return Status;
}

/**
  Send the card APP_CMD command with the following command indicated
  by CommandIndex.

  @param  This                    Pointer to EFI_SD_HOST_IO_PROTOCOL.
  @param  CommandIndex            The command index to set the command index field of command register.
  @param  Argument                Command argument to set the argument field of command register.
  @param  DataType                TRANSFER_TYPE, indicates no data, data in or data out.
  @param  Buffer                  Contains the data read from / write to the device.
  @param  BufferSize              The size of the buffer.
  @param  ResponseType            RESPONSE_TYPE.
  @param  TimeOut                 Time out value in 1 ms unit.
  @param  ResponseData            Depending on the ResponseType, such as CSD or card status.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_UNSUPPORTED         Command unsuported.

**/
EFI_STATUS
SendAppCommand (
  IN   CARD_DATA                  *CardData,
  IN   UINT16                     CommandIndex,
  IN   UINT32                     Argument,
  IN   TRANSFER_TYPE              DataType,
  IN   UINT8                      *Buffer, OPTIONAL
  IN   UINT32                     BufferSize,
  IN   RESPONSE_TYPE              ResponseType,
  IN   UINT32                     TimeOut,
  OUT  UINT32                     *ResponseData
  )
{

  EFI_STATUS                 Status;
  EFI_SD_HOST_IO_PROTOCOL    *SdHostIo;
  UINT8                      Index;

  SdHostIo = CardData->SdHostIo;
  Status = EFI_SUCCESS;

  for (Index = 0; Index < 2; Index++) {
    Status = SdHostIo->SendHCommand (
                         SdHostIo,
                         APP_CMD,
                         (CardData->Address << 16),
                         NoData,
                         NULL,
                         0,
                         ResponseR1,
                         TIMEOUT_COMMAND,
                         (UINT32*)&(CardData->CardStatus)
                         );
    if (!EFI_ERROR (Status)) {
        Status = CheckCardStatus (*(UINT32*)&(CardData->CardStatus));
        if (CardData->CardStatus.SAPP_CMD != 1) {
          Status = EFI_DEVICE_ERROR;
        }
        if (!EFI_ERROR (Status)) {
           break;
        }
    } else {
       SdHostIo->ResetSdHost (SdHostIo, Reset_Auto);
    }
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SdHostIo->SendHCommand (
                       SdHostIo,
                       CommandIndex,
                       Argument,
                       DataType,
                       Buffer,
                       BufferSize,
                       ResponseType,
                       TimeOut,
                       ResponseData
                       );
  if (!EFI_ERROR (Status)) {
    if (ResponseType == ResponseR1 || ResponseType == ResponseR1b) {
      ASSERT(ResponseData != NULL);
      Status = CheckCardStatus (*ResponseData);
    }
  } else {
    SdHostIo->ResetSdHost (SdHostIo, Reset_Auto);
  }

  return Status;
}

EFI_STATUS
MmcDecodeOCR (
  IN OCR  *OCRReg
  ) 
{
  DEBUG((EFI_D_INFO, "\n==========DECODE MMC OCR REGISTER==================\n"));
  DEBUG((EFI_D_ERROR," OCR = 0x%08X\n", *((UINT32 *)OCRReg)));
  DEBUG((EFI_D_ERROR, " CARD_NOT_BUSY      = 0x%X\n",OCRReg->Busy));
  DEBUG((EFI_D_ERROR, " ACCESS_MODE        = 0x%X\n",OCRReg->AccessMode));
  DEBUG((EFI_D_ERROR, " VDD_270_360        = 0x%X\n",OCRReg->V270_V360));
  DEBUG((EFI_D_ERROR, " VDD_200_260        = 0x%X\n",OCRReg->V200_V260));
  DEBUG((EFI_D_ERROR, " VDD_170_195        = 0x%X\n",OCRReg->V170_V195));
  DEBUG((EFI_D_INFO, "==================================================\n"));
  return 0;
}



EFI_STATUS
MmcDecodeCID (
  IN CID  *CIDReg
  ) 
{
  UINT32 i = 0;
  DEBUG((EFI_D_INFO, "\n==========DECODE MMC CID REGISTER==================\n"));
  DEBUG((EFI_D_INFO," CID = 0x%032X\n",CIDReg));
  DEBUG((EFI_D_INFO," MANUFACTURER_ID     = 0x%X\n",CIDReg->MID));
  DEBUG((EFI_D_INFO," CARD_OR_BGA         = 0x%X\n",(CIDReg->OID & 0xFF00)>>6));
  DEBUG((EFI_D_INFO," OEM_APPLICATION_ID  = 0x%X\n",(CIDReg->OID>>8)&0xFF));
  DEBUG((EFI_D_INFO," PRODUCT_NAME        = "));
  for (i=0; i < 6; i++) {
    DEBUG((EFI_D_INFO, "%c",CIDReg->PNM[i]));
  }
  DEBUG((EFI_D_INFO, "\n"));

  DEBUG((EFI_D_INFO," PRODUCT_REVISION    = 0x%X\n",CIDReg->PRV));
  DEBUG((EFI_D_INFO," PRODUCT_SERIAL_NUM  = 0x%X\n",CIDReg->PSN));
  DEBUG((EFI_D_INFO," MANUFACTURE_DATE    = 0x%X\n",CIDReg->MDT));
  DEBUG((EFI_D_INFO, "==================================================\n"));
  return 0;
}

EFI_STATUS
MmcDecodeCSD (
  IN CSD  *CSDReg
  ) 
{
  DEBUG((EFI_D_INFO, "\n==========DECODE MMC CSD REGISTER==================\n"));
  DEBUG((EFI_D_INFO, "csd_struct        : [0x%0x] \n",CSDReg->CSD_STRUCTURE));
  DEBUG((EFI_D_INFO, "specs_ver         : [0x%0x] \n",CSDReg->SPEC_VERS));
  DEBUG((EFI_D_INFO, "reserve2          : [0x%0x] \n",CSDReg->Reserved2));
  DEBUG((EFI_D_INFO, "taac              : [0x%0x] \n",CSDReg->TAAC));
  DEBUG((EFI_D_INFO, "nsac              : [0x%0x] \n",CSDReg->NSAC));
  DEBUG((EFI_D_INFO, "tran_speed        : [0x%0x] \n",CSDReg->TRAN_SPEED));
  DEBUG((EFI_D_INFO, "ccc               : [0x%0x] \n",CSDReg->CCC));
  DEBUG((EFI_D_INFO, "read_bl_len       : [0x%0x] \n",CSDReg->READ_BL_LEN));
  DEBUG((EFI_D_INFO, "read_partial      : [0x%0x] \n",CSDReg->READ_BL_PARTIAL));
  DEBUG((EFI_D_INFO, "write_misalign    : [0x%0x] \n",CSDReg->WRITE_BLK_MISALIGN));
  DEBUG((EFI_D_INFO, "read_misalign     : [0x%0x] \n",CSDReg->READ_BLK_MISALIGN));
  DEBUG((EFI_D_INFO, "dsr_imp           : [0x%0x] \n",CSDReg->DSR_IMP));
  DEBUG((EFI_D_INFO, "reserve1          : [0x%0x] \n",CSDReg->Reserved1));
  DEBUG((EFI_D_INFO, "c_size            : [0x%0x] \n",CSDReg->C_SIZELow2 | CSDReg->C_SIZEHigh10<<2));
  DEBUG((EFI_D_INFO, "vdd_r_curr_min    : [0x%0x] \n",CSDReg->VDD_R_CURR_MIN));
  DEBUG((EFI_D_INFO, "vdd_r_curr_max    : [0x%0x] \n",CSDReg->VDD_R_CURR_MAX));
  DEBUG((EFI_D_INFO, "vdd_w_curr_min    : [0x%0x] \n",CSDReg->VDD_W_CURR_MIN));
  DEBUG((EFI_D_INFO, "vdd_w_curr_max    : [0x%0x] \n",CSDReg->VDD_W_CURR_MAX));
  DEBUG((EFI_D_INFO, "c_size_mult       : [0x%0x] \n",CSDReg->C_SIZE_MULT));
  DEBUG((EFI_D_INFO, "erase_grp_size    : [0x%0x] \n",CSDReg->ERASE_GRP_SIZE));
  DEBUG((EFI_D_INFO, "erase_grp_mult    : [0x%0x] \n",CSDReg->ERASE_GRP_MULT));
  DEBUG((EFI_D_INFO, "wp_grp_size       : [0x%0x] \n",CSDReg->WP_GRP_SIZE));
  DEBUG((EFI_D_INFO, "wp_grp_enable     : [0x%0x] \n",CSDReg->WP_GRP_ENABLE));
  DEBUG((EFI_D_INFO, "default_ecc       : [0x%0x] \n",CSDReg->DEFAULT_ECC));
  DEBUG((EFI_D_INFO, "r2w_factor        : [0x%0x] \n",CSDReg->R2W_FACTOR));
  DEBUG((EFI_D_INFO, "write_bl_len      : [0x%0x] \n",CSDReg->WRITE_BL_LEN));
  DEBUG((EFI_D_INFO, "write_partial     : [0x%0x] \n",CSDReg->WRITE_BL_PARTIAL));
  DEBUG((EFI_D_INFO, "reserve0          : [0x%0x] \n",CSDReg->Reserved0));
  DEBUG((EFI_D_INFO, "content_prot_app  : [0x%0x] \n",CSDReg->CONTENT_PROT_APP));
  DEBUG((EFI_D_INFO, "file_format_grp   : [0x%0x] \n",CSDReg->FILE_FORMAT_GRP));
  DEBUG((EFI_D_INFO, "copy              : [0x%0x] \n",CSDReg->COPY));
  DEBUG((EFI_D_INFO, "perm_write_protect: [0x%0x] \n",CSDReg->PERM_WRITE_PROTECT));
  DEBUG((EFI_D_INFO, "tmp_write_prot    : [0x%0x] \n",CSDReg->TMP_WRITE_PROTECT));
  DEBUG((EFI_D_INFO, "file_format       : [0x%0x] \n",CSDReg->FILE_FORMAT));
  DEBUG((EFI_D_INFO, "ecc               : [0x%0x] \n",CSDReg->ECC));
  DEBUG((EFI_D_INFO, "==================================================\n"));
  return 0;
}

EFI_STATUS
MmcDecodeExtCSD (
  IN EXT_CSD  *ExtCSDReg
  ) 
{
  DEBUG((EFI_D_INFO, "\n==========DECODE MMC EXT CSD REGISTER==================\n"));
  DEBUG((EFI_D_INFO," SUPPORTED_CMD_SETS        = 0x%X\n",ExtCSDReg->CMD_SET));
  DEBUG((EFI_D_INFO," HPI_FEATURES              = 0x%X\n",ExtCSDReg->HPI_FEATURES));
  DEBUG((EFI_D_INFO," BKOPS_SUPPORT             = 0x%X\n",ExtCSDReg->BKOPS_SUPPORT));
  DEBUG((EFI_D_INFO," BKOPS_STATUS              = 0x%X\n",ExtCSDReg->BKOPS_STATUS));
  DEBUG((EFI_D_INFO," CORRECTLY_PRG_SECTORS_NUM = 0x%X%X%X%X\n",ExtCSDReg->CORRECTLY_PRG_SECTORS_NUM[3], \
  	                    ExtCSDReg->CORRECTLY_PRG_SECTORS_NUM[2], ExtCSDReg->CORRECTLY_PRG_SECTORS_NUM[1], ExtCSDReg->CORRECTLY_PRG_SECTORS_NUM[0]));
  DEBUG((EFI_D_INFO," INI_TIMEOUT_AP            = 0x%X\n",ExtCSDReg->INI_TIMEOUT_AP));
  DEBUG((EFI_D_INFO," PWR_CL_DDR_52_195         = 0x%X\n",ExtCSDReg->PWR_CL_DDR_52_195));
  DEBUG((EFI_D_INFO," PWR_CL_DDR_52_360         = 0x%X\n",ExtCSDReg->PWR_CL_DDR_52_360));
  DEBUG((EFI_D_INFO," MIN_PRF_DDR_W_8_52        = 0x%X\n",ExtCSDReg->MIN_PERF_DDR_W_8_52));
  DEBUG((EFI_D_INFO," MIN_PRF_DDR_R_8_52        = 0x%X\n",ExtCSDReg->MIN_PERF_DDR_R_8_52));
  DEBUG((EFI_D_INFO," TRIM_MULT                 = 0x%X\n",ExtCSDReg->TRIM_MULT));
  DEBUG((EFI_D_INFO," SEC_FEATURE_SUPP          = 0x%X\n",ExtCSDReg->SEC_FEATURE_SUPPORT));
  DEBUG((EFI_D_INFO," SEC_ERASE_MULT            = 0x%X\n",ExtCSDReg->SEC_ERASE_MULT));
  DEBUG((EFI_D_INFO," SEC_TRIM_MULT             = 0x%X\n",ExtCSDReg->SEC_TRIM_MULT));
  DEBUG((EFI_D_INFO," BOOT_INFO                 = 0x%X\n",ExtCSDReg->BOOT_INFO));
  DEBUG((EFI_D_INFO," BOOT_PART_SIZE            = 0x%X\n",ExtCSDReg->BOOT_SIZE_MULTI));
  DEBUG((EFI_D_INFO," ACCESS_SIZE               = 0x%X\n",ExtCSDReg->ACC_SIZE));
  DEBUG((EFI_D_INFO," HI_CAP_ER_GRP_SIZE        = 0x%X\n",ExtCSDReg->HC_ERASE_GRP_SIZE));
  DEBUG((EFI_D_INFO," HI_CAP_ER_TIMEOUT         = 0x%X\n",ExtCSDReg->ERASE_TIMEOUT_MULT));
  DEBUG((EFI_D_INFO," REL_WR_SECTOR_CNT         = 0x%X\n",ExtCSDReg->REL_WR_SEC_C));
  DEBUG((EFI_D_INFO," HI_CAP_WP_GRP_SIZE        = 0x%X\n",ExtCSDReg->HC_WP_GRP_SIZE));
  DEBUG((EFI_D_INFO," SLEEP_CURRENT_VCC         = 0x%X\n",ExtCSDReg->S_C_VCC));
  DEBUG((EFI_D_INFO," SLEEP_CURRENT_VCCQ        = 0x%X\n",ExtCSDReg->S_C_VCCQ));
  DEBUG((EFI_D_INFO," SLP_AWK_TIMEOUT           = 0x%X\n",ExtCSDReg->S_A_TIMEOUT));
  DEBUG((EFI_D_INFO," SECTOR_COUNT              = 0x%X\n",*(UINT32*)((UINT8 *)&ExtCSDReg->SEC_COUNT)));
  DEBUG((EFI_D_INFO," MIN_PERF_W_8_52           = 0x%X\n",ExtCSDReg->MIN_PERF_W_8_52));
  DEBUG((EFI_D_INFO," MIN_PERF_R_8_52           = 0x%X\n",ExtCSDReg->MIN_PERF_R_8_52));
  DEBUG((EFI_D_INFO," MIN_PERF_W_8_26_4_52      = 0x%X\n",ExtCSDReg->MIN_PERF_W_8_26_4_52));
  DEBUG((EFI_D_INFO," MIN_PERF_W_8_26_4_52      = 0x%X\n",ExtCSDReg->MIN_PERF_W_8_26_4_52));
  DEBUG((EFI_D_INFO," MIN_PERF_W_4_26           = 0x%X\n",ExtCSDReg->MIN_PERF_W_4_26));
  DEBUG((EFI_D_INFO," MIN_PERF_R_4_26           = 0x%X\n",ExtCSDReg->MIN_PERF_R_4_26));
  DEBUG((EFI_D_INFO," PWR_CLASS_26_360          = 0x%X\n",ExtCSDReg->PWR_CL_26_360));
  DEBUG((EFI_D_INFO," PWR_CLASS_52_360          = 0x%X\n",ExtCSDReg->PWR_CL_52_360));
  DEBUG((EFI_D_INFO," PWR_CLASS_26_195          = 0x%X\n",ExtCSDReg->PWR_CL_26_195));
  DEBUG((EFI_D_INFO," PWR_CLASS_52_195          = 0x%X\n",ExtCSDReg->PWR_CL_52_195));
  DEBUG((EFI_D_INFO," PARTITION_SWITCH_TIME     = 0x%X\n",ExtCSDReg->PARTITION_SWITCH_TIME));
  DEBUG((EFI_D_INFO," OUT_OF_INTERRUPT_TIME     = 0x%X\n",ExtCSDReg->OUT_OF_INTERRUPT_TIME));
  DEBUG((EFI_D_INFO," CARD_TYPE                 = 0x%X\n",ExtCSDReg->CARD_TYPE));
  DEBUG((EFI_D_INFO," CSD_STRUCTURE             = 0x%X\n",ExtCSDReg->CSD_STRUCTURE));
  DEBUG((EFI_D_INFO," EXT_CSD_REV               = 0x%X\n",ExtCSDReg->EXT_CSD_REV));
  DEBUG((EFI_D_INFO," CMD_SET                   = 0x%X\n",ExtCSDReg->CMD_SET));
  DEBUG((EFI_D_INFO," CMD_SET_REV               = 0x%X\n",ExtCSDReg->CMD_SET_REV));
  DEBUG((EFI_D_INFO," PWR_CLASS                 = 0x%X\n",ExtCSDReg->POWER_CLASS));
  DEBUG((EFI_D_INFO," HI_SPEED_TIMING           = 0x%X\n",ExtCSDReg->HS_TIMING));
  DEBUG((EFI_D_INFO," BUS_WIDTH_MODE            = 0x%X\n",ExtCSDReg->BUS_WIDTH));
  DEBUG((EFI_D_INFO," ERASED_MEM_CONTENT        = 0x%X\n",ExtCSDReg->ERASED_MEM_CONT));
  DEBUG((EFI_D_INFO," PARTITION_CONFIG          = 0x%X\n",ExtCSDReg->PARTITION_CONFIG));
  DEBUG((EFI_D_INFO," BOOT_CONFIG_PROT          = 0x%X\n",ExtCSDReg->BOOT_CONFIG_PROT));
  DEBUG((EFI_D_INFO," BOOT_BUS_WIDTH            = 0x%X\n",ExtCSDReg->BOOT_BUS_WIDTH));
  DEBUG((EFI_D_INFO," HI_DEN_ER_GRP_DEF         = 0x%X\n",ExtCSDReg->ERASE_GROUP_DEF));
  DEBUG((EFI_D_INFO," BOOT_WP                   = 0x%X\n",ExtCSDReg->BOOT_WP));
  DEBUG((EFI_D_INFO," USER_WP                   = 0x%X\n",ExtCSDReg->USER_WP));
  DEBUG((EFI_D_INFO," FW_CONFIG                 = 0x%X\n",ExtCSDReg->FW_CONFIG));
  DEBUG((EFI_D_INFO," RPMB_SIZE_MULT            = 0x%X\n",ExtCSDReg->RPMB_SIZE_MULT));
  DEBUG((EFI_D_INFO," RST_N_FUNCTION            = 0x%X\n",ExtCSDReg->RST_n_FUNCTION));
  DEBUG((EFI_D_INFO," PARTITIONING_SUPP         = 0x%X\n",ExtCSDReg->PARTITIONING_SUPPORT));
  DEBUG((EFI_D_INFO," MAX_ENH_SIZE_MULT         = 0x%02X%02X%02X\n",ExtCSDReg->MAX_ENH_SIZE_MULT[2],ExtCSDReg->MAX_ENH_SIZE_MULT[1],ExtCSDReg->MAX_ENH_SIZE_MULT[0]));
  DEBUG((EFI_D_INFO," PART_ATTRIBUTE            = 0x%X\n",ExtCSDReg->PARTITIONS_ATTRIBUTES));
  DEBUG((EFI_D_INFO," PART_SETTING_COMP         = 0x%X\n",ExtCSDReg->PARTITION_SETTING_COMPLETED));
  DEBUG((EFI_D_INFO," GP_SIZE_MULT              = 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n",ExtCSDReg->GP_SIZE_MULT_4[2],ExtCSDReg->GP_SIZE_MULT_4[1],ExtCSDReg->GP_SIZE_MULT_4[0],
                                                  ExtCSDReg->GP_SIZE_MULT_3[2],ExtCSDReg->GP_SIZE_MULT_3[1],ExtCSDReg->GP_SIZE_MULT_3[0],
                                                  ExtCSDReg->GP_SIZE_MULT_2[2],ExtCSDReg->GP_SIZE_MULT_2[1],ExtCSDReg->GP_SIZE_MULT_2[0],
                                                  ExtCSDReg->GP_SIZE_MULT_1[2],ExtCSDReg->GP_SIZE_MULT_1[1],ExtCSDReg->GP_SIZE_MULT_1[0]));
  DEBUG((EFI_D_INFO," ENH_SIZE_MULT             = 0x%02X%02X%02X\n",ExtCSDReg->ENH_SIZE_MULT[2],ExtCSDReg->ENH_SIZE_MULT[1],ExtCSDReg->ENH_SIZE_MULT[0]));
  DEBUG((EFI_D_INFO," ENH_START_ADDR            = 0x%02X%02X%02X%02X\n",ExtCSDReg->ENH_START_ADDR[3],ExtCSDReg->ENH_START_ADDR[2],ExtCSDReg->ENH_START_ADDR[1],ExtCSDReg->ENH_START_ADDR[0]));
  DEBUG((EFI_D_INFO," SEC_BAD_BLK_MGMNT         = 0x%X\n",ExtCSDReg->SEC_BAD_BLOCK_MGMNT));
  DEBUG((EFI_D_INFO, "==================================================\n"));
  return 0;
}


/**
  Send the card FAST_IO command.

  @param  CardData                Pointer to CARD_DATA.
  @param  RegisterAddress         Register Address.
  @param  RegisterData            Pointer to register Data.
  @param  Write                   TRUE for write, FALSE for read.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_DEVICE_ERROR        Hardware Error.
  @retval EFI_UNSUPPORTED         Command unsuported.

**/
EFI_STATUS
FastIO (
  IN      CARD_DATA   *CardData,
  IN      UINT8       RegisterAddress,
  IN  OUT UINT8       *RegisterData,
  IN      BOOLEAN     Write
  )
{
  EFI_SD_HOST_IO_PROTOCOL    *SdHostIo;
  EFI_STATUS                 Status;
  UINT32                     Argument;
  UINT32                     Data;

  Status   = EFI_SUCCESS;
  SdHostIo = CardData->SdHostIo;
  
  if (RegisterData == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  Argument = (CardData->Address << 16) | (RegisterAddress << 8);
  if (Write) {
    Argument |= BIT15 | (*RegisterData);
  }

  Status = SendCommand (
             SdHostIo, 
             FAST_IO,
             Argument,
             NoData, 
             NULL,
             0,
             ResponseR4,
             TIMEOUT_COMMAND,
             &Data
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  if ((Data & BIT15) == 0) {
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

  if (!Write) {
   *RegisterData = (UINT8)Data;
  }

Exit:
  return Status;
}

/**
  Send the card GO_INACTIVE_STATE command.

  @param  CardData                Pointer to CARD_DATA.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_UNSUPPORTED         Command unsuported.

**/
EFI_STATUS
PutCardInactive (
  IN  CARD_DATA   *CardData
  )
{
  EFI_SD_HOST_IO_PROTOCOL    *SdHostIo;
  EFI_STATUS                 Status;

  SdHostIo = CardData->SdHostIo;

  Status = SendCommand (
             SdHostIo, 
             GO_INACTIVE_STATE,
             (CardData->Address << 16),
             NoData, 
             NULL,
             0,
             ResponseNo,
             TIMEOUT_COMMAND,
             NULL
             );

  gBS->Stall (1000);
  return Status;

}

/**
  Get card interested information for CSD rergister.

  @param  CardData                Pointer to CARD_DATA.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_UNSUPPORTED         Command unsuported.

**/
EFI_STATUS
CaculateCardParameter (
  IN  CARD_DATA    *CardData
  )
{
  EFI_STATUS     Status;
  UINT32         Frequency;
  UINT32         Multiple;
  UINT32         CSize;
  CSD_SDV2       *CsdSDV2;

  Status = EFI_SUCCESS;

  switch (CardData->CSDRegister.TRAN_SPEED & 0x7) {
    case 0:
      Frequency = 100 * 1000;
      break;

    case 1:
      Frequency = 1 * 1000 * 1000;
      break;

    case 2:
      Frequency = 10 * 1000 * 1000;
      break;

    case 3:
      Frequency = 100 * 1000 * 1000;
      break;

    default:
      DEBUG((EFI_D_ERROR, "Invalid CSD TRAN_SPEED Frequency: 0x%x\n", CardData->CSDRegister.TRAN_SPEED & 0x7));
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
  }

  switch ((CardData->CSDRegister.TRAN_SPEED >> 3) & 0xF) {
    case 1:
      Multiple = 10;
      break;

    case 2:
      Multiple = 12;
      break;

    case 3:
      Multiple = 13;
      break;

    case 4:
      Multiple = 15;
      break;

    case 5:
      Multiple = 20;
      break;

    case 6:
      if (CardData->CardType == MMCCard) {
        Multiple = 26;
      } else {
        Multiple = 25;
      }
      break;

    case 7:
      Multiple = 30;
      break;

    case 8:
      Multiple = 35;
      break;

    case 9:
      Multiple = 40;
      break;

    case 10:
      Multiple = 45;
      break;

    case 11:
      if (CardData->CardType == MMCCard) {
        Multiple = 52;
      } else {
        Multiple = 50;
      }
      break;

    case 12:
      Multiple = 55;
      break;

    case 13:
      Multiple = 60;
      break;

    case 14:
      Multiple = 70;
      break;

    case 15:
      Multiple = 80;
      break;

    default:
      DEBUG((EFI_D_ERROR, "CalculateCardParameter: Invalid CSD TRAN_SPEED Multiple: 0x%x\n", CardData->CSDRegister.TRAN_SPEED >> 3));
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
  }

  Frequency = Frequency * Multiple / 10;
  CardData->MaxFrequency = Frequency;

  if ((CardData->ExtCSDRegister.CARD_TYPE & BIT2) ||
      (CardData->ExtCSDRegister.CARD_TYPE & BIT3)) {
    CardData->BlockLen = 512;
  } else {
    CardData->BlockLen = 1 << CardData->CSDRegister.READ_BL_LEN;
  }

  if (CardData->CardType == SDMemoryCard2High) {
    ASSERT(CardData->CSDRegister.CSD_STRUCTURE == 1);
    CsdSDV2 = (CSD_SDV2*)&CardData->CSDRegister;
    //
    // The SD Spec 2.0 says (CSize + 1) * 512K is the total size, so block numbber is (CSize + 1) * 1K
    // the K here means 1024 not 1000
    //
    CardData->BlockNumber = DivU64x32 (MultU64x32 (CsdSDV2->C_SIZE + 1, 512 * 1024) , CardData->BlockLen);
  } else {
    //
    // For MMC card > 2G, the block number will be recaculate later
    //
    CSize = CardData->CSDRegister.C_SIZELow2 | (CardData->CSDRegister.C_SIZEHigh10 << 2);
    CardData->BlockNumber = MultU64x32 (LShiftU64 (1, CardData->CSDRegister.C_SIZE_MULT + 2), CSize + 1);
  }

  //
  //For >= 2G card, BlockLen may be 1024, but the transfer size is still 512 bytes
  //
  if (CardData->BlockLen > 512) {
    CardData->BlockNumber = DivU64x32 (MultU64x32 (CardData->BlockNumber, CardData->BlockLen), 512);
    CardData->BlockLen    = 512;
  }

  DEBUG((
    EFI_D_ERROR,
          "CalculateCardParameter: Card Size: 0x%lx\n", MultU64x32 (CardData->BlockNumber, CardData->BlockLen)
    ));

Exit:
  return Status;
}

/**
  Test the bus width setting for MMC card
  It is used only for verification purpose.

  @param  CardData                Pointer to CARD_DATA.
  @param  Width                   1, 4, 8 bits.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_UNSUPPORTED         Command unsuported.

**/
EFI_STATUS
MmcCardBusWidthTest (
  IN  CARD_DATA             *CardData,
  IN  UINT32                Width
  )
{
  EFI_STATUS                 Status;
  EFI_SD_HOST_IO_PROTOCOL    *SdHostIo;
  UINT64                     Data; 
  UINT64                     Value; 

  ASSERT(CardData != NULL);

  SdHostIo = CardData->SdHostIo;

  Value = 0;

  switch (Width) {
    case 1:
      Data = 0x80;
      break;

    case 4:
      Data = 0x5A;
      break;

    case 8:
      Data = 0xAA55;
      break;

    default:
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
  }

  CopyMem (CardData->AlignedBuffer, &Data, Width);
  Status  = SendCommand (
              SdHostIo,
              BUSTEST_W,
              0,
              OutData,
              CardData->AlignedBuffer,
              Width,
              ResponseR1,
              TIMEOUT_COMMAND,
              (UINT32*)&(CardData->CardStatus)
              );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  Data = 0;
  gBS->Stall (1000);

  Status  = SendCommand (
              SdHostIo,
              BUSTEST_R,
              0,
              InData,
              CardData->AlignedBuffer,
              Width,
              ResponseR1,
              TIMEOUT_COMMAND,
              (UINT32*)&(CardData->CardStatus)
              );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }
  CopyMem (&Data, CardData->AlignedBuffer, Width);

  switch (Width) {
    case 1:
      Value = (~(Data ^ 0x80)) & 0xC0;
      break;
    case 4:
      Value = (~(Data ^ 0x5A)) & 0xFF;
      break;
    case 8:
      Value = (~(Data ^ 0xAA55)) & 0xFFFF;
      break;
  }

  if (Value == 0) {
    Status = EFI_SUCCESS;
  } else {
    Status = EFI_UNSUPPORTED;
  }

Exit:
  return Status;
}

/**
  This function can detect these card types
    1. MMC card
    2. SD 1.1 card
    3. SD 2.0 standard card
    3. SD 2.0 high capacity card

  @param  CardData                Pointer to CARD_DATA.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_UNSUPPORTED         Command unsuported.

**/
EFI_STATUS
GetCardType (
  IN  CARD_DATA              *CardData
  )
{
  EFI_STATUS                 Status;
  EFI_SD_HOST_IO_PROTOCOL    *SdHostIo;
  UINT32                     Argument;
  UINT32                     ResponseData;
  UINT32                     Count;
  BOOLEAN                    SDCommand8Support = FALSE;
  UINT32                     TimeOut=5000;

  SdHostIo = CardData->SdHostIo;
  //
  // To bring back the normal MMC card to work
  // after sending the SD command. Otherwise some
  // card could not work
  Status  = SendCommand (
              SdHostIo,
              GO_IDLE_STATE,
              0,
              NoData,
              NULL,
              0,
              ResponseNo,
              TIMEOUT_COMMAND,
              NULL
              );
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "GO_IDLE_STATE Fail Status = 0x%x\n", Status));
  }

  //
  //No spec requirment, can be adjusted
  //
  gBS->Stall (10 * 1000);

  //
  // Only 2.7V - 3.6V is supported for SD2.0, only SD 2.0 card can pass
  // MMC and SD1.1 card will fail this command
  //
  Argument          = (VOLTAGE_27_36 << 8) | CHECK_PATTERN;
  ResponseData      = 0;

  Status  = SendCommand (
              SdHostIo,
              SEND_IF_COND,
              Argument,
              NoData,
              NULL,
              0,
              ResponseR7,
              TIMEOUT_COMMAND,
              &ResponseData
              );
  if (EFI_ERROR (Status)) {
    gBS->Stall (10 * 1000); // eMMC initalization will fail without this
    DEBUG((EFI_D_ERROR, "SEND_IF_COND -> %r\n", Status));
    if (Status != EFI_TIMEOUT) {
      goto Exit;
    }
  } else {
     if (ResponseData != Argument) {
       DEBUG((EFI_D_ERROR, "SEND_IF_COND Fail, respond data does not match send data\n"));
       Status = EFI_DEVICE_ERROR;
       goto Exit;
    }
    SDCommand8Support = TRUE;
  }

  Argument = 0;
      if (SdHostIo->HostCapability.V30Support == TRUE) {
    Argument |= BIT17 | BIT18;
      } else if (SdHostIo->HostCapability.V33Support == TRUE) {
    Argument |= BIT20 | BIT21;
  }

  if (SDCommand8Support) {
    //
    //If command SD_SEND_OP_COND sucessed, it should be set.
    // SD 1.1 card will ignore it
    // SD 2.0 standard card will repsond with CCS 0, SD high capacity card will respond with CCS 1
    // CCS is BIT30 of OCR
    Argument |= BIT30;
  }

  Count = 100;
  //
  //Only SD card will respond to this command, and spec says the card only checks condition at first ACMD41 command
  //
  do {
    DEBUG ((EFI_D_ERROR, "Argument = %8x.\n", Argument));
    Status  = SendAppCommand (
                CardData,
                SD_SEND_OP_COND,
                Argument,
                NoData,
                NULL,
                0,
                ResponseR3,
                TIMEOUT_COMMAND,
                (UINT32*)&(CardData->OCRRegister)
                );
    if (EFI_ERROR (Status)) {
      if ((Status == EFI_TIMEOUT) && (!SDCommand8Support)) {
        DEBUG((EFI_D_ERROR, "Card Type: MMC\n"));
        CardData->CardType = MMCCard;
        Status = EFI_SUCCESS;
        break;
      } else {
        //
        // Not as expected, MMC card should has no response, which means timeout.
        // SD card should pass this command
        //
        DEBUG((EFI_D_ERROR, "SD_SEND_OP_COND Fail, check whether it is neither a MMC card nor a SD card\n"));
      }
      goto Exit;
    }

    gBS->Stall (50 * 1000);
    Count--;
    if (Count == 0) {
      DEBUG((EFI_D_ERROR, "Card is always in busy state\n"));
      Status = EFI_TIMEOUT;
      goto Exit;
    }
  } while (CardData->OCRRegister.Busy != 1);


  if (CardData->CardType == MMCCard) {
    CardData->CardType = MMCCard;
    SdHostIo->SetupDevice (SdHostIo);
    //
    // To bring back the normal MMC card to work
    // after sending the SD command. Otherwise some
    // card could not work

    Status  = SendCommand (
                SdHostIo,
                GO_IDLE_STATE,
                0,
                NoData,
                NULL,
                0,
                ResponseNo,
                TIMEOUT_COMMAND,
                NULL
                );

    DEBUG((EFI_D_ERROR, "Check OCR register for busy 0x%x\n",*((UINT32*)&CardData->OCRRegister )));

    gBS->Stall (1000);

    while (CardData->OCRRegister.Busy != 1){
      CardData->OCRRegister.AccessMode = 2;
      Status  = SendCommand (
                  SdHostIo,
                  SEND_OP_COND,
                  *(UINT32*)&(CardData->OCRRegister),
                  NoData,
                  NULL,
                  0,
                  ResponseR3,
                  TIMEOUT_COMMAND,
                  (UINT32*)&(CardData->OCRRegister)
                  );
      if (EFI_ERROR (Status)) {
        DEBUG((EFI_D_ERROR, "SEND_OP_COND Fail Status = 0x%x\n", Status));
        goto Exit;
      }
      gBS->Stall (1000);
      TimeOut --;
      if (TimeOut == 0) {
        DEBUG((EFI_D_ERROR, "Card is always in busy state\n"));
        Status = EFI_TIMEOUT;
        goto Exit;
      }
    }
  }

  if (CardData->CardType != MMCCard) {
    //
    //Check supported voltage
    //
    Argument = 0;
  if (SdHostIo->HostCapability.V30Support == TRUE) {
      if ((CardData->OCRRegister.V270_V360 & BIT2) == BIT2) {
        Argument |= BIT17;
      } else if ((CardData->OCRRegister.V270_V360 & BIT3) == BIT3) {
        Argument |= BIT18;
      }
  } else if (SdHostIo->HostCapability.V33Support == TRUE) {
       if ((CardData->OCRRegister.V270_V360 & BIT5) == BIT5) {
         Argument |= BIT20;
       } else if ((CardData->OCRRegister.V270_V360 & BIT6) == BIT6) {
         Argument |= BIT21;
       }
    }

    if (Argument == 0) {
      //
      //No matched support voltage
      //
      PutCardInactive (CardData);
      DEBUG((EFI_D_ERROR, "No matched voltage for this card\n"));
      Status = EFI_UNSUPPORTED;
      goto Exit;
    }

    CardData->CardType = SDMemoryCard;
    if (SDCommand8Support == TRUE) {
      CardData->CardType = SDMemoryCard2;
    }

    if ((CardData->OCRRegister.AccessMode & BIT1) == BIT1) {
      CardData->CardType = SDMemoryCard2High;
    }
  }

Exit:
  return Status;
}

/**
  MMC card high/low voltage selection function.

  @param  CardData                Pointer to CARD_DATA.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_UNSUPPORTED         Command unsuported.

**/
EFI_STATUS
MmcCardVoltageSelection (
  IN  CARD_DATA              *CardData
  )
{
  EFI_STATUS                 Status;
  EFI_SD_HOST_IO_PROTOCOL    *SdHostIo;
  UINT8                      Index;
  UINT8                      Retry;
  UINT32                     TimeOut;

  SdHostIo = CardData->SdHostIo;
  Status   = EFI_SUCCESS;
  if (CardData->CardType != MMCCard) {
  //
  //First try the high voltage, then if supported choose the low voltage
  //
  for (Index = 0; Index < 2; Index++) {

    for (Retry = 0; Retry < 3; Retry++) {
      //
      // To bring back the normal MMC card to work
      // after sending the SD command. Otherwise some
      // card could not work

      Status  = SendCommand (
                  SdHostIo,
                  GO_IDLE_STATE,
                  0,
                  NoData,
                  NULL,
                  0,
                  ResponseNo,
                  TIMEOUT_COMMAND,
                  NULL
                  );
      if (EFI_ERROR (Status)) {
        DEBUG((EFI_D_ERROR, "GO_IDLE_STATE Fail Status = 0x%x\n", Status));
        continue;
      }
      //
      //CE-ATA device needs long delay
      //
      gBS->Stall ((Retry + 1) * 5 * 1000);

      //
      //Get OCR register to check voltage support, first time the OCR is 0
      //
      Status  = SendCommand (
                  SdHostIo,
                  SEND_OP_COND,
                  0,
                  NoData,
                  NULL,
                  0,
                  ResponseR3,
                  TIMEOUT_COMMAND,
                  (UINT32*)&(CardData->OCRRegister)
                  );
      if (!EFI_ERROR (Status)) {
        break;
      }
    }

    if (Retry == 3) {
      DEBUG((EFI_D_ERROR, "SEND_OP_COND Fail Status = 0x%x\n", Status));
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    if (CardData->OCRRegister.V170_V195 == 1) {
      CardData->DualVoltage = TRUE;
    }
    if (CardData->OCRRegister.V270_V360 != 0x1F &&
        CardData->OCRRegister.V200_V260 != 0) {
      DEBUG((EFI_D_ERROR, "Incompatiable voltage device\n"));
      PutCardInactive (CardData);
      Status = EFI_INCOMPATIBLE_VERSION;
      goto Exit;
    }

    if (Index == 0) {
      //
      //Choose the high voltage first
      //
      CardData->OCRRegister.V170_V195 = 0;
    } else {
      //
      //Choose the low voltage
      //
      CardData->OCRRegister.V170_V195 = 1;
      CardData->OCRRegister.V270_V360 = 0;
    }

        //
        // Set sector mode
        //
        CardData->OCRRegister.AccessMode |= 2;
    
        //
    //TimeOut Value, 5000 * 100 * 1000 = 5 s
    //
    TimeOut = 5000;

    do {
      Status  = SendCommand (
                  SdHostIo,
                  SEND_OP_COND,
                  *(UINT32*)&(CardData->OCRRegister),
                  NoData,
                  NULL,
                  0,
                  ResponseR3,
                  TIMEOUT_COMMAND,
                  (UINT32*)&(CardData->OCRRegister)
                  );
      if (EFI_ERROR (Status)) {
        DEBUG((EFI_D_ERROR, "SEND_OP_COND Fail Status = 0x%x\n", Status));
        goto Exit;
      }

      gBS->Stall (1 * 1000);
      TimeOut--;
      if (TimeOut == 0) {
        DEBUG((EFI_D_ERROR, "Card is always in busy state\n"));
        Status = EFI_TIMEOUT;
        goto Exit;
      }
    } while (CardData->OCRRegister.Busy != 1);

  if (CardData->DualVoltage == TRUE && SdHostIo->HostCapability.V18Support == TRUE) {
      //
      //Power Off the card and then power on into low voltage
      //
      SdHostIo->SetHostVoltage (SdHostIo, 0);
      gBS->Stall (1 * 1000);
      SdHostIo->SetHostVoltage (SdHostIo, 18);
    } else {
      //
      //Not support the low voltage, exit
      //
      break;
    }
  }
  }
Exit:
  return Status;

}

/**
  This function set the bus and device width for MMC card.

  @param  CardData                Pointer to CARD_DATA.
  @param  Width                   1, 4, 8 bits.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_UNSUPPORTED         Command unsuported.

**/
EFI_STATUS
MmcCardSetBusWidth (
  IN  CARD_DATA              *CardData,
  IN  UINT8                  BusWidth
  )
{
  EFI_STATUS                 Status;
  EFI_SD_HOST_IO_PROTOCOL    *SdHostIo;
  SWITCH_ARGUMENT            SwitchArgument;
  UINT8                      Value;

  SdHostIo = CardData->SdHostIo;
  Value = 0;
  switch (BusWidth) {
    case 28:	//20 in 28 indicates DDR in 8 bit bus
      Value = 6;
      break;
          
    case 24:    //20 in 24 indicates DDR in 4 bit bus
      Value = 5;
      break;

    case 8:
      Value = 2;
      break;

    case 4:
      Value = 1;
      break;

    case 1:
      Value = 0;
      break;

    default:
     ASSERT(0);
  }

  ZeroMem(&SwitchArgument, sizeof (SWITCH_ARGUMENT));
  SwitchArgument.CmdSet = 0;
  SwitchArgument.Value  = Value;
  SwitchArgument.Index  = (UINT32)((UINTN)
  (&(CardData->ExtCSDRegister.BUS_WIDTH)) - (UINTN)(&(CardData->ExtCSDRegister)));
  SwitchArgument.Access = WriteByte_Mode;
  Status  = SendCommand (
              SdHostIo,
              SWITCH,
              *(UINT32*)&SwitchArgument,
              NoData,
              NULL,
              0,
              ResponseR1b,
              TIMEOUT_COMMAND,
              (UINT32*)&(CardData->CardStatus)
              );
  if (!EFI_ERROR (Status)) {
     Status  = SendCommand (
                 SdHostIo,
                 SEND_STATUS,
                 (CardData->Address << 16),
                 NoData,
                 NULL,
                 0,
                 ResponseR1,
                 TIMEOUT_COMMAND,
                 (UINT32*)&(CardData->CardStatus)
                 );
    if (EFI_ERROR (Status)) {
      DEBUG((EFI_D_ERROR, "SWITCH %d bits Fail\n", BusWidth));
      goto Exit;
    } else {
      if ((BusWidth == 24) || (BusWidth == 28)){
        Status = SdHostIo->SetBusWidth (SdHostIo, BusWidth - 20);
      } else {
        Status = SdHostIo->SetBusWidth (SdHostIo, BusWidth);
      }
      if (EFI_ERROR (Status)) {
         goto Exit;
      }
      gBS->Stall (5 * 1000);
    }
  }
  if ((BusWidth == 24) || (BusWidth == 28)) {
    //Status = MmcCardBusWidthTest (CardData, BusWidth - 20); //Illegal commands
    goto Exit;
  } else {
    Status = MmcCardBusWidthTest (CardData, BusWidth);
  }
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "MMCCardBusWidthTest %d bit Fail\n", BusWidth));
    goto Exit;
  }

  CardData->CurrentBusWidth = BusWidth;

Exit:
  return Status;
}

/**
  MMC/SD card init function.

  @param  CardData                Pointer to CARD_DATA.

  @retval EFI_INVALID_PARAMETER   Parameter is error.
  @retval EFI_SUCCESS             Success.
  @retval EFI_UNSUPPORTED         Command unsuported.
  @retval EFI_BAD_BUFFER_SIZE     Buffer size is bad.
  
**/
EFI_STATUS
MmcSdCardInit (
  IN  CARD_DATA              *CardData
  )
{
  EFI_STATUS                 Status;
  EFI_SD_HOST_IO_PROTOCOL    *SdHostIo;
  SWITCH_ARGUMENT            SwitchArgument;
  UINT32                     Data;
  UINT32                     Argument;
  UINT8                      PowerValue;
  UINT8                      DoubleSpeed;

  ASSERT(CardData != NULL);
  SdHostIo                  = CardData->SdHostIo;
  CardData->CurrentBusWidth = 1;

  Status = GetCardType (CardData);
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "GetCardType -> %r\n", Status));
    goto Exit;
  }

  ASSERT (CardData->CardType != UnknownCard);
  //
  //MMC, SD card need host auto stop command support
  //
  SdHostIo->EnableAutoStopCmd (SdHostIo, TRUE);
  if (CardData->CardType == MMCCard) {
    Status = MmcCardVoltageSelection (CardData);
    if (EFI_ERROR(Status)) {
      DEBUG((EFI_D_ERROR, "MMCCardVoltageSelection -> %r\n", Status));
      goto Exit;
    }
  }
  //
  // Get CID Register, but the info is not used currently
  //
  Status  = SendCommand (
              SdHostIo,
              ALL_SEND_CID,
              0,
              NoData,
              NULL,
              0,
              ResponseR2,
              TIMEOUT_COMMAND,
              (UINT32*)&(CardData->CIDRegister)
              );
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "ALL_SEND_CID -> %r\n", Status));
    goto Exit;
  }

  //
  //SET_RELATIVE_ADDR
  //
  if (CardData->CardType == MMCCard) {
    CardData->Address = 1;

    //
    // Set RCA Register
    //
    Status  = SendCommand (
                SdHostIo,
                SET_RELATIVE_ADDR,
                (CardData->Address << 16),
                NoData,
                NULL,
                0,
                ResponseR1,
                TIMEOUT_COMMAND,
                (UINT32*)&(CardData->CardStatus)
                );
    if (EFI_ERROR (Status)) {
      DEBUG((EFI_D_ERROR, "SET_RELATIVE_ADDR -> %r\n", Status));
      goto Exit;
    }
  } else {
    Data = 0;
    Status  = SendCommand (
                SdHostIo,
                SET_RELATIVE_ADDR,
                0,
                NoData,
                NULL,
                0,
                ResponseR6,
                TIMEOUT_COMMAND,
                &Data
                );
    if (EFI_ERROR (Status)) {
      DEBUG((EFI_D_ERROR, "SET_RELATIVE_ADDR -> %r\n", Status));
      goto Exit;
    }

    CardData->Address = (UINT16)(Data >> 16);
    *(UINT32*)&CardData->CardStatus      = Data & 0x1FFF;
    CardData->CardStatus.ERROR           = (Data >> 13) & 0x1;
    CardData->CardStatus.ILLEGAL_COMMAND = (Data >> 14) & 0x1;
    CardData->CardStatus.COM_CRC_ERROR   = (Data >> 15) & 0x1;
    Status = CheckCardStatus (*(UINT32*)&CardData->CardStatus);
    if (EFI_ERROR (Status)) {
      DEBUG((EFI_D_ERROR, "SET_RELATIVE_ADDR -> %r\n", Status));
      goto Exit;
    }
  }

  //
  // Get CSD Register
  //
  Status  = SendCommand (
              SdHostIo,
              SEND_CSD,
              (CardData->Address << 16),
              NoData,
              NULL,
              0,
              ResponseR2,
              TIMEOUT_COMMAND,
              (UINT32*)&(CardData->CSDRegister)
              );
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "SEND_CSD -> %r\n", Status));
    goto Exit;
  }
  
  MmcDecodeCSD (&CardData->CSDRegister);

  Status = CaculateCardParameter (CardData);
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "CalculateCardParameter -> %r\n", Status));
    goto Exit;
  }

  //
  // It is platform and hardware specific, need hadrware engineer input
  //
  if (CardData->CSDRegister.DSR_IMP == 1) {
    //
    // Default is 0x404
    //
    Status  = SendCommand (
                SdHostIo,
                SET_DSR,
                (DEFAULT_DSR_VALUE << 16),
                NoData,
                NULL,
                0,
                ResponseNo,
                TIMEOUT_COMMAND,
                NULL
                );
    if (EFI_ERROR (Status)) {
      DEBUG((EFI_D_ERROR, "SET_DSR -> %r\n", Status));
      //
      // Assume can operate even fail
      //
    }
  }

  //
  //Change clock frequency from 400KHz to max supported
  //
  Status = SdHostIo->SetClockFrequency (SdHostIo, CardData->MaxFrequency);

  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "SetClockFrequency -> %r\n", Status));
    goto Exit;
  }

  //
  //Put the card into tran state
  //
  Status = SendCommand (
              SdHostIo,
              SELECT_DESELECT_CARD,
              (CardData->Address << 16),
              NoData,
              NULL,
              0,
              ResponseR1,
              TIMEOUT_COMMAND,
              (UINT32*)&(CardData->CardStatus)
              );
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "SELECT_DESELECT_CARD -> %r\n", Status));
    goto Exit;
  }

  Status  = SendCommand (
              SdHostIo,
              SEND_STATUS,
              (CardData->Address << 16),
              NoData,
              NULL,
              0,
              ResponseR1,
              TIMEOUT_COMMAND,
              (UINT32*)&(CardData->CardStatus)
              );
  if (EFI_ERROR (Status)) {
    DEBUG((EFI_D_ERROR, "SELECT_DESELECT_CARD SEND_STATUS Fail Status = 0x%x\n", Status));
    goto Exit;
  }

  if (CardData->CardType == MMCCard) {
    //
    //Only V4.0 and above supports more than 1 bits and high speed
    //
    if (CardData->CSDRegister.SPEC_VERS >= 4) {
      //
      //Get ExtCSDRegister
      //
      Status  = SendCommand (
                  SdHostIo,
                  SEND_EXT_CSD,
                  0,   // stuff bits are 0
                  InData,
                  CardData->AlignedBuffer,
                  sizeof (EXT_CSD),
                  ResponseR1,
                  TIMEOUT_DATA,
                  (UINT32*)&(CardData->CardStatus)
                  );
      if (EFI_ERROR (Status)) {
        DEBUG((EFI_D_ERROR, "SEND_EXT_CSD -> %r\n", Status));
        goto Exit;
      }

      CopyMem (&(CardData->ExtCSDRegister), CardData->AlignedBuffer, sizeof (EXT_CSD));
      MmcDecodeExtCSD(&CardData->ExtCSDRegister);

      //
      // Recaculate the block number for >2G MMC card
      //
      Data  = (CardData->ExtCSDRegister.SEC_COUNT[0]) |
              (CardData->ExtCSDRegister.SEC_COUNT[1] << 8) |
              (CardData->ExtCSDRegister.SEC_COUNT[2] << 16) |
              (CardData->ExtCSDRegister.SEC_COUNT[3] << 24);

      if (Data != 0) {
        CardData->BlockNumber = Data;
      }
      DEBUG((DEBUG_INFO, "CardData->BlockNumber  %d\n", Data));
      //
      // Check the DDR setting
      //
      DoubleSpeed = 0 ;
      DEBUG((EFI_D_ERROR, "CardData->ExtCSDRegister.CARD_TYPE -> %d\n", (UINTN)CardData->ExtCSDRegister.CARD_TYPE));
      if ((CardData->ExtCSDRegister.CARD_TYPE & BIT2)||
          (CardData->ExtCSDRegister.CARD_TYPE & BIT3)) {
        DEBUG((DEBUG_INFO, "Card support DDR\n"));
        DoubleSpeed = 20;   //Add 20 for double speed, decoded in MMCCardSetBusWidth()
      }

      if (SdHostIo->HostCapability.HighSpeedSupport) {

        //
        //Change to high frequency mode
        //
        ZeroMem(&SwitchArgument, sizeof (SWITCH_ARGUMENT));
        SwitchArgument.CmdSet = 0;
        SwitchArgument.Value  = 1;
        SwitchArgument.Index  = (UINT32)((UINTN)
        (&(CardData->ExtCSDRegister.HS_TIMING)) - (UINTN)(&(CardData->ExtCSDRegister)));
        SwitchArgument.Access = WriteByte_Mode;
        Status  = SendCommand (
                    CardData->SdHostIo,
                    SWITCH,
                    *(UINT32*)&SwitchArgument,
                    NoData,
                    NULL,
                    0,
                    ResponseR1b,
                    TIMEOUT_COMMAND,
                    (UINT32*)&(CardData->CardStatus)
                    );
        if (EFI_ERROR (Status)) {
          DEBUG((EFI_D_ERROR, "SWITCH frequency -> %r\n", Status));
        }

        if (!EFI_ERROR (Status)) {
          Status  = SendCommand (
                      SdHostIo,
                      SEND_STATUS,
                      (CardData->Address << 16),
                      NoData,
                      NULL,
                      0,
                      ResponseR1,
                      TIMEOUT_COMMAND,
                      (UINT32*)&(CardData->CardStatus)
                      );
          if (!EFI_ERROR (Status)) {
             //
             // Enable the high speed mode
             // 

             if (DoubleSpeed != 0) {
                DEBUG((EFI_D_ERROR, "Set to DDR50 mode \n", Status));
                Status = SdHostIo->SetHostDdrMode(SdHostIo, TRUE);
                SetSdhcBackDoorReg(SdHostIo, 0x185, 0x34);
             } else  {
                DEBUG((EFI_D_ERROR, "Set to HS mode \n", Status));
                SdHostIo->SetHostSpeedMode (SdHostIo, 1);
                SetSdhcBackDoorReg(SdHostIo, 0x185, 0x81);
             }
            //
            //Change host clock
            //
            if ((CardData->ExtCSDRegister.CARD_TYPE & BIT1) != 0) {
              Status = SdHostIo->SetClockFrequency (SdHostIo, FREQUENCY_MMC_PP_HIGH);
            } else if ((CardData->ExtCSDRegister.CARD_TYPE & BIT0) != 0) {
              Status = SdHostIo->SetClockFrequency (SdHostIo, FREQUENCY_MMC_PP);
            } else {
              Status = EFI_UNSUPPORTED;
            }
            if (EFI_ERROR (Status)) {
              goto Exit;
            }
            //
            // It seems no need to stall after changing bus freqeuncy.
            // It is said that the freqeuncy can be changed at any time. Just appends 8 clocks after command.
            // But SetClock alreay has delay.
            //
          }
        }
      }

      //
      // Prefer wide bus width for performance
      //
      //
      // Set to BusWidth bits mode, only version 4.0 or above support more than 1 bits
      //
      if (SdHostIo->HostCapability.BusWidth8 == 1) {         
         Status = MmcCardSetBusWidth (CardData, DoubleSpeed + 8);
         if (EFI_ERROR (Status)) {
            //
            // CE-ATA may support 8 bits and 4 bits, but has no software method for detection
            //
            Status = MmcCardSetBusWidth (CardData, DoubleSpeed + 4);
            if (EFI_ERROR (Status)) {
              goto Exit;
            }
         }
      } else if (SdHostIo->HostCapability.BusWidth4 == 1) {
         Status = MmcCardSetBusWidth (CardData, DoubleSpeed + 4);
         if (EFI_ERROR (Status)) {
           goto Exit;
         }
      }

      PowerValue = 0;

      if (CardData->CurrentBusWidth == 8) {
        if ((CardData->ExtCSDRegister.CARD_TYPE & BIT1) != 0) {
          PowerValue = CardData->ExtCSDRegister.PWR_CL_52_360;
          PowerValue = PowerValue >> 4;
        } else if ((CardData->ExtCSDRegister.CARD_TYPE & BIT0) != 0) {
          PowerValue = CardData->ExtCSDRegister.PWR_CL_26_360;
          PowerValue = PowerValue >> 4;
        }
      } else if (CardData->CurrentBusWidth == 4) {
         if ((CardData->ExtCSDRegister.CARD_TYPE & BIT1) != 0) {
          PowerValue = CardData->ExtCSDRegister.PWR_CL_52_360;
          PowerValue = PowerValue & 0xF;
         } else if ((CardData->ExtCSDRegister.CARD_TYPE & BIT0) != 0) {
           PowerValue = CardData->ExtCSDRegister.PWR_CL_26_360;
           PowerValue = PowerValue & 0xF;
         }
      }

      if (PowerValue != 0) {
        //
        //Update Power Class
        //
        ZeroMem(&SwitchArgument, sizeof (SWITCH_ARGUMENT));
        SwitchArgument.CmdSet = 0;
        SwitchArgument.Value  = PowerValue;
        SwitchArgument.Index  = (UINT32)((UINTN)
        (&(CardData->ExtCSDRegister.POWER_CLASS)) - (UINTN)(&(CardData->ExtCSDRegister)));
        SwitchArgument.Access = WriteByte_Mode;
        Status  = SendCommand (
                    SdHostIo,
                    SWITCH,
                    *(UINT32*)&SwitchArgument,
                    NoData,
                    NULL,
                    0,
                    ResponseR1b,
                    TIMEOUT_COMMAND,
                    (UINT32*)&(CardData->CardStatus)
                    );
         if (!EFI_ERROR (Status)) {
           Status  = SendCommand (
                       SdHostIo,
                       SEND_STATUS,
                       (CardData->Address << 16),
                       NoData,
                       NULL,
                       0,
                       ResponseR1,
                       TIMEOUT_COMMAND,
                       (UINT32*)&(CardData->CardStatus)
                       );
           if (EFI_ERROR (Status)) {
             DEBUG((EFI_D_ERROR, "SWITCH Power Class -> %r\n", Status));
           }
           //gBS->Stall (10 * 1000);
         }
      }
    } else {
      DEBUG((EFI_D_ERROR, "MMC Card version %d only supportes 1 bits at lower transfer speed\n",CardData->CSDRegister.SPEC_VERS));
    }
  } else {
      //
      // Pin 1, at power up this line has a 50KOhm pull up enabled in the card.
      // This pull-up should be disconnected by the user, during regular data transfer,
      // with SET_CLR_CARD_DETECT (ACMD42) command
      //
      Status  = SendAppCommand (
                  CardData,
                  SET_CLR_CARD_DETECT,
                  0,
                  NoData,
                  NULL,
                  0,
                  ResponseR1,
                  TIMEOUT_COMMAND,
                  (UINT32*)&(CardData->CardStatus)
                  );
      if (EFI_ERROR (Status)) {
        DEBUG((EFI_D_ERROR, "SET_CLR_CARD_DETECT Fail -> %r\n", Status));
        goto Exit;
      }
      //
      // Set Bus Width to 4
      //
      Status  = SendAppCommand (
                  CardData,
                  SET_BUS_WIDTH,
                  SD_BUS_WIDTH_4,
                  NoData,
                  NULL,
                  0,
                  ResponseR1,
                  TIMEOUT_COMMAND,
                  (UINT32*)&(CardData->CardStatus)
                  );
      if (EFI_ERROR (Status)) {
        DEBUG((EFI_D_ERROR, "SET_BUS_WIDTH 4 bits -> %r\n", Status));
        goto Exit;
      }

      Status = SdHostIo->SetBusWidth (SdHostIo, 4);
      if (EFI_ERROR (Status)) {
        goto Exit;
      }
      CardData->CurrentBusWidth = 4;

      
      if ((SdHostIo->HostCapability.HighSpeedSupport == FALSE) ||
          ((CardData->CSDRegister.CCC & BIT10) != BIT10)) {
        //
        // Host must support high speed
        // Card must support Switch function
        //
        goto Exit;
      }

      //
      //Mode = 0, group 1, function 1, check operation
      //
      Argument    = 0xFFFF01;
      ZeroMem (&CardData->SwitchStatus, sizeof (SWITCH_STATUS));

      Status  = SendCommand (
                  SdHostIo,
                  SWITCH_FUNC,
                  Argument,
                  InData,
                  CardData->AlignedBuffer,
                  sizeof (SWITCH_STATUS),
                  ResponseR1,
                  TIMEOUT_COMMAND,
                  (UINT32*)&(CardData->CardStatus)
                  );
      if (EFI_ERROR (Status)) {
        goto Exit;
      }
      CopyMem (&(CardData->SwitchStatus), CardData->AlignedBuffer, sizeof (SWITCH_STATUS));

      if ((CardData->SwitchStatus.DataStructureVersion == 0x0) ||
          ((CardData->SwitchStatus.Group1BusyStatus & BIT1) != BIT1)) {
        //
        // 1. SD 1.1 card does not suppport busy bit
        // 2. Ready state
        //
        //

        //
        //Mode = 1, group 1, function 1, BIT31 set means set mode
        //
        Argument = 0xFFFF01 | BIT31;
        ZeroMem (&CardData->SwitchStatus, sizeof (SWITCH_STATUS));

        Status  = SendCommand (
                    SdHostIo,
                    SWITCH_FUNC,
                    Argument,
                    InData,
                    CardData->AlignedBuffer,
                    sizeof (SWITCH_STATUS),
                    ResponseR1,
                    TIMEOUT_COMMAND,
                   (UINT32*)&(CardData->CardStatus)
                   );
         if (EFI_ERROR (Status)) {
            goto Exit;
         }
         CopyMem (&(CardData->SwitchStatus), CardData->AlignedBuffer, sizeof (SWITCH_STATUS));

         if ((CardData->SwitchStatus.DataStructureVersion == 0x0) ||
            ((CardData->SwitchStatus.Group1BusyStatus & BIT1) != BIT1)) {
          //
          // 1. SD 1.1 card does not suppport busy bit
          // 2. Ready state
          //

          //
          // 8 clocks, (1/ 25M) * 8 ==> 320 us, so 1ms > 0.32 ms
          //
          gBS->Stall (1000);

         }
      }
  }

  if (!((CardData->ExtCSDRegister.CARD_TYPE & BIT2) ||
        (CardData->ExtCSDRegister.CARD_TYPE & BIT3))) {
    //
    // Set Block Length, to improve compatibility in case of some cards
    //
    Status  = SendCommand (
                  SdHostIo,
                SET_BLOCKLEN,
                512,
                NoData,
                NULL,
                0,
                ResponseR1,
                TIMEOUT_COMMAND,
                (UINT32*)&(CardData->CardStatus)
                );
    if (EFI_ERROR (Status)) {
      DEBUG((EFI_D_ERROR, "SET_BLOCKLEN -> %r\n", Status));
      goto Exit;
    }
  }
  SdHostIo->SetBlockLength (SdHostIo, 512);

Exit:
  return Status;
}


EFI_STATUS
MmcSelect (
  IN CARD_DATA     *CardData,
  IN BOOLEAN       Select
  )
{
  return SendCommand (
           CardData->SdHostIo,
           SELECT_DESELECT_CARD,
           Select ? (CardData->Address << 16) : ~(CardData->Address << 16),
           NoData,
           NULL,
           0,
           ResponseR1,
           TIMEOUT_COMMAND,
           (UINT32*)&(CardData->CardStatus)
           );
}


EFI_STATUS
MmcSendSwitch (
  IN  CARD_DATA              *CardData,
  IN SWITCH_ARGUMENT         *SwitchArgument
  )
{
  EFI_STATUS                 Status;
  EFI_SD_HOST_IO_PROTOCOL    *SdHostIo;

  SdHostIo = CardData->SdHostIo;

  Status  = SendCommand (
              SdHostIo,
              SWITCH,
              *(UINT32*)SwitchArgument,
              NoData,
              NULL,
              0,
              ResponseR1b,
              TIMEOUT_DATA,
              (UINT32*)&(CardData->CardStatus)
              );
  if (!EFI_ERROR (Status)) {
     Status  = SendCommand (
                 SdHostIo,
                 SEND_STATUS,
                 (CardData->Address << 16),
                 NoData,
                 NULL,
                 0,
                 ResponseR1,
                 TIMEOUT_COMMAND,
                 (UINT32*)&(CardData->CardStatus)
                 );
    if (EFI_ERROR (Status)) {
      DEBUG((EFI_D_ERROR, "SWITCH FAILURE\n"));
    }
  }

  return Status;
}


EFI_STATUS
MmcUpdateCardStatus (
  IN CARD_DATA     *CardData
  )
{
  return SendCommand (
           CardData->SdHostIo,
           SEND_STATUS,
           (CardData->Address << 16),
           NoData,
           NULL,
           0,
           ResponseR1,
           TIMEOUT_COMMAND,
           (UINT32*)&(CardData->CardStatus)
           );
}


EFI_STATUS
MmcMoveToTranState (
  IN CARD_DATA     *CardData
  )
{
  EFI_STATUS Status;

  Status = EFI_SUCCESS;

  if (CardData->CardStatus.CURRENT_STATE != Tran_STATE) {
    //
    // Put the card into tran state
    //
    Status = MmcSelect (CardData, TRUE);
    DEBUG((EFI_D_INFO, "MmcMoveToTranState: CMD7 -> %r\n", Status));
    MmcUpdateCardStatus (CardData);
  }

  if (CardData->CardStatus.CURRENT_STATE != Tran_STATE) {
    DEBUG((EFI_D_ERROR, "MmcMoveToTranState: Unable to put card into tran state\n"));
    return EFI_DEVICE_ERROR;
  }

  return Status;
}


EFI_STATUS
MmcReadExtCsd (
  IN CARD_DATA     *CardData
  )
{
  EFI_STATUS Status;

  Status = MmcMoveToTranState (CardData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status  = SendCommand (
              CardData->SdHostIo,
              SEND_EXT_CSD,
              (CardData->Address << 16),
              InData,
              CardData->AlignedBuffer,
              sizeof (EXT_CSD),
              ResponseR1,
              TIMEOUT_DATA,
              (UINT32*)&(CardData->CardStatus)
              );
  DEBUG ((EFI_D_INFO, "MmcReadExtCsd: SEND_EXT_CSD -> %r\n", Status));
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  CopyMem (&(CardData->ExtCSDRegister), CardData->AlignedBuffer, sizeof (EXT_CSD));

  return Status;
}


EFI_STATUS
MmcSetExtCsd8 (
  IN  CARD_DATA              *CardData,
  IN  UINT8                  Index,
  IN  UINT8                  Value
  )
{
  EFI_STATUS                 Status;
  SWITCH_ARGUMENT            SwitchArgument;

  Status = MmcMoveToTranState (CardData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ZeroMem(&SwitchArgument, sizeof (SWITCH_ARGUMENT));
  SwitchArgument.CmdSet = 0;
  SwitchArgument.Value  = (UINT8) Value;
  SwitchArgument.Index  = (UINT8) Index;
  SwitchArgument.Access = WriteByte_Mode; // SetBits_Mode;
  return MmcSendSwitch (CardData, &SwitchArgument);
}

UINT32
MmcGetExtCsd8 (
  IN CARD_DATA                        *CardData,
  IN UINTN                            Offset
  )
{
  ASSERT (Offset < sizeof (CardData->ExtCSDRegister));
  return ((UINT8*)&CardData->ExtCSDRegister)[Offset];
}


UINT32
MmcGetExtCsd32 (
  IN CARD_DATA                        *CardData,
  IN UINTN                            Offset
  )
{
  return *(UINT32*) (((UINT8*)&CardData->ExtCSDRegister) + Offset);
}


UINT32
MmcGetExtCsd24 (
  IN CARD_DATA                        *CardData,
  IN UINTN                            Offset
  )
{
  return MmcGetExtCsd32 (CardData, Offset) & 0xffffff;
}


UINTN
MmcGetCurrentPartitionNum (
  IN  CARD_DATA              *CardData
  )
{
  return MmcGetExtCsd8 (
           CardData,
           OFFSET_OF (EXT_CSD, PARTITION_CONFIG)
           ) & 0x7;
}


EFI_STATUS
MmcSelectPartitionNum (
  IN  CARD_DATA              *CardData,
  IN  UINT8                  Partition
  )
{
  EFI_STATUS  Status;
  UINTN       Offset;
  UINT8       *ExtByte;
  UINTN       CurrentPartition;

  if (Partition > 7) {
    return EFI_INVALID_PARAMETER;
  }
  
  CurrentPartition = MmcGetCurrentPartitionNum (CardData);
  if (Partition == CurrentPartition) {
    return EFI_SUCCESS;
  }

  Status = MmcReadExtCsd (CardData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DEBUG ((EFI_D_INFO,
    "MmcSelectPartitionNum: Switch partition: %d => %d\n",
    CurrentPartition,
    Partition
    ));

  Offset = OFFSET_OF (EXT_CSD, PARTITION_CONFIG);
  Status = MmcSetExtCsd8 (CardData, (UINT8)Offset, Partition);

#if 1
  Status = MmcReadExtCsd (CardData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  CurrentPartition = MmcGetCurrentPartitionNum (CardData);
  if (Partition != CurrentPartition) {
    DEBUG ((EFI_D_INFO, "MmcSelectPartitionNum: Switch partition failed!\n"));
    return EFI_DEVICE_ERROR;
  }

  ExtByte = NULL;
#else
  if (!EFI_ERROR (Status)) {
    ExtByte = ((UINT8*)&CardData->ExtCSDRegister) + Offset;
    *ExtByte = (UINT8) ((*ExtByte & 0xF7) | Partition);
  }
#endif

  return Status;
}

EFI_STATUS
MmcSelectPartition (
  IN  MMC_PARTITION_DATA     *Partition
  )
{
  return MmcSelectPartitionNum (
           Partition->CardData,
           (UINT8)CARD_DATA_PARTITION_NUM (Partition)
           );
}

