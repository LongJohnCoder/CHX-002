/** @file

Copyright (c) 2006 - 2014, Byosoft Corporation.<BR>
All rights reserved.This software and associated documentation (if any)
is furnished under a license and may only be used or copied in
accordance with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be reproduced,
stored in a retrieval system, or transmitted in any form or by any
means without the express written consent of Byosoft Corporation.

File Name:
   Uart.sd

Abstract:
   Uart Devices setting.

Revision History:

$END--------------------------------------------------------------------

**/

#ifdef DEVICES_FORM_SET
  #ifdef FORM_SET_GOTO
      goto CHIPSET_SB_UART_FORM_ID,
        prompt = STRING_TOKEN(STR_UART_FORM),
        help = STRING_TOKEN(STR_EMPTY);
  #endif

  #ifdef FORM_SET_FORM

    form formid = CHIPSET_SB_UART_FORM_ID,
    title = STRING_TOKEN(STR_UART_FORM);

      subtitle text = STRING_TOKEN(STR_COM_EMPTY);
      
        oneof varid   = SETUP_DATA.OnChipUartMode,
        prompt   = STRING_TOKEN(STR_UART_MODE),
        help   = STRING_TOKEN(STR_UART_MODE_HELP),
        option text = STRING_TOKEN(STR_DISABLED), value = 0, flags =  RESET_REQUIRED;
        option text = STRING_TOKEN(STR_PCI_UART), value = 1, flags =DEFAULT | MANUFACTURING | RESET_REQUIRED;
       endoneof; 
  suppressif
    ideqval SETUP_DATA.OnChipUartMode == 0;
//HYL-2016100701-start  
#ifdef CHX001_PKG_A      
   grayoutif	TRUE;
#endif
//HYL-2016100701-end 
       oneof varid   = SETUP_DATA.UartModuleSelection,
        prompt   = STRING_TOKEN(STR_UART_MODE_SEL),
        help   = STRING_TOKEN(STR_UART_MODE_SEL_HELP),
        option text = STRING_TOKEN(STR_LEGACY), value = 0, flags =DEFAULT | MANUFACTURING | RESET_REQUIRED;
        option text = STRING_TOKEN(STR_PCI_DMA), value = 1, flags =  RESET_REQUIRED;
       endoneof;       
//HYL-2016100701-start  
#ifdef CHX001_PKG_A      
   endif;
#endif
//HYL-2016100701-end 

//HYL-2016100701-start  
#ifdef CHX001_PKG_A      
   suppressif	TRUE;
       oneof varid   = SETUP_DATA.Uart0Enable,
        prompt   = STRING_TOKEN(STR_UART0_EN),
        help   = STRING_TOKEN(STR_UART0_EN),
        option text = STRING_TOKEN(STR_DISABLED), value = 0, flags = RESET_REQUIRED | DEFAULT | MANUFACTURING;
        option text = STRING_TOKEN(STR_ENABLED), value = 1, flags = RESET_REQUIRED;
       endoneof; 
      
       oneof varid   = SETUP_DATA.Uart0IoBaseSelection,
        prompt   = STRING_TOKEN(STR_UART0_BASE_SEL),
        help   = STRING_TOKEN(STR_UART0_BASE_SEL_HELP),
        option text = STRING_TOKEN(STR_3F8), value = 0, flags =DEFAULT | MANUFACTURING |  RESET_REQUIRED;
        option text = STRING_TOKEN(STR_2F8), value = 1, flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_3E8), value = 2, flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_2E8), value = 3, flags = RESET_REQUIRED;
       endoneof;       
      
       oneof varid   = SETUP_DATA.Uart0IRQSelection,
        prompt   = STRING_TOKEN(STR_UART0_IRQ),
        help   = STRING_TOKEN(STR_UART0_IRQ_HELP),
        option text = STRING_TOKEN(STR_3), value =3, flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_4), value = 4, flags =DEFAULT | MANUFACTURING |  RESET_REQUIRED;
        option text = STRING_TOKEN(STR_7), value = 7, flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_11), value = 11, flags = RESET_REQUIRED;
       endoneof; 

    	subtitle text = STRING_TOKEN(STR_EMPTY);
    
       oneof varid   = SETUP_DATA.Uart1Enable,
        prompt   = STRING_TOKEN(STR_UART1_EN),
        help   = STRING_TOKEN(STR_EMPTY),
        option text = STRING_TOKEN(STR_DISABLED), value = 0, flags = RESET_REQUIRED | DEFAULT | MANUFACTURING;
        option text = STRING_TOKEN(STR_ENABLED), value = 1, flags = RESET_REQUIRED;
       endoneof; 
      
       oneof varid   = SETUP_DATA.Uart1IoBaseSelection,
        prompt   = STRING_TOKEN(STR_UART1_BASE_SEL),
        help   = STRING_TOKEN(STR_EMPTY),
        option text = STRING_TOKEN(STR_3F8), value = 0, flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_2F8), value = 1, flags =DEFAULT | MANUFACTURING |  RESET_REQUIRED;
        option text = STRING_TOKEN(STR_3E8), value = 2, flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_2E8), value = 3, flags = RESET_REQUIRED;
       endoneof; 
     
       oneof varid   = SETUP_DATA.Uart1IRQSelection,
        prompt   = STRING_TOKEN(STR_UART1_IRQ),
        help   = STRING_TOKEN(STR_EMPTY),
        option text = STRING_TOKEN(STR_3), value =3, flags =DEFAULT | MANUFACTURING | RESET_REQUIRED;
        option text = STRING_TOKEN(STR_4), value = 4, flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_7), value = 7, flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_11), value = 11, flags = RESET_REQUIRED;
        endoneof; 	   
   endif;
