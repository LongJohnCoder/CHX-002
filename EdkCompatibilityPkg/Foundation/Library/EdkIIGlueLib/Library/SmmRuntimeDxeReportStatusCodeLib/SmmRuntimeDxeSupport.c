/*++

Copyright (c) 2004 - 2011, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.              


Module Name:

  SmmRuntimeDxeSupport.c
  
Abstract: 

  Report Status Code Library for DXE Phase.

--*/

#include "ReportStatusCodeLibInternal.h"
#include EFI_PROTOCOL_DEFINITION (SmmStatusCode)
#include EFI_GUID_DEFINITION (GlobalVariable)

#define DEBUG_MASK_VARIABLE_NAME L"EFIDebug"

EFI_EVENT                     mVirtualAddressChangeEvent;
EFI_EVENT                     mExitBootServicesEvent;
EFI_EVENT                     mSmmStatusCodeEvent = NULL;
EFI_STATUS_CODE_DATA          *mStatusCodeData;
BOOLEAN                       mInSmm;
EFI_SMM_BASE_PROTOCOL         *mSmmBase;
EFI_RUNTIME_SERVICES          *mRTSmmRuntimeDxeReportStatusCodeLib;
BOOLEAN                       mHaveExitedBootServices = FALSE;
EFI_SMM_STATUS_CODE_PROTOCOL  *mSmmStatusCode;
UINTN                         mErrorLevel = EDKII_GLUE_DebugPrintErrorLevel;
BOOLEAN                       mDebugMaskInitialized = FALSE;

EFI_STATUS
InstallSmmDebugMaskProtocol (
  IN EFI_HANDLE ImageHandle
  );

EFI_STATUS
InstallRuntimeDebugMaskProtocol (
  IN EFI_HANDLE ImageHandle
  );

EFI_STATUS
UnInstallRuntimeDebugMaskProtocol (
  IN EFI_HANDLE ImageHandle
  );

EFI_STATUS
UnInstallSmmDebugMaskProtocol (
  IN EFI_HANDLE ImageHandle
  );

VOID
InitializeDebugMask (
  )
/*++

Routine Description:

  Initialize the debug mask when the variable service is ready.

--*/
{
  EFI_STATUS              Status;
  UINTN                   DebugMask;
  UINTN                   DataSize;

  DataSize = sizeof(UINTN);
  Status = gRT->GetVariable(
                  DEBUG_MASK_VARIABLE_NAME,
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &DataSize,
                  &DebugMask
                  );

  if (Status != EFI_NOT_AVAILABLE_YET) {
    //
    // If EFI Variable Services are available, then set a flag so the EFI
    // Variable will not be read again by this module.
    //
    mDebugMaskInitialized = TRUE;
    if (!EFI_ERROR (Status)) {
      //
      // If the EFI Varible exists, then set this module's module's mask to
      // the global level mask value.
      //
      mErrorLevel = DebugMask;
    }
  }
}

EFI_STATUS
EFIAPI
GetDebugMask (
  IN EFI_DEBUG_MASK_PROTOCOL      *This,             // Calling context
  IN OUT UINTN                    *CurrentDebugMask  // Ptr to store current debug mask
  )
