/* blitzer -- code for driving the Parallax SX-Blitz programmer
 * Copyright 2001 Adam Mayer
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* Main code for palm blitzer */

#include <PalmOS.h>
#include "callback.h"
#include "serial.h"
#include "blitzerRsc.h"
#include "blitz.h"
#include "debug.h"

/* prototype logging debug fns */
void resetLog();
Char* getLog();

typedef struct {
  Char name[32];
  Chip chip;
} ChipRecord;

DmOpenRef db = NULL;
UInt16 selected_idx = noListSelection;

void busyOn() {
  FormType* frm;
  frm = FrmGetFormPtr(FORM_PROGRESS);
  FrmDrawForm(frm);
}

void busyOff() {
  FormType* frm;
  frm = FrmGetFormPtr(FORM_PROGRESS);
  FrmEraseForm(frm);
}

static UInt16 getSelected() {
  FormType* frm;
  ListType* lst;
  frm = FrmGetFormPtr(FORM_MAIN);
  lst = (ListType*)FrmGetObjectPtr(frm,FrmGetObjectIndex(frm, LIST_PROGS));
  return (selected_idx = LstGetSelection(lst));
}
          
/* open SXdf database. */
static Boolean openDB() {
  UInt16 cardNo = 0;
  LocalID localID;
  Char* dbName = "blitzerDB";
  if (db != NULL) return true;
  localID = DmFindDatabase(cardNo, dbName);
  if (localID == 0) {
    UInt32 dbType = ('S' << 24) + ('X' << 16) + ('d' << 8) + 'f';
    UInt32 dbCreator = ('p' << 24) + ('h' << 16) + ('o' << 8) + '4';
    DmCreateDatabase(cardNo, dbName, dbCreator, dbType, false);
    localID = DmFindDatabase (cardNo, dbName);
  }
  db = DmOpenDatabase(cardNo, localID, dmModeReadWrite);
  if (db == 0 ) {
    Char s[10];
    Err err = DmGetLastErr();
    StrIToA(s,err);
    FrmCustomAlert(ALERT_GENERIC_ERROR,"Opening db",s,NULL);
  }
  return true;
}

static Boolean closeDB() {
  if ( db == NULL ) return false;
  DmCloseDatabase( db );
  return true;
}

static Boolean readSX() {
  Boolean rv = true;
  ChipRecord* chipRec = (ChipRecord*)MemPtrNew(sizeof(ChipRecord) + sizeof(UInt16)*2048);
  Char* opname = "Read";
  Err err;
  busyOn();
  StrCopy(chipRec->name, "New SX Read");
  resetLog();
  if ((err=open_serial()) == 0) {
    if ( (!sx_connect()) && (!sx_read(&(chipRec->chip))) && (!sx_reset()) ) {
        sx_end();
        FrmCustomAlert(ALERT_GENERIC_SUCCESS,"Read SX successfully.",NULL,NULL);
    } else {
      FrmCustomAlert(ALERT_GENERIC_ERROR,opname,getLog(),NULL);
      rv = false;
    }
    close_serial();
  } else {
    FrmCustomAlert(ALERT_GENERIC_ERROR,opname,"Couldn't open serial port",NULL);
    rv = false;
  }
  if (rv == true) {
    UInt16 position = dmMaxRecordIndex;
    MemHandle record = DmNewRecord(db, &position,
                                  sizeof(ChipRecord) + sizeof(UInt16)*2048);
    ChipRecord* chipRec2 = (ChipRecord*)MemHandleLock( record );
    {
      char buf[10];
      msg_debug("memsize ");
      StrIToA(buf,chipRec->chip.mem_size);
      msg_debug(buf);
      FrmCustomAlert(ALERT_GENERIC_SUCCESS,getLog(),NULL,NULL);
      resetLog();
    }
    DmWrite(chipRec2, 0, chipRec, sizeof(ChipRecord) + sizeof(UInt16)*2048);
    MemHandleUnlock( record );
    DmReleaseRecord(db, position, true);
  }
  MemPtrFree( chipRec );
  busyOff();
  return rv;
}