#else
//HYL-2016100701-end 
    subtitle text = STRING_TOKEN(STR_EMPTY);
   grayoutif
    ideqval SETUP_DATA.UartModuleSelection == 1;   
       oneof varid   = SETUP_DATA.Uart0Enable,
        prompt   = STRING_TOKEN(STR_UART0_EN),
        help   = STRING_TOKEN(STR_UART0_EN),
        option text = STRING_TOKEN(STR_DISABLED), value = 0, flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_ENABLED), value = 1, flags = DEFAULT | MANUFACTURING | RESET_REQUIRED;
       endoneof; 
      
       oneof varid   = SETUP_DATA.Uart0IoBaseSelection,
        prompt   = STRING_TOKEN(STR_UART0_BASE_SEL),
        help   = STRING_TOKEN(STR_UART0_BASE_SEL_HELP),
        option text = STRING_TOKEN(STR_3F8), value = 0, flags =DEFAULT | MANUFACTURING |  RESET_REQUIRED;
        option text = STRING_TOKEN(STR_2F8), value = 1, flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_3E8), value = 2, flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_2E8), value = 3, flags = RESET_REQUIRED;
       endoneof;       
      
       oneof varid   = SETUP_DATA.Uart0IRQSelection,
        prompt   = STRING_TOKEN(STR_UART0_IRQ),
        help   = STRING_TOKEN(STR_UART0_IRQ_HELP),
        option text = STRING_TOKEN(STR_3), value =3, flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_4), value = 4, flags =DEFAULT | MANUFACTURING |  RESET_REQUIRED;
        option text = STRING_TOKEN(STR_7), value = 7, flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_11), value = 11, flags = RESET_REQUIRED;
       endoneof; 
 endif;   //end gray out  
      
    subtitle text = STRING_TOKEN(STR_EMPTY);
    
       oneof varid   = SETUP_DATA.Uart1Enable,
        prompt   = STRING_TOKEN(STR_UART1_EN),
        help   = STRING_TOKEN(STR_EMPTY),
        option text = STRING_TOKEN(STR_DISABLED), value = 0, flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_ENABLED), value = 1, flags = DEFAULT | MANUFACTURING | RESET_REQUIRED;
       endoneof; 
      
       oneof varid   = SETUP_DATA.Uart1IoBaseSelection,
        prompt   = STRING_TOKEN(STR_UART1_BASE_SEL),
        help   = STRING_TOKEN(STR_EMPTY),
        option text = STRING_TOKEN(STR_3F8), value = 0, flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_2F8), value = 1, flags =DEFAULT | MANUFACTURING |  RESET_REQUIRED;
        option text = STRING_TOKEN(STR_3E8), value = 2, flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_2E8), value = 3, flags = RESET_REQUIRED;
       endoneof; 
      
       oneof varid   = SETUP_DATA.Uart1IRQSelection,
        prompt   = STRING_TOKEN(STR_UART1_IRQ),
        help   = STRING_TOKEN(STR_EMPTY),
        option text = STRING_TOKEN(STR_3), value =3, flags =DEFAULT | MANUFACTURING | RESET_REQUIRED;
        option text = STRING_TOKEN(STR_4), value = 4, flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_7), value = 7, flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_11), value = 11, flags = RESET_REQUIRED;
        endoneof; 
#endif //HYL-2016100701         
     
    subtitle text = STRING_TOKEN(STR_EMPTY);
#ifdef CHX001_HAPS //HYL-2016100701  
      oneof varid   = SETUP_DATA.Uart2Enable,
        prompt   = STRING_TOKEN(STR_UART2_EN),
        help   = STRING_TOKEN(STR_EMPTY),
        option text = STRING_TOKEN(STR_DISABLED), value = 0, flags = RESET_REQUIRED | DEFAULT | MANUFACTURING;
        option text = STRING_TOKEN(STR_ENABLED),  value = 1, flags = RESET_REQUIRED;
      endoneof; 
