//**********************************************************************
//**********************************************************************
//**                                                                  **
//**     Copyright (c) 2018 Shanghai Zhaoxin Semiconductor Co., Ltd.  **
//**                                                                  **
//**********************************************************************
//**********************************************************************

#include <PiDxe.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Protocol/LegacyBios.h>
#include <Protocol/LegacyInterruptHandler.h>
#include <Library/IoLib.h>
#include <PlatformDefinition.h>






UINT8 ChipsetInt15Handler[] = {
//old_handler:
0, 0, 0, 0,                     //old INT15 handler, will be fixed up
0x9c,                           //pushf
0x3d, 0x00, 0x5f,               //cmp ax, 5f00h
0x72, 0x07,                     //jb not_us
0x3d, 0xa0, 0x5f,               //cmp ax, 5fa0h
0x77, 0x02,                     //ja not_us
0xeb, 0x06,                     //ja thats_us
//not_us:
0x9d,                           //popf
0x2e, 0xff, 0x2e, 0x00,0x00,    //jmp cs:old_handler
//thats_us:
0x9d,                           //popf
0x66, 0x0f, 0xb7, 0xea,         //movzx ebp, dx
0x66, 0xc1, 0xe5, 0x10,         //shl ebp, 10h
0x0b, 0xe8,                     //or bp, ax
0xb0, 0x5f,                     //mov al, 5Fh
0xba, 0x2f, 0x08,               //mov dx, 82Fh
0xee,                           //out dx, al
0xeb, 0x00,                     //jmp $+2
0xfb,                           //sti
0xca, 02, 0x00                  //retf 2
};

//

EFI_STATUS
EFIAPI
InstallChipsetInt15 (
  IN    EFI_HANDLE                  ImageHandle,
  IN    EFI_SYSTEM_TABLE            *SystemTable
  )
{
  EFI_STATUS                Status;
  EFI_LEGACY_BIOS_PROTOCOL  *LegacyBios;
  INTERRUPT_HANDLER         *NewHandler;
  LEGACY_VECTOR             *Vector;
  EFI_BOOT_MODE             BootMode;    
  UINT32                    IfCallINT15;
    
    DEBUG((EFI_D_ERROR, "(L%d) %a() , Hook INT 15 \n", __LINE__, __FUNCTION__));
	  //IVS-20181015 Judge whether to Call INT15
	IfCallINT15 = MmioRead32(SCRCH_PCI_REG(D0F6_BIOS_SCRATCH_REG_12))&(BIT0);

    BootMode = GetBootModeHob();
    if(BootMode == BOOT_IN_RECOVERY_MODE || BootMode == BOOT_ON_FLASH_UPDATE){
      return EFI_SUCCESS;
    }      
    
     //Now fixup ChipsetInt15Handler
    Vector = (LEGACY_VECTOR *) (UINTN)(0x15 * 4);
    ChipsetInt15Handler[0] = Vector->Offset & 0xff;
    ChipsetInt15Handler[1] = (Vector->Offset>>8) & 0xff;
    ChipsetInt15Handler[2] = Vector->Segment & 0xff;
    ChipsetInt15Handler[3] = (Vector->Segment>>8) & 0xff;

    Status = gBS->LocateProtocol (
                  &gEfiLegacyBiosProtocolGuid,
                  NULL,
                  &LegacyBios
                  );
    ASSERT_EFI_ERROR (Status);
    DEBUG((EFI_D_ERROR, "(L%d) %a()\n", __LINE__, __FUNCTION__));

		if(IfCallINT15)
		{
    Status = LegacyBios->GetLegacyRegion (
                            LegacyBios,
                            sizeof(ChipsetInt15Handler)/sizeof(ChipsetInt15Handler[0]),
                            0,  // E or F segment
                            16,// 16-byte aligned
                            (VOID **) &NewHandler
                            );
    ASSERT_EFI_ERROR (Status);
  DEBUG((EFI_D_ERROR, "(L%d) %a()\n", __LINE__, __FUNCTION__));

    ASSERT (((UINTN) NewHandler & 0xF) == 0);
   
    Status = LegacyBios->CopyLegacyRegion (
                            LegacyBios,
                            sizeof(ChipsetInt15Handler)/sizeof(ChipsetInt15Handler[0]),
                            NewHandler,
                            ChipsetInt15Handler
                            );
    ASSERT_EFI_ERROR (Status);
     DEBUG((EFI_D_ERROR, "(L%d) %a()\n", __LINE__, __FUNCTION__));
 
    Vector->Segment = (UINT16) ((UINTN) &NewHandler->Code >> 4);
    Vector->Offset  = (UINT16) ((UINTN) &NewHandler->Code & 0xF); 
		}
    return EFI_SUCCESS;
}
