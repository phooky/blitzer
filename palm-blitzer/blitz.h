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
#ifndef BLITZER_BLITZ_H
#define BLITZER_BLITZ_H

#include <PalmOS.h>

#define CMD_END 0x00
#define CMD_ERASE 0x01
#define CMD_PROGRAM 0x02
#define CMD_READ 0x03
#define CMD_RESET 0x04

#define RSP_SUCCESS 0x00
#define RSP_VPP_FAILED 0x01
#define RSP_CONN_FAILED 0x02
#define RSP_PROG_FAILED 0x03

extern int version;

enum {
  DEBUG_QUIET = 0,
  DEBUG_NORMAL,
  DEBUG_VERBOSE
};

extern int debug;

#define FX_MEMORY_BITS 0x0003

typedef struct {
  UInt16 device;
  UInt16 fusex;  
  UInt16 fuse;
  UInt32 mem_size;
  UInt16 id[16];
  UInt16 code[0];
} Chip;

int sx_connect();

int sx_end();
int sx_erase();
int sx_program( Chip* chip );
int sx_read( Chip* chip );
int sx_reset();

#endif /*BLITZER_BLITZ_H*/
