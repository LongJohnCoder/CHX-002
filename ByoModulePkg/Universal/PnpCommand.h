

#ifndef _PNP_COMMAND_H_
#define _PNP_COMMAND_H_

///
/// Identifies the structure-setting operation to be performed.
///
typedef enum {
  ByteChanged,
  WordChanged,
  DoubleWordChanged,
  AddChanged,
  DeleteChanged,
  StringChanged,
  BlockChanged,
  Reseved
} FUNC52_CMD;

#endif