static Boolean writeSX() {
  Boolean rv = true;
  ChipRecord* chipRec;
  Char* opname = "Write";
  Err err;
  MemHandle rec;
  busyOn();
  debug = DEBUG_VERBOSE;
  if ( getSelected() == noListSelection ) {
    FrmCustomAlert(ALERT_GENERIC_ERROR,opname,"No program selected!",NULL);
    return false;
  }
  rec = DmGetRecord(db,selected_idx);
  chipRec = MemHandleLock( rec );
  resetLog();
  //FrmCustomAlert(ALERT_GENERIC_ERROR,opname,chipRec->name,NULL);
  if (chipRec->chip.mem_size == 0) {
    UInt32 fixmem = 2048;
    msg_debug(" Fixing memsize... ");
    //StrIToA(buf,chipRec->chip.mem_size);
    //msg_debug(buf);
    DmWrite( chipRec, (UInt32)&(((ChipRecord*)0)->chip.mem_size), &fixmem, sizeof(fixmem) );
    FrmCustomAlert(ALERT_GENERIC_SUCCESS,getLog(),NULL,NULL);
    resetLog();
  }
  if ((err=open_serial()) == 0) {
    if ( (!sx_connect()) && (!sx_erase()) && (!sx_program(&(chipRec->chip))) && (!sx_reset()) ) {
      sx_end();
      FrmCustomAlert(ALERT_GENERIC_SUCCESS,"Wrote SX successfully.",NULL,NULL);
      FrmCustomAlert(ALERT_GENERIC_SUCCESS,getLog(),NULL,NULL);
    } else {
      FrmCustomAlert(ALERT_GENERIC_ERROR,opname,getLog(),NULL);
      rv = false;
    }
    close_serial();
  } else {
    FrmCustomAlert(ALERT_GENERIC_ERROR,opname,"Couldn't open serial port",NULL);
    rv = false;
  }
  MemHandleUnlock( rec );
  DmReleaseRecord(db, selected_idx, false);
  debug = DEBUG_NORMAL;
  busyOff();
  return rv;
}

static Boolean eraseSX() {
  Boolean rv = true;
  Char* opname = "Erase";
  Err err;
  busyOn();
  resetLog();
  if ((err=open_serial()) == 0) {
    if ( (!sx_connect()) && (!sx_erase()) && (!sx_reset()) ) {
      sx_end();
      FrmCustomAlert(ALERT_GENERIC_SUCCESS,"SX erased.",NULL,NULL);
    } else {
      FrmCustomAlert(ALERT_GENERIC_ERROR,opname,getLog(),NULL);
      rv = false;
    }
    close_serial();
  } else {
    FrmCustomAlert(ALERT_GENERIC_ERROR,opname,"Couldn't open serial port",NULL);
    rv = false;
  }
  busyOff();
  return rv;
}

static Boolean resetSX() {
  Boolean rv = true;
  Char* opname = "Reset";
  Err err;
  busyOn();
  resetLog();
  if ((err=open_serial()) == 0) {
    if ( (!sx_connect()) && (!sx_reset()) ) {
      sx_end();
      FrmCustomAlert(ALERT_GENERIC_SUCCESS,"SX reset.",NULL,NULL);
    } else {
      FrmCustomAlert(ALERT_GENERIC_ERROR,opname,getLog(),NULL);
      rv = false;
    }
    close_serial();
  } else {
    FrmCustomAlert(ALERT_GENERIC_ERROR,opname,"Couldn't open serial port",NULL);
    rv = false;
  }
  busyOff();
  return rv;
}


