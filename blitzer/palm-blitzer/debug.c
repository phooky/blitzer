#include <PalmOS.h>
#include "debug.h"

Char log[2048];
UInt16 logoff = 0;

void msg_error( char* error ) {
  while ((*error != '\0') && logoff < 2048)
    log[logoff++] = *(error++);
  log[logoff] = '\0';
}

void msg_debug( char* debug ) {
  while ((*debug != '\0') && logoff < 2048)
    log[logoff++] = *(debug++);
  log[logoff] = '\0';
}

Char* getLog() {
  return log;
}

void resetLog() {
  logoff = 0;
  log[logoff] = '\0';
}
