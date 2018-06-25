
//
// JEP106, Standard Manufacturer's Identification Code.
//
#ifndef _JEP_106H_
#define _JEP_106H_

// Following definition is based on the document JEP106AV, July 2017.
#define JEP106_MAX_BANK_NUM       9    // bank0 is the first bank.
#define JEP106_BANK_MAX_ITEM_NUM  126  // 0x01 - 0x7e
#define JEP106_MAX_STRING_LEN     64   // Max string length in the list. 
                                       // This value should be updated when JEP106 list updating.
                                       // make sure that the string length less than this value.

CHAR8 * Jep106GetManufacturerString(
  UINT8  Bank,
  UINT8  Id
);

#endif