static Boolean loadTable() {
  Char** table = NULL;
  UInt16 count;
  if ( db == NULL ) {
    return false;
  }
  count = DmNumRecords(db);
  if (count > 0) {
    UInt16 idx = 0;
    Char** tptr;
    Char*  nptr;
    table = (Char**)MemPtrNew( count * (32 + sizeof(Char*)) );
    if (table == 0) {
      FrmCustomAlert(ALERT_GENERIC_ERROR,"Opening db",
                     "Not enough available memory-- aborting",NULL);
    }
    tptr = table;
    nptr = (Char*)(table + count);
    while (idx < count) {
      int i;
      MemHandle recHdl = DmGetRecord(db, idx);
      ChipRecord* chiprec = (ChipRecord*)MemHandleLock(recHdl);
      *(tptr++) = nptr;
      for (i = 0; i < 32; i++) {
        *(nptr++) = chiprec->name[i];
      }
      MemHandleUnlock(recHdl);
      DmReleaseRecord(db, idx, false);
      idx++;
    }
  } else count = 0;
  {
    FormPtr frm;
    ListType* lst;
    UInt16 listIdx;
    frm = FrmGetFormPtr(FORM_MAIN);
    listIdx = FrmGetObjectIndex(frm, LIST_PROGS);
    lst = (ListType*)FrmGetObjectPtr(frm,listIdx);
    LstSetListChoices(lst, table, count);
    LstDrawList(lst);
    //FrmDrawForm(frm);
  }
  if (count != 0) MemPtrFree(table);
  return true;
}

static Boolean MainFormHandleEvent (EventPtr e)
{
  Boolean handled = false;
  FormPtr frm = NULL;
  Char* opname;
    
  CALLBACK_PROLOGUE

    switch (e->eType) {
    case frmOpenEvent:
      frm = FrmGetActiveForm();
      loadTable();
      FrmDrawForm(frm);

      handled = true;
      break;

    case menuEvent:
      MenuEraseStatus(NULL);
      opname = "Contacting SX-Blitz";
      switch(e->data.menu.itemID) {
        Err err;
      case MI_TEST_BLITZ:
        if ((err=open_serial()) == 0) {
          resetLog();
          if((err = sx_connect()) == 0) {
            if((err = sx_end()) == 0) {
              FrmCustomAlert(ALERT_GENERIC_SUCCESS,"Contacted blitz successfully.",NULL,NULL);
            } else {
              FrmCustomAlert(ALERT_GENERIC_ERROR,opname,"Couldn't end session",NULL);
            }
          } else {
            FrmCustomAlert(ALERT_GENERIC_ERROR,opname,getLog(),NULL);
          }
          if ((err=close_serial()) != 0) {
            FrmAlert(ALERT_SERIAL_NOCLOSE);
          }
        }
        break;
      case MI_TEST_SERIAL:
        opname = "Testing serial port";
        if ((err=open_serial()) == 0) {
          if ((err=close_serial()) == 0) {
            FrmCustomAlert(ALERT_GENERIC_SUCCESS,"Serial operation seems OK.",NULL,NULL);
          } else {
            FrmAlert(ALERT_SERIAL_NOCLOSE);
          }
        } else {
          Char buffer[128];
          if (err > 3) err = 3;
          SysStringByIndex(STRTAB_ST_ERRORS, err, buffer, 128);
          FrmCustomAlert(ALERT_GENERIC_ERROR,opname,buffer,NULL);
        }
        break;
      case MI_READ:
        readSX();
        loadTable();
        break;
      case MI_ERASE:
        eraseSX();
        break;
      case MI_RESET:
        resetSX();
        break;
      case MI_WRITE:
        writeSX();
        break;
      case MI_DETAILS:
        if ( getSelected() != noListSelection )
          FrmPopupForm(FORM_DETAILS);
        break;
      case MI_DELETE:
        if ( getSelected() != noListSelection )
          DmDeleteRecord( db, selected_idx );
      }
      handled = true;
      break;
      
    case winEnterEvent:
      if (e->data.winEnter.enterWindow == (WinHandle)FrmGetFormPtr(FORM_MAIN))
        break;
    case winExitEvent:
      if (e->data.winExit.exitWindow == (WinHandle)FrmGetFormPtr(FORM_MAIN))
        break;
    case ctlSelectEvent:
      frm = FrmGetActiveForm();
      switch(e->data.ctlSelect.controlID) {
      case BU_READ:
        readSX();
        loadTable();
        break;
      case BU_WRITE:
        writeSX();
        break;
      case BU_DETAILS:
        {
          ListType* lst;
          lst = (ListType*)FrmGetObjectPtr(frm,FrmGetObjectIndex(frm, LIST_PROGS));
          if ( (selected_idx = LstGetSelection(lst)) != noListSelection )
            FrmPopupForm(FORM_DETAILS);
        }
        break;
      }
      break;

    case nilEvent:
      // here goes the meat
    default:
      break;
    }

  CALLBACK_EPILOGUE

    return handled;
}