//HYL-2016100701-start 
#else
      oneof varid   = SETUP_DATA.Uart2Enable,
        prompt   = STRING_TOKEN(STR_UART2_EN),
        help   = STRING_TOKEN(STR_EMPTY),
        option text = STRING_TOKEN(STR_DISABLED), value = 0, flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_ENABLED),  value = 1, flags = RESET_REQUIRED | DEFAULT | MANUFACTURING;
      endoneof; 
#endif   
//HYL-2016100701-end      
       oneof varid = SETUP_DATA.Uart2IoBaseSelection,
        prompt   = STRING_TOKEN(STR_UART2_BASE_SEL),
        help   = STRING_TOKEN(STR_EMPTY),
        option text = STRING_TOKEN(STR_3F8), value = 0, flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_2F8), value = 1, flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_3E8), value = 2, flags =DEFAULT | MANUFACTURING |RESET_REQUIRED;
        option text = STRING_TOKEN(STR_2E8), value = 3, flags = RESET_REQUIRED;
       endoneof; 
       
       oneof varid = SETUP_DATA.Uart2IRQSelection,
        prompt = STRING_TOKEN(STR_UART2_IRQ),
        help   = STRING_TOKEN(STR_EMPTY),
        option text = STRING_TOKEN(STR_3),  value = 3,  flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_4),  value = 4,  flags = RESET_REQUIRED | DEFAULT | MANUFACTURING ;
        option text = STRING_TOKEN(STR_7),  value = 7,  flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_11), value = 11, flags = RESET_REQUIRED;
       endoneof; 
      
      
    subtitle text = STRING_TOKEN(STR_EMPTY);
#ifdef CHX001_HAPS //HYL-2016100701      
       oneof varid = SETUP_DATA.Uart3Enable,
        prompt = STRING_TOKEN(STR_UART3_EN),
        help   = STRING_TOKEN(STR_EMPTY),
        option text = STRING_TOKEN(STR_DISABLED), value = 0, flags = RESET_REQUIRED | DEFAULT | MANUFACTURING;
        option text = STRING_TOKEN(STR_ENABLED),  value = 1, flags = RESET_REQUIRED;
       endoneof; 
//HYL-2016100701-start 
#else
       oneof varid = SETUP_DATA.Uart3Enable,
        prompt = STRING_TOKEN(STR_UART3_EN),
        help   = STRING_TOKEN(STR_EMPTY),
        option text = STRING_TOKEN(STR_DISABLED), value = 0, flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_ENABLED),  value = 1, flags = RESET_REQUIRED | DEFAULT | MANUFACTURING;
       endoneof; 
#endif   
//HYL-2016100701-end   
   
       oneof varid   = SETUP_DATA.Uart3IoBaseSelection,
        prompt   = STRING_TOKEN(STR_UART3_BASE_SEL),
        help   = STRING_TOKEN(STR_EMPTY),
        option text = STRING_TOKEN(STR_3F8), value = 0, flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_2F8), value = 1, flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_3E8), value = 2, flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_2E8), value = 3, flags =DEFAULT | MANUFACTURING |  RESET_REQUIRED;
       endoneof; 
      
       oneof varid   = SETUP_DATA.Uart3IRQSelection,
        prompt   = STRING_TOKEN(STR_UART3_IRQ),
        help   = STRING_TOKEN(STR_EMPTY),
        option text = STRING_TOKEN(STR_3),  value = 3,  flags = RESET_REQUIRED | DEFAULT | MANUFACTURING;
        option text = STRING_TOKEN(STR_4),  value = 4,  flags=  RESET_REQUIRED;
        option text = STRING_TOKEN(STR_7),  value = 7,  flags = RESET_REQUIRED;
        option text = STRING_TOKEN(STR_11), value = 11, flags = RESET_REQUIRED;
       endoneof; 

       oneof varid = SETUP_DATA.UartFLREn,
        prompt = STRING_TOKEN(STR_UART_FLR_EN),
        help   = STRING_TOKEN(STR_EMPTY),
        option text = STRING_TOKEN(STR_DISABLED), value = 0, flags = RESET_REQUIRED| DEFAULT | MANUFACTURING;
        option text = STRING_TOKEN(STR_ENABLED),  value = 1, flags = RESET_REQUIRED;
       endoneof; 
      
		endif;//end suppressif
     endform;//end CHIPSET_SB_UART_FORM_ID

  #endif  //end, FORM_SET_FORM
#endif  //end, DEVICES_FORM_SET