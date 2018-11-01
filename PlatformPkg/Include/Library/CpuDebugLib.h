
#ifdef ZX_SECRET_CODE   

//**********************************************************************
//**********************************************************************
//**                                                                  **
//**     Copyright (c) 2015 Shanghai Zhaoxin Semiconductor Co., Ltd.  **
//**                                                                  **
//**********************************************************************
//********************************************************************** 

#ifndef __CPU_DEBUG_LIB_H__
#define __CPU_DEBUG_LIB_H__

#define UC_SIZE_256MB  0x0
#define UC_SIZE_512MB  0x1
#define UC_SIZE_1024MB 0x2
#define UC_SIZE_2048MB 0x3

#define FSBC_C2P        BIT0
#define FSBC_P2C        BIT1
#define FSBC_C2M        BIT2
#define FSBC_Asyc       BIT4
#define FSBC_Debug      BIT5
#define FSBC_All        BIT7

#define FSBC_TRIGGER_TYPE_SOCCAP		BIT0
#define FSBC_TRIGGER_TYPE_TRANSACTION	BIT1
#define FSBC_TRIGGER_TYPE_REQUEST_COUNT	BIT2
#define FSBC_TRIGGER_TYPE_NO_REQUEST	BIT3
#define FSBC_TRIGGER_TYPE_APIC			BIT4
#define FSBC_TRIGGER_TYPE_GPIO			BIT5
#define FSBC_TRIGGER_TYPE_USER_STOP		BIT6
#define FSBC_TRIGGER_TYPE_HIF_HANGE		BIT7

#define FSBC_TRIGGER_POSITION_SNAPSHOT_MODE  0x000

typedef enum {
    EnumFsbcTriggerPositionSnapShotMode,
    EnumFsbcTriggerPosition12_5,
    EnumFsbcTriggerPosition25,
    EnumFsbcTriggerPosition37_5,
    EnumFsbcTriggerPosition50,
    EnumFsbcTriggerPosition62_5,
    EnumFsbcTriggerPosition75,
    EnumFsbcTriggerPosition87_5
} FSBC_TRIGGER_POSITION;

typedef enum {
    EnumFsbcTransactionC2P,
    EnumFsbcTransactionC2M,
    EnumFsbcTransactionP2C,
    EnumFsbcTransactionVPITX,
    EnumFsbcTransactionVPIRX
} FSBC_TRIGGER_TRANSACTION;
typedef struct _OTHER_FSBC_TRIGGER_METHOD{
	BOOLEAN	    CPU_FSBC_MISSPACKE_EN;
	BOOLEAN	    CPU_FSBC_TIGPULSE_EN;
	BOOLEAN	    CPU_FSBC_IFSBCSTP_EN;
}OTHER_FSBC_TRIGGER_METHOD;

typedef struct _FSBC_TRIGGER_CONDITION{
	BOOLEAN						IsStreamModeEn;
	BOOLEAN						IsDumpToPCIE;
	BOOLEAN						IsSoccapEn;
	BOOLEAN						IsNeedConfigTriggerCondition;
	FSBC_TRIGGER_POSITION		FsbcTriggerPosition;
	UINT16						TriggerType;
	UINT8						TriggerTransaction;
	BOOLEAN						IsWriteTriggerTransaction;
	UINT8						PciePortNum;
	OTHER_FSBC_TRIGGER_METHOD	OtherTriggerMethod;
}FSBC_TRIGGER_CONDITION;

typedef struct _FSBC_CONFIG_PARA{
	FSBC_TRIGGER_CONDITION		*FsbcTriggerCondition;
	UINT64        				MasterFsbcBase;
	UINT64        				SlaveFsbcBase;
	UINT8						FsbcSize;
	BOOLEAN						IsMasterEn;
	BOOLEAN						IsSlaveEn;	
}FSBC_CONFIG_PARA;

typedef struct _MSR_CONFIG_INFO_DUMP{
	UINT32		ApicId;
	UINT8       SocketId;
	UINT64      MsrAddress;
	UINT64	    MsrValue;
}MSR_CONFIG_INFO_DUMP;  

typedef struct _CPU_CONTEXT{
  UINTN Processor;
  EFI_PHYSICAL_ADDRESS MasterAddress;
  EFI_PHYSICAL_ADDRESS SlaveAddress;
  UINT32 			 NumOfInstPer2Dump;
}CPU_CONTEXT;

VOID EFIAPI fsbc_init (IN OUT VOID  *Buffer);
VOID SetAllMSRFunc_After(IN OUT VOID  *Buffer);
VOID SetAllMSRFunc_Before(IN OUT VOID  *Buffer);
VOID EFIAPI SetMSR (IN OUT VOID  *Buffer);

VOID DumpFsbcMsr1();
VOID DumpTracerMsr1();

#endif
#endif
