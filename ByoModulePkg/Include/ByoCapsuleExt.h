
#ifndef _BYO_CAPSULE_EXT_
#define _BYO_CAPSULE_EXT_

#define EFI_CAPSULE_EXT_VARIABLE_NAME L"CapsuleUpdateExt"

typedef struct {
    UINT32 CRC32;
    UINT32 Flags;
} BYO_CAPSULE_EXTEND;

extern EFI_GUID gEfiCapsuleVendorGuid;

#endif

