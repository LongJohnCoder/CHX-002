/** @file
  Ihis library uses TPM2 device to calculation hash.

Copyright (c) 2012, Intel Corporation. All rights reserved. <BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/DebugLib.h>
#include <Library/HashLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/MemoryAllocationLib.h>




/**
  Single function calculates SHA1 digest value for all raw data. It
  combines Sha1Init(), Sha1Update() and Sha1Final().

  @param[in]  Data          Raw data to be digested.
  @param[in]  DataLen       Size of the raw data.
  @param[out] Digest        Pointer to a buffer that stores the final digest.
  
  @retval     EFI_SUCCESS   Always successfully calculate the final digest.
**/
EFI_STATUS
EFIAPI
CalcHashSha1 (
  IN  CONST UINT8     *Data,
  IN        UINTN     DataLen,
  OUT       UINT8     *Digest
  )
{
  VOID     *Sha1Ctx;
  UINTN    CtxSize;

  CtxSize = Sha1GetContextSize ();
  Sha1Ctx = AllocatePool (CtxSize);
  ASSERT (Sha1Ctx != NULL);

  Sha1Init (Sha1Ctx);
  Sha1Update (Sha1Ctx, Data, DataLen);
  Sha1Final (Sha1Ctx, Digest);

  FreePool (Sha1Ctx);

  return EFI_SUCCESS;
}

/**
  Hash data and extend to PCR.

  @param PcrIndex      PCR to be extended.
  @param DataToHash    Data to be hashed.
  @param DataToHashLen Data size.
  @param DigestList    Digest list.

  @retval EFI_SUCCESS     Hash data and DigestList is returned.
**/
EFI_STATUS
EFIAPI
HashAndExtend (
  IN TPMI_DH_PCR                    PcrIndex,
  IN VOID                           *DataToHash,
  IN UINTN                          DataToHashLen,
  OUT TPML_DIGEST_VALUES            *DigestList
  )
{
  EFI_STATUS         Status;

  ZeroMem(DigestList, sizeof(*DigestList));
  DigestList->count = 1;
  DigestList->digests[0].hashAlg = TPM_ALG_SHA1;
  Status = CalcHashSha1(DataToHash, DataToHashLen, (UINT8*)&DigestList->digests[0].digest.sha1[0]);
  if (EFI_ERROR(Status)) {
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

  Status = Tpm2Extend (
             DigestList,
             PcrIndex
             );
  if (EFI_ERROR(Status)) {
    Status = EFI_DEVICE_ERROR;
    goto Exit;
  }

Exit:
  return Status;
}


/**
  Start hash sequence.

  @param HashHandle Hash handle.

  @retval EFI_SUCCESS          Hash sequence start and HandleHandle returned.
  @retval EFI_OUT_OF_RESOURCES No enough resource to start hash.
**/
EFI_STATUS
EFIAPI
HashStart (
  OUT HASH_HANDLE    *HashHandle
  )
{
  VOID     *Sha1Ctx;
  UINTN    CtxSize;

  CtxSize = Sha1GetContextSize ();
  Sha1Ctx = AllocatePool (CtxSize);
  ASSERT (Sha1Ctx != NULL);

  Sha1Init(Sha1Ctx);
  *HashHandle = (HASH_HANDLE)(UINTN)Sha1Ctx;
  return EFI_SUCCESS;
}

/**
  Update hash sequence data.

  @param HashHandle    Hash handle.
  @param DataToHash    Data to be hashed.
  @param DataToHashLen Data size.

  @retval EFI_SUCCESS     Hash sequence updated.
**/
EFI_STATUS
EFIAPI
HashUpdate (
  IN HASH_HANDLE    HashHandle,
  IN VOID           *DataToHash,
  IN UINTN          DataToHashLen
  )
{
  Sha1Update ((VOID*)(UINTN)HashHandle, DataToHash, DataToHashLen);
  return EFI_SUCCESS;
}




/**
  Hash sequence complete and extend to PCR.

  @param HashHandle    Hash handle.
  @param PcrIndex      PCR to be extended.
  @param DataToHash    Data to be hashed.
  @param DataToHashLen Data size.
  @param DigestList    Digest list.

  @retval EFI_SUCCESS     Hash sequence complete and DigestList is returned.
**/
EFI_STATUS
EFIAPI
HashCompleteAndExtend (
  IN HASH_HANDLE         HashHandle,
  IN TPMI_DH_PCR         PcrIndex,
  IN VOID                *DataToHash,
  IN UINTN               DataToHashLen,
  OUT TPML_DIGEST_VALUES *DigestList
  )
{
  VOID             *Sha1Ctx;
  EFI_STATUS       Status;

  ZeroMem(DigestList, sizeof(*DigestList));
  DigestList->count = 1;
  DigestList->digests[0].hashAlg = TPM_ALG_SHA1;  
  
  Sha1Ctx = (VOID*)(UINTN)HashHandle;
  Sha1Update(Sha1Ctx, DataToHash, DataToHashLen);
  Sha1Final (Sha1Ctx, (UINT8*)&DigestList->digests[0].digest.sha1[0]);
  FreePool(Sha1Ctx);
  
  Status = Tpm2Extend (
             DigestList,
             PcrIndex
             );

  return Status;
}