static Boolean DetailsFormHandleEvent (EventPtr e)
{
  Boolean handled = false;
  FormPtr frm;
  FieldType* fld;
  
  CALLBACK_PROLOGUE
    
    frm = FrmGetActiveForm();
  
  switch (e->eType) {
  case frmOpenEvent:
    fld = (FieldType*)FrmGetObjectPtr(frm,FrmGetObjectIndex(frm, FIELD_NAME));
    FldSetText(fld,DmGetRecord(db,selected_idx),0,32);
    FrmDrawForm(frm);
    handled = true;
    break;
  case winEnterEvent:
    if (e->data.winEnter.enterWindow == (WinHandle)FrmGetFormPtr(FORM_DETAILS))
      break;
  case winExitEvent:
    if (e->data.winExit.exitWindow == (WinHandle)FrmGetFormPtr(FORM_DETAILS))
      break;
  case ctlSelectEvent:
    switch(e->data.ctlSelect.controlID) {
    case BU_CLOSE:
      fld = (FieldType*)FrmGetObjectPtr(frm,FrmGetObjectIndex(frm, FIELD_NAME));
      FldSetText(fld,NULL,0,0);
      DmReleaseRecord(db, selected_idx, false);
      FrmReturnToForm(FORM_MAIN);
      loadTable();
      handled = true;
      break;
    case BU_DELETE:
      fld = (FieldType*)FrmGetObjectPtr(frm,FrmGetObjectIndex(frm, FIELD_NAME));
      FldSetText(fld,NULL,0,0);
      DmReleaseRecord(db, selected_idx, false);
      DmRemoveRecord(db, selected_idx);
      selected_idx = noListSelection;
      FrmReturnToForm(FORM_MAIN);
      loadTable();
      handled = true;
      break;
    }
    break;

  case nilEvent:
  default:
    break;
  }

  CALLBACK_EPILOGUE

    return handled;
}

static Boolean ApplicationHandleEvent(EventPtr e)
{
  FormPtr frm;
  Int16    formId;
  Boolean handled = false;

  if (e->eType == frmLoadEvent) {
    formId = e->data.frmLoad.formID;
    frm = FrmInitForm(formId);
    FrmSetActiveForm(frm);
        
    switch(formId) {
    case FORM_MAIN:
      FrmSetEventHandler(frm, MainFormHandleEvent);
      break;
    case FORM_DETAILS:
      FrmSetEventHandler(frm, DetailsFormHandleEvent);
    }
    handled = true;
  }
  return handled;
}

/* Get preferences, open (or create) app database */
static Int16 StartApplication(void)
{
  FrmInitForm(FORM_PROGRESS);
  FrmGotoForm(FORM_MAIN);
  openDB();
  return 0;
}

/* Save preferences, close forms, close app database */
static void StopApplication(void)
{
  closeDB();
  FrmSaveAllForms();
  FrmCloseAllForms();
}

/* The main event loop */
static void EventLoop(void)
{
  Int16 err;
  EventType e;

  do {
    EvtGetEvent(&e, -1);
    if (! SysHandleEvent (&e))
      if (! MenuHandleEvent (NULL, &e, &err))
        if (! ApplicationHandleEvent (&e))
          FrmDispatchEvent (&e);
  } while (e.eType != appStopEvent);
}

/* Main entry point; it is unlikely you will need to change this except to
   handle other launch command codes */
UInt32 PilotMain(UInt16 cmd, void *cmdPBP, UInt16 launchFlags) {
  Int16 err;

  if (cmd == sysAppLaunchCmdNormalLaunch) {

    err = StartApplication();
    if (err) return err;

    EventLoop();
    StopApplication();

  } else {
    return sysErrParamErr;
  }

  return 0;
}
