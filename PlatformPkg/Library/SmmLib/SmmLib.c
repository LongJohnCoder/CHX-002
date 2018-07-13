

#include <Base.h>
#include <Library/SmmLib.h>
#include <library/PcdLib.h>
#include <library/IoLib.h>
#include <library/DebugLib.h>
#include <PlatformDefinition.h>

/**
  Triggers an SMI at boot time.  

  This function triggers a software SMM interrupt at boot time.

**/
VOID
EFIAPI
TriggerBootServiceSoftwareSmi (
  VOID
  )
{
  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__)); 
  return;
}


/**
  Triggers an SMI at run time.  

  This function triggers a software SMM interrupt at run time.

**/
VOID
EFIAPI
TriggerRuntimeSoftwareSmi (
  VOID
  )
{
  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__)); 
  return;
}



/**
  Test if a boot time software SMI happened.  

  This function tests if a software SMM interrupt happened. If a software SMM 
  interrupt happened and it was triggered at boot time, it returns TRUE. Otherwise, 
  it returns FALSE.

  @retval TRUE   A software SMI triggered at boot time happened.
  @retval FALSE  No software SMI happened or the software SMI was triggered at run time.

**/
BOOLEAN
EFIAPI
IsBootServiceSoftwareSmi (
  VOID
  )
{
  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__)); 
  return FALSE;
}


/**
  Test if a run time software SMI happened.  

  This function tests if a software SMM interrupt happened. If a software SMM 
  interrupt happened and it was triggered at run time, it returns TRUE. Otherwise, 
  it returns FALSE.

  @retval TRUE   A software SMI triggered at run time happened.
  @retval FALSE  No software SMI happened or the software SMI was triggered at boot time.

**/
BOOLEAN
EFIAPI
IsRuntimeSoftwareSmi (
  VOID
  )
{
  DEBUG((EFI_D_INFO, "%a()\n", __FUNCTION__)); 
  return FALSE;
}

/**
  Clear APM SMI Status Bit; Set the EOS bit. 
  
**/
VOID
EFIAPI
ClearSmi (
  VOID
  )
{

  IoWrite16(PMIO_REG(PMIO_GLOBAL_STA), PMIO_SWSMIS);  ///PMIO_Rx28[6] Software SMI Status => Clear the APM SMI Status Bit
  IoOr16(PMIO_REG(PMIO_GLOBAL_CTL), PMIO_INSMI);      ///PMIO_Rx2C[8] SMI Active Status => Set the EOS Bit
}

BOOLEAN IsSmiSourceActive()
{
  BOOLEAN  rc;
  rc = (IoRead16(PMIO_REG(PMIO_GLOBAL_CTL)) & PMIO_INSMI)?TRUE:FALSE; ///PMIO_Rx2C[8] SMI Active Status 
	return rc;
}




