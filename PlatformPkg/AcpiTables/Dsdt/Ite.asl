//
//  Copyright (c)  2006 - 2016 Byosoft Corporation.  All rights reserved.
//
//  This software and associated documentation (if any) is furnished under
//  a license and may only be used or copied in accordance with the terms
//  of the license.  Except as permitted by such license, no part of this
//  software or documentation may be reproduced, stored in a retrieval system,
//  or transmitted in any form or by any means without the express written
//  consent of Byosoft Corporation.
//
//*************************************************************
//  Filename:  It8625E.AsL
//
//  ACPI ASL source file for ITE IT8625E Super IO device
//
//
//  In order to get/set the Configuration Mode of the Logical Devices
//  inside of the 18x, it is necessary to access the i/o ports for
//  the Configuration registers.  This section defines the
//  OperationRegion necessary for this i/o.
//
//  The code here defines the "normal" CfgBase of 0x3f0, which overlaps
//  2 unused i/o ports on the Primary FDC.  The FDC device used with
//  this code does *NOT* claim these ports, so there is no conflict.
//  However, there should be code SOMEWHERE that DOES claim these 2
//  ports for the Motherboard so that nothing else tries to use them.
//

/*++
==========================================================================================
      NOTICE: Copyright (c) 2006 - 2016 Byosoft Corporation. All rights reserved.
              This program and associated documentation (if any) is furnished
              under a license. Except as permitted by such license,no part of this
              program or documentation may be reproduced, stored divulged or used
              in a public system, or transmitted in any form or by any means
              without the express written consent of Byosoft Corporation.
==========================================================================================
Module Name:
    Ite.asl

Abstract:
    ACPI ASL source file for ITE IT8625E Super IO device

Revision History:
  ----------------------------------------------------------------------------------------
  Rev   Date        Name    Description
  ----------------------------------------------------------------------------------------
  ----------------------------------------------------------------------------------------
--*/

OperationRegion (
    ITE1,      
    SystemIO,   
    0x02E,      
    2)         


Field ( ITE1,      
        ByteAcc,  
        NoLock,     
        Preserve)  
{
    INDX, 8, 
    DATA, 8 
}


IndexField (INDX,     
            DATA,      
            ByteAcc,    
            NoLock,     
            Preserve)   
{
    
    Offset(7),
    LDN, 8,  

    Offset(0x20),
    DIDR, 8, 

    Offset(0x25),
    GIO1, 8, 

    Offset(0x26),
    GIO2, 8, 

    Offset(0x2A),
    SFSR, 8,

    Offset(0x30),
    ACTR, 8, 

    Offset(0x60),
    IOAH, 8, 
    IOAL, 8, 

    Offset(0x70),
    INTR, 8, 

    Offset(0x72),
    INT1, 8, 

    Offset(0x74),
    DMCH, 8, 

    Offset(0xB0),
    PLS1, 8, 

    Offset(0xB1),
    PLS2, 8, 

    Offset(0xC0),
    SSE1, 8, 

    Offset(0xC1),
    SSE2, 8, 

    Offset(0xC8),
    SSO1, 8, 

    Offset(0xC9),
    SSO2, 8, 

    Offset(0xf0),
    OPT0, 8, 
    OPT1, 8,
    OPT2, 8,
    OPT3, 8,
    OPT4, 8,
    OPT5, 8,
    OPT6, 8,
    Offset(0xf8),
    OPT8, 8,
    OPT9,8,
    OPTA,8,
    OPTB,8

} 

    Mutex(MUT0, 0)   
    Method(ENFG, 0) {       
        Acquire(MUT0, 0xfff)
        Store(0x87, INDX)
        Store(0x01, INDX)
        Store(0x55, INDX)
        Store(0x55, INDX)
    }

    Method(EXFG,0) {       
        Store(0x02, INDX)
        Store(0x02, DATA) 
        Release(MUT0)
    }