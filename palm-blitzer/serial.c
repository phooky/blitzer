/* Serial code for palm blitzer */
#include <System/SerialMgr.h>
#include "serial.h"
#include "debug.h"
#include "blitzerRsc.h"

#define SERIAL_TIMEOUT 500
/*  #define SERIAL_OK 0 */
/*  #define SERIAL_ERR_NOLIB 1 */
/*  #define SERIAL_ERR_ALREADY_OPEN 2 */
/*  #define SERIAL_ERR_UNKNOWN_ERR 3 */
Boolean port_open = 0;
UInt16 refNum;

int open_serial() {
  Err rv;
  SerSettingsType ss;
  if (port_open) return SERIAL_OK;
  port_open = 0;
  if (SysLibFind ("Serial Library", &refNum) != 0) return SERIAL_ERR_NOLIB;
  rv = SerOpen(refNum, 0, 57600);
  if (rv == serErrAlreadyOpen) {    
    SerClose(refNum);
    return SERIAL_ERR_ALREADY_OPEN;
  }
  if (rv != 0)
    return SERIAL_ERR_UNKNOWN_ERR;
  ss.baudRate = 57600;
  ss.flags = serSettingsFlagStopBits1 | serSettingsFlagBitsPerChar8;
  ss.ctsTimeout = 0;
  if (SerSetSettings(refNum, &ss) != 0) {
    SerClose(refNum);
    return SERIAL_ERR_UNKNOWN_ERR;
  }    
  port_open = 1;
  return SERIAL_OK;
  
}

int close_serial() {
  if (port_open) {
    SerClose(refNum);
  }
  port_open = 0;
  return SERIAL_OK;
}

int blitz_recv( unsigned char* buffer, int length ) {
  Err err;
  UInt32 recvd = SerReceive(refNum, (void*)buffer, length, SERIAL_TIMEOUT, &err);
  if (err) return 0;
  return recvd;
}

int blitz_send( unsigned char* buffer, int length ) {
  Err err;
  UInt32 recvd;
  UInt32 sent = SerSend(refNum, (void*)buffer, length, &err);
  if (err) return 0;  
  if (SerSendWait(refNum, -1)) {
    msg_error("Error waiting for send. ");
    return 0;
  }
  recvd = SerReceive(refNum, (void*)buffer, length, SERIAL_TIMEOUT, &err);
  if (err) {
    switch( err ) {
    case serErrLineErr:
      msg_error("Serial line error. ");
      break;
    case serErrTimeOut:
      msg_error("Serial timeout. ");
      break;
    }
    return 0;
  }
  if (sent != recvd) return 0;
  return recvd;
}
