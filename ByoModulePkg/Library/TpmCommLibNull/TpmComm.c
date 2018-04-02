/*++
  This file contains 'Framework Code' and is licensed as such   
  under the terms of your license agreement with Intel or your  
  vendor.  This file may not be modified, except as allowed by  
  additional terms of your license agreement.                   
--*/
/** @file
  Basic TPM command functions.

Copyright (c) 2005 - 2010, Intel Corporation. All rights reserved.<BR>
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.

**/

#include <Library/TpmCommLib.h>

/**
  Check whether the value of a TPM chip register satisfies the input BIT setting.

  @param  Register     Address port of register to be checked.
  @param  BitSet       Check these data bits are set.
  @param  BitClear     Check these data bits are clear.
  @param  TimeOut      The max wait time (in units of 30 MicroSeconds) to be used 
                       in checking the register.

  @retval EFI_SUCCESS  The register satisfies the check bit.
  @retval EFI_TIMEOUT  The register check timed out.
**/
EFI_STATUS
EFIAPI
TisPcWaitRegisterBits (
  IN UINT8   *Register,
  IN UINT8   BitSet,   
  IN UINT8   BitClear, 
  IN UINT32  TimeOut   
  )
{
  return EFI_SUCCESS;
}

/**
  Get BurstCount by reading the BurstCount field of a TIS register 
  in the time of default TIS_TIMEOUT_D.

  @param  TisReg      Pointer to TIS register.
  @param  BurstCount  Pointer to a buffer to store the got BurstConut.

  @retval EFI_SUCCESS           Get BurstCount.
  @retval EFI_INVALID_PARAMETER TisPeg is NULL, or BurstCount is NULL.
  @retval EFI_TIMEOUT           BurstCount could not be retrieved before timeout.
**/
EFI_STATUS
EFIAPI
TisPcReadBurstCount (
  IN  TIS_PC_REGISTERS_PTR  TisReg,
  OUT UINT16                *BurstCount
  )
{
  return EFI_SUCCESS;
}

/**
  Set TPM chip to ready state by sending ready command TIS_PC_STS_READY 
  to Status Register in time.

  @param TisReg  The pointer to the TIS register.

  @retval EFI_SUCCESS           TPM chip entered into ready state.
  @retval EFI_INVALID_PARAMETER TisPeg is NULL.
  @retval EFI_TIMEOUT           TPM chip could not be set to ready state in time.
**/
EFI_STATUS
EFIAPI
TisPcPrepareCommand (
  IN TIS_PC_REGISTERS_PTR  TisReg
  )
{
  return EFI_SUCCESS;
}

/**
  Get the control of TPM chip by sending RequestUse command TIS_PC_ACC_RQUUSE 
  to ACCESS Register in the time of default TIS_TIMEOUT_D.

  @param TisReg  Pointer to TIS register.

  @retval EFI_SUCCESS           Get the control of TPM chip.
  @retval EFI_INVALID_PARAMETER TisPeg is NULL.
  @retval EFI_NOT_FOUND         TPM chip does not exist.
  @retval EFI_TIMEOUT           Could not get the TPM control in time.
**/
EFI_STATUS
EFIAPI
TisPcRequestUseTpm (
  IN TIS_PC_REGISTERS_PTR  TisReg
  )
{
  return EFI_SUCCESS;
}

/**
  Single function calculates SHA1 digest value for all raw data. It
  combines Sha1Init(), Sha1Update() and Sha1Final().

  @param  Data          Raw data to be digested.
  @param  DataLen       Size of the raw data.
  @param  Digest        Pointer to a buffer that stores the final digest.
  
  @retval EFI_SUCCESS   Always successfully calculate the final digest.
**/
EFI_STATUS
EFIAPI
TpmCommHashAll (
  IN  CONST UINT8                   *Data,
  IN        UINTN                   DataLen,
  OUT       TPM_DIGEST              *Digest
  )
{
  return EFI_SUCCESS;
}

