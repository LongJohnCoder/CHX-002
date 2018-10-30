#include <PlatformSetup.h>



EFI_STATUS
EFIAPI
AdvancedFormCallback (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL *This,
  IN EFI_BROWSER_ACTION                   Action,
  IN EFI_QUESTION_ID                      KeyValue,
  IN UINT8                                Type,
  IN EFI_IFR_TYPE_VALUE                   *Value,
  OUT EFI_BROWSER_ACTION_REQUEST          *ActionRequest
  )
{
  EFI_HII_HANDLE                 HiiHandle = NULL;  
  SETUP_DATA        CurrentSetupData;
  UINT8 i;
   
  DEBUG((EFI_D_ERROR,"\nDevicesFormCallback(), Action :%d.\n", Action));
    
  if ((Action == EFI_BROWSER_ACTION_FORM_OPEN) || \
      (Action == EFI_BROWSER_ACTION_FORM_CLOSE)) {
    //
    // Do nothing for UEFI OPEN/CLOSE Action.
    //
    return EFI_SUCCESS;
  }

  if (Action != EFI_BROWSER_ACTION_CHANGING && Action != EFI_BROWSER_ACTION_CHANGED) {
    //
    // Do nothing for other UEFI Action. Only do call back when data is changed.
    //
    return EFI_UNSUPPORTED;
  }

	//
	 // Get Security form hii databas handle.
	 //
	 for (i=0;i<gSetupCallbackInfoNumber;i++) {
	   if (ADVANCED_FORM_SET_CLASS == gSetupCallbackInfo[i].Class) {
		 HiiHandle = gSetupCallbackInfo[i].HiiHandle;
		 break;
	   }
	 }	
	 if (NULL == HiiHandle) {
	   DEBUG((EFI_D_ERROR,"AdvancedFormCallback(),Can't get Advanced Form HiiHandle.\n"));
	   return EFI_INVALID_PARAMETER;
	 }
 
 HiiGetBrowserData (&gPlatformSetupVariableGuid, L"Setup", sizeof (SETUP_DATA), (UINT8 *) &CurrentSetupData);

	switch (KeyValue){

  case KEY_VALUE_NBSPE:
   if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 != 0)) {
	 CurrentSetupData.D0F0SPEValue=CurrentSetupData.D0F1SPEValue=CurrentSetupData.D0F2SPEValue=CurrentSetupData.D0F3SPEValue=CurrentSetupData.D0F4SPEValue\
	 =CurrentSetupData.D0F5SPEValue=CurrentSetupData.D0F6SPEValue=CurrentSetupData.D3F0SPEValue=CurrentSetupData.D3F1SPEValue\
	 =CurrentSetupData.D3F2SPEValue=CurrentSetupData.D3F3SPEValue=CurrentSetupData.D4F0SPEValue=CurrentSetupData.D4F1SPEValue=CurrentSetupData.D5F0SPEValue\
	 =CurrentSetupData.D5F1SPEValue=CurrentSetupData.RCRBHSPEValue=CurrentSetupData.PCIEEPHYSPEValue=CurrentSetupData.D1F0SPEValue\
	 =CurrentSetupData.D8F0SPEValue=CurrentSetupData.D9F0SPEValue=CurrentSetupData.NBSPEValue-1;
   }
   
   break;

 case KEY_VALUE_SBSPE:
   if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 != 0)) {
	   CurrentSetupData.VARTSPEValue=CurrentSetupData.ESPISPEValue=CurrentSetupData.BusCtrlSPEValue\
	   	=CurrentSetupData.PMUSPEValue=CurrentSetupData.PCCASPEValue=CurrentSetupData.HDACSPEValue=CurrentSetupData.SPISPEValue=CurrentSetupData.SBSPEValue-1;

	  /* if(((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 1))){//S mode,disable dynamic clock
	    CurrentSetupData.SBDynamicClkControl=0;
	   	}
	   else{//p-e mode ,enable dynamic clock
		   CurrentSetupData.SBDynamicClkControl=1;
	   	}*/
   }
   break;
   
 case KEY_VALUE_IOVEN:
	 if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 != 0)) {
	   //Only DMA cycle on snoop path can be remapped by TACTL, so GFX must be configurated to using snoop path only.
	   //Only DMA cycle on snoop path can be remapped by TACTL, so the HDAC should be configurated to use snoop path only when TACTL is enabled.
	   //CurrentSetupData.GFX_snoop_path_only=0;
	   CurrentSetupData.GoNonSnoopPath=0;
	 }
	 else
	 {
	   //CurrentSetupData.GFX_snoop_path_only=1;
	   CurrentSetupData.GoNonSnoopPath=1;
	 }
	 break;
	 
case KEY_VALUE_IOVQIEN:
	 if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 != 0))
	 {
		CurrentSetupData.IOVINTREnable = 1;
	 }
	 else
	 {
		//if QI capability is disabled, INTR capability should also be disabled.
		CurrentSetupData.IOVINTREnable = 0;
	 }
		 break;
case KEY_VALUE_CRB_MODE_SEL:
	 if ((Type == EFI_IFR_TYPE_NUM_SIZE_8) && (Value->u8 == 0))
	 {
//CRB Platform Mode choose notebook mode
		CurrentSetupData.CpuCState = 5;
		CurrentSetupData.CxEEnable = 0;
		CurrentSetupData.XhcU1U2Ctrl = 1;
		CurrentSetupData.IDEHIPMEn = 1;
		CurrentSetupData.PcieASPM = 0;
		CurrentSetupData.PcieASPMBootArch = 0;
		#ifdef IOE_EXIST
		CurrentSetupData.IoeSataHIPMEn = 0;
		CurrentSetupData.IoeSataALPMEn = 0;
        #endif
	 }
	 else
	 {
	
//CRB Platform Mode choose  desktop mode
		
		CurrentSetupData.CxEEnable = 0;
		CurrentSetupData.CpuCState = 0;
		CurrentSetupData.XhcU1U2Ctrl = 0;
		CurrentSetupData.IDEHIPMEn = 0;
		CurrentSetupData.PcieASPM = 0;
		CurrentSetupData.PcieASPMBootArch = 0;
		#ifdef IOE_EXIST
		CurrentSetupData.IoeSataHIPMEn = 0;
		CurrentSetupData.IoeSataALPMEn = 0;
        #endif
	 }

     break;
  default:
		 break;
	 }
	
	HiiSetBrowserData (&gPlatformSetupVariableGuid, L"Setup", sizeof (SETUP_DATA), (UINT8 *)&CurrentSetupData, NULL);

		return EFI_SUCCESS;
}