/*++

Routine Description:
  DebugMask protocol member function.
  Gets the current debug mask for an image, on which this protocol has been installed.
  
Arguments:

  This                  - Indicates calling context 
  CurrentDebugMask      - Ptr to store current debug mask

Returns:
  EFI_SUCCESS           - Debug mask is retrieved successfully
  EFI_INVALID_PARAMETER - CurrentDebugMask is NULL.
  EFI_UNSUPPORTED       - The handle on which this protocol is installed is not an image handle.

--*/
{
  //
  // Check Parameter
  //
  if (CurrentDebugMask == NULL){
    return EFI_INVALID_PARAMETER;
  }

  *CurrentDebugMask = mErrorLevel;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SetDebugMask (
  IN  EFI_DEBUG_MASK_PROTOCOL     *This,             // Calling context
  IN  UINTN                       NewDebugMask       // New Debug Mask value to set
  )
/*++

Routine Description:
  DebugMask protocol member function.
  Updates the current debug mask for an image, on which this protocol has been installed.
  
Arguments:

  This                  - Calling context
  NewDebugMask          - New Debug Mask value to set

Returns:
  EFI_SUCCESS           - Debug mask is updated with the new value successfully
  EFI_UNSUPPORTED       - The handle on which this protocol is installed is not an image handle.

--*/
{

  if (!mDebugMaskInitialized) {
    //
    // If DebugMask is set, it is initialized. Don't need to init again.
    //
    mDebugMaskInitialized = TRUE;
  }

  mErrorLevel = NewDebugMask;
  return EFI_SUCCESS;
}  

//
// Debug Mask Protocol instance
//
EFI_DEBUG_MASK_PROTOCOL  mDebugMaskProtocol = {
  EFI_DEBUG_MASK_REVISION,
  GetDebugMask,
  SetDebugMask
};

EFI_STATUS
InstallSmmDebugMaskProtocol (
  IN EFI_HANDLE ImageHandle
  )
/*++

Routine Description:

  Install Smm debug mask protocol on an image handle.

Arguments:

  ImageHandle           - Image handle which debug mask protocol will install on

Returns:

  EFI_INVALID_PARAMETER - Invalid image handle
  EFI_OUT_OF_RESOURCES  - No enough buffer could be allocated
  EFI_SUCCESS           - Debug mask protocol successfully installed

--*/
{
  EFI_STATUS                 Status;

  //
  // Install Debug Mask Protocol in Image Handle
  //
  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gEfiSmmDebugMaskProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mDebugMaskProtocol
                  );
  return Status;
}

EFI_STATUS
UnInstallSmmDebugMaskProtocol (
  IN EFI_HANDLE ImageHandle
  )
