#include <PalmOS.h>
#include <Core/System/SerialMgrOld.h>
#include "types.h"

enum {
  SERIAL_OK = 0,
  SERIAL_ERR_NOLIB,
  SERIAL_ERR_ALREADY_OPEN,
  SERIAL_ERR_UNKNOWN_ERR,
  SERIAL_ERR_LAST
};

int open_serial();
int close_serial();
int blitz_recv( unsigned char* buffer, int length );
int blitz_send( unsigned char* buffer, int length );