/*++

Routine Description:

  Uninstall Smm debug mask protocol on an image handle.

Arguments:

  ImageHandle           - Image handle which debug mask protocol will uninstall on

Returns:

  EFI_INVALID_PARAMETER - Invalid image handle
  EFI_SUCCESS           - Debug mask protocol successfully uninstalled

--*/
{
  EFI_STATUS                 Status;

  if (ImageHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Install Debug Mask Protocol in Image Handle
  //
  Status = gBS->UninstallProtocolInterface (
                  &ImageHandle,
                  &gEfiSmmDebugMaskProtocolGuid,
                  &mDebugMaskProtocol
                  );

  if (EFI_ERROR (Status)) {
    return Status;
  }
  return EFI_SUCCESS;
}

EFI_STATUS
InstallRuntimeDebugMaskProtocol (
  IN EFI_HANDLE ImageHandle
  )
/*++

Routine Description:

  Install Runtime debug mask protocol on an image handle.

Arguments:

  ImageHandle           - Image handle which debug mask protocol will install on

Returns:

  EFI_INVALID_PARAMETER - Invalid image handle
  EFI_OUT_OF_RESOURCES  - No enough buffer could be allocated
  EFI_SUCCESS           - Debug mask protocol successfully installed

--*/
{
  EFI_STATUS                 Status;

  //
  // Install Debug Mask Protocol in Image Handle
  //
  Status = gBS->InstallProtocolInterface (
             &ImageHandle,
             &gEfiRuntimeDebugMaskProtocolGuid,
             EFI_NATIVE_INTERFACE,
             &mDebugMaskProtocol
             );
  return EFI_SUCCESS;
}

EFI_STATUS
UnInstallRuntimeDebugMaskProtocol (
  IN EFI_HANDLE ImageHandle
  )
/*++

Routine Description:

  Uninstall Runtime debug mask protocol on an image handle.

Arguments:

  ImageHandle           - Image handle which debug mask protocol will uninstall on

Returns:

  EFI_INVALID_PARAMETER - Invalid image handle
  EFI_SUCCESS           - Debug mask protocol successfully uninstalled

--*/
{
  EFI_STATUS                 Status;

  if (ImageHandle == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Remove Protocol from ImageHandle
  //                    
  Status = gBS->UninstallProtocolInterface (
             ImageHandle,
             &gEfiDebugMaskProtocolGuid,
             &mDebugMaskProtocol
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  return EFI_SUCCESS;
}

VOID
EFIAPI
InitializeSmmStatusCode (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
/*++

Routine Description:

  Initialize the Smm Status code point

Arguments:

  Event   The Event that is being processed
  Context Event Context

Returns:

  None

--*/
{
  EFI_STATUS Status;

  Status = gBS->LocateProtocol (&gEfiSmmStatusCodeProtocolGuid, NULL, (VOID **) &mSmmStatusCode);
  if (EFI_ERROR (Status)) {
    mSmmStatusCode = NULL;
  } else {
    gBS->CloseEvent (mSmmStatusCodeEvent);
  }
}

EFI_STATUS
SmmStatusCodeReport (
  IN EFI_STATUS_CODE_TYPE     Type,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId OPTIONAL,
  IN EFI_STATUS_CODE_DATA     *Data     OPTIONAL
  )
{
  if (mSmmStatusCode != NULL) {
    (mSmmStatusCode->ReportStatusCode) (mSmmStatusCode, Type, Value, Instance, CallerId, Data);
  }
  return EFI_SUCCESS;
}

/**
  Locate he report status code service.

  @return     EFI_REPORT_STATUS_CODE    function point to
              ReportStatusCode.
**/
EFI_REPORT_STATUS_CODE
InternalGetReportStatusCode (
  VOID
  )
{
#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
  EFI_STATUS_CODE_PROTOCOL  *StatusCodeProtocol;
  EFI_STATUS                Status;
#endif

  if (mInSmm) {
    return (EFI_REPORT_STATUS_CODE) SmmStatusCodeReport;
#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
  } else if (!mHaveExitedBootServices) {
  	//
  	// Check gBS just in case. ReportStatusCode is called before gBS is initialized.
  	//
    if (gBS != NULL) {
      Status = gBS->LocateProtocol (&gEfiStatusCodeRuntimeProtocolGuid, NULL, (VOID**)&StatusCodeProtocol);
      if (!EFI_ERROR (Status) && StatusCodeProtocol != NULL) {
        return StatusCodeProtocol->ReportStatusCode;
      }
    }
  }
#elif (TIANO_RELEASE_VERSION != 0)
  } else if (mRTSmmRuntimeDxeReportStatusCodeLib != NULL) {
    return mRTSmmRuntimeDxeReportStatusCodeLib->ReportStatusCode;
  }
#endif

  return NULL;
}


/**
  Fixup internal report status code protocol interface.

  @param[in]    Event   The Event that is being processed
  @param[in]    Context Event Context
**/
VOID
EFIAPI
ReportStatusCodeLibVirtualAddressChange (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  mRTSmmRuntimeDxeReportStatusCodeLib->ConvertPointer (0, (VOID **) &gDebugMaskInterface->GetDebugMask);
  mRTSmmRuntimeDxeReportStatusCodeLib->ConvertPointer (0, (VOID **) &gDebugMaskInterface);
  if (NULL != mReportStatusCode) {
    mRTSmmRuntimeDxeReportStatusCodeLib->ConvertPointer (0, (VOID **) &mReportStatusCode);
  }
  mRTSmmRuntimeDxeReportStatusCodeLib->ConvertPointer (0, (VOID **) &mStatusCodeData);
  mRTSmmRuntimeDxeReportStatusCodeLib->ConvertPointer (0, (VOID **) &mRTSmmRuntimeDxeReportStatusCodeLib);
}

/**
  Update the In Runtime Indicator.

  @param[in]    Event   The Event that is being processed
  @param[in]    Context Event Context
**/
VOID
EFIAPI
ReportStatusCodeLibExitBootServices (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  if (!mDebugMaskInitialized) {
    InitializeDebugMask ();
  }

  if (mReportStatusCode == NULL) {
    mReportStatusCode = InternalGetReportStatusCode ();
  }
  mHaveExitedBootServices = TRUE;
}

/**
  Intialize Report Status Code Lib.

  @param[in]  ImageHandle   The firmware allocated handle for the EFI image.
  @param[in]  SystemTable   A pointer to the EFI System Table.

  @return     EFI_STATUS    always returns EFI_SUCCESS.
**/
EFI_STATUS
EFIAPI
ReportStatusCodeLibConstruct (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS            Status;
  VOID                  *SmmStatusCodeRegistration;

  //
  // SMM driver depends on the SMM BASE protocol.
  // the SMM driver must be success to locate protocol.
  // 
  Status = gBS->LocateProtocol (&gEfiSmmBaseProtocolGuid, NULL, (VOID **) &mSmmBase);
  if (!EFI_ERROR (Status)) {
    mSmmBase->InSmm (mSmmBase, &mInSmm);
    if (mInSmm) {
      Status = mSmmBase->SmmAllocatePool (
                           mSmmBase,
                           EfiRuntimeServicesData, 
                           sizeof (EFI_STATUS_CODE_DATA) + EFI_STATUS_CODE_DATA_MAX_SIZE, 
                           (VOID **) &mStatusCodeData
                           );
      ASSERT_EFI_ERROR (Status);
      Status = gBS->LocateProtocol (&gEfiSmmStatusCodeProtocolGuid, NULL, (VOID **) &mSmmStatusCode);
      if (EFI_ERROR (Status)) {
        mSmmStatusCode = NULL;
        //
        // Create the event
        //
        Status = gBS->CreateEvent (
                        EFI_EVENT_NOTIFY_SIGNAL,
                        TPL_CALLBACK,
                        InitializeSmmStatusCode,
                        NULL,
                        &mSmmStatusCodeEvent
                        );
        ASSERT_EFI_ERROR (Status);
        //
        // Register for protocol notifactions on this event
        // NOTE: Because this protocol will be installed in SMM, it is safety to
        // register ProtocolNotify here. This event will be triggered in SMM later.
        //
        Status = gBS->RegisterProtocolNotify (
                        &gEfiSmmStatusCodeProtocolGuid,
                        mSmmStatusCodeEvent,
                        &SmmStatusCodeRegistration
                        );
        ASSERT_EFI_ERROR (Status);
      }
      Status = InstallSmmDebugMaskProtocol (ImageHandle);
      return EFI_SUCCESS;
    }
  }

  Status = InstallRuntimeDebugMaskProtocol (ImageHandle);

  //
  // Library should not use the gRT directly, since it
  // may be converted by other library instance.
  // 
  mRTSmmRuntimeDxeReportStatusCodeLib = gRT;
  mInSmm  = FALSE;

  (gBS->AllocatePool) (EfiRuntimeServicesData, sizeof (EFI_STATUS_CODE_DATA) + EFI_STATUS_CODE_DATA_MAX_SIZE, (VOID **)&mStatusCodeData);
  ASSERT (NULL != mStatusCodeData);
  //
  // Cache the report status code service
  // 
  mReportStatusCode = InternalGetReportStatusCode ();

  //
  // Register the call back of virtual address change
  // 
  Status = gBS->CreateEvent (
                  EFI_EVENT_SIGNAL_VIRTUAL_ADDRESS_CHANGE,
                  TPL_NOTIFY,
                  ReportStatusCodeLibVirtualAddressChange,
                  NULL,
                  &mVirtualAddressChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);


  //
  // Register the call back of exit boot services
  // 
  Status = gBS->CreateEvent (
                  EFI_EVENT_SIGNAL_EXIT_BOOT_SERVICES,
                  TPL_NOTIFY,
                  ReportStatusCodeLibExitBootServices,
                  NULL,
                  &mExitBootServicesEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Desctructor of library will close events.
  
  @param ImageHandle callder module's image handle
  @param SystemTable pointer to EFI system table.
  @return the status of close event.
**/
EFI_STATUS
EFIAPI
ReportStatusCodeLibDestruct (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  if (!mInSmm) {
    //
    // Close SetVirtualAddressMap () notify function
    //
    ASSERT (gBS != NULL);
    Status = gBS->CloseEvent (mVirtualAddressChangeEvent);
    ASSERT_EFI_ERROR (Status);
    Status = gBS->CloseEvent (mExitBootServicesEvent);
    ASSERT_EFI_ERROR (Status);

    (gBS->FreePool) (mStatusCodeData);
    UnInstallRuntimeDebugMaskProtocol (ImageHandle);
  } else {
  	if (mSmmStatusCodeEvent != NULL) {
      Status = gBS->CloseEvent (mSmmStatusCodeEvent);
      ASSERT_EFI_ERROR (Status);
    }
    mSmmBase->SmmFreePool (mSmmBase, mStatusCodeData);
    UnInstallSmmDebugMaskProtocol (ImageHandle);
  }

  return EFI_SUCCESS;
}

/**
  Reports a status code with full parameters.

  The function reports a status code.  If ExtendedData is NULL and ExtendedDataSize
  is 0, then an extended data buffer is not reported.  If ExtendedData is not
  NULL and ExtendedDataSize is not 0, then an extended data buffer is allocated.
  ExtendedData is assumed not have the standard status code header, so this function
  is responsible for allocating a buffer large enough for the standard header and
  the extended data passed into this function.  The standard header is filled in
  with a GUID specified by ExtendedDataGuid.  If ExtendedDataGuid is NULL, then a
  GUID of gEfiStatusCodeSpecificDatauid is used.  The status code is reported with
  an instance specified by Instance and a caller ID specified by CallerId.  If
  CallerId is NULL, then a caller ID of gEfiCallerIdGuid is used.

  ReportStatusCodeEx()must actively prevent recursion.  If ReportStatusCodeEx()
  is called while processing another any other Report Status Code Library function,
  then ReportStatusCodeEx() must return EFI_DEVICE_ERROR immediately.

  If ExtendedData is NULL and ExtendedDataSize is not zero, then ASSERT().
  If ExtendedData is not NULL and ExtendedDataSize is zero, then ASSERT().

  @param  Type              Status code type.
  @param  Value             Status code value.
  @param  Instance          Status code instance number.
  @param  CallerId          Pointer to a GUID that identifies the caller of this
                            function.  If this parameter is NULL, then a caller
                            ID of gEfiCallerIdGuid is used.
  @param  ExtendedDataGuid  Pointer to the GUID for the extended data buffer.
                            If this parameter is NULL, then a the status code
                            standard header is filled in with
                            gEfiStatusCodeSpecificDataGuid.
  @param  ExtendedData      Pointer to the extended data buffer.  This is an
                            optional parameter that may be NULL.
  @param  ExtendedDataSize  The size, in bytes, of the extended data buffer.

  @retval  EFI_SUCCESS           The status code was reported.
  @retval  EFI_OUT_OF_RESOURCES  There were not enough resources to allocate
                                 the extended data section if it was specified.
  @retval  EFI_UNSUPPORTED       Report status code is not supported

**/
EFI_STATUS
EFIAPI
InternalReportStatusCodeEx (
  IN EFI_STATUS_CODE_TYPE   Type,
  IN EFI_STATUS_CODE_VALUE  Value,
  IN UINT32                 Instance,
  IN CONST EFI_GUID         *CallerId          OPTIONAL,
  IN CONST EFI_GUID         *ExtendedDataGuid  OPTIONAL,
  IN CONST VOID             *ExtendedData      OPTIONAL,
  IN UINTN                  ExtendedDataSize
  )
{
  ASSERT (!((ExtendedData == NULL) && (ExtendedDataSize != 0)));
  ASSERT (!((ExtendedData != NULL) && (ExtendedDataSize == 0)));

  if (ExtendedDataSize > EFI_STATUS_CODE_DATA_MAX_SIZE) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Fill in the extended data header
  //
  mStatusCodeData->HeaderSize = sizeof (EFI_STATUS_CODE_DATA);
  mStatusCodeData->Size = (UINT16)ExtendedDataSize;
  if (ExtendedDataGuid == NULL) {
    ExtendedDataGuid = &gEfiStatusCodeSpecificDataGuid;
  }
  CopyGuid (&mStatusCodeData->Type, ExtendedDataGuid);

  //
  // Fill in the extended data buffer
  //
  CopyMem (mStatusCodeData + 1, ExtendedData, ExtendedDataSize);

  //
  // Report the status code
  //
  if (CallerId == NULL) {
    CallerId = &gEfiCallerIdGuid;
  }
  return  InternalReportStatusCode (Type, Value, Instance, CallerId, mStatusCodeData);
}