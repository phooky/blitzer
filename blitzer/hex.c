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
#include "blitz.h"
#include "hex.h"

int char2nibble( unsigned char c ) {
  if ( c >= '0' && c <= '9' )
    return c - '0';
  else if ( c >= 'A' && c <= 'F' )
    return (c - 'A') + 10;
  else if ( c >= 'a' && c <= 'f' )
    return (c - 'a') + 10;
  return 0;
}

int readByte( char** bptr ) {
  return (char2nibble(*((*bptr)++)) << 4) + char2nibble(*((*bptr)++));
}
  
int readLine( FILE* file ) {
  char buf[128];
  int address, length, type, i;
  char* bptr;
  short* cptr;
  bptr = fgets( buf, 127, file );
  if ( bptr == NULL || *(bptr++) != ':' ) return -1;
  length = readByte( &bptr );
  address = readByte( &bptr ) << 8;
  address += readByte( &bptr );
  type = readByte( &bptr );
  switch (type) {
  case 0:
    if ( address < 0x1000 ) { /* CODE */
      cptr = code + (address/2);
      i = length;
    } else if (address >= 0x1000 && address < 0x1010 ) { /* ID */
      cptr = id + ((address-0x1000)/2);
      i = length;
    } else if (address == 0x1010) {
      cptr = &fuse;
      i = 2;
    } else if (address == 0x1011) {
      cptr = &fusex;
      i = 2;
    } else return 1;
    while ( i ) {
      *cptr = readByte( &bptr );
      *cptr += readByte( &bptr ) << 8;
      cptr++; i -= 2;
    }
    break;
  case 1:
    return 0;
  case 2:
    return 1;
  default:
    return -1;
  }
  return 1;
}

int hexReadFile( FILE* file ) {
  int rv, i;
  mem_size = 2048;
  fuse = 0x77F;
  fusex = 0xFFF;
  for ( i = 0; i < mem_size; i++ )
    code[i] = 0x0FFF;
  
  while ( rv=readLine( file ), rv > 0 );

  return rv;
}

void writeLine( unsigned short* data, int address, int length, FILE* file ) {
  int checksum = 0;
  int i;
  /* don't write an all 0xFF line? */
  if (length == 16) {
    for ( i = 0; i < 8; i++) {
      if ((data[i] & 0xFFF) != 0xFFF) break;
    }
    if ( i == 8 ) return;
  } 
  fprintf(file,":%02X%04X00",length,(address & 0xFFFF));
  checksum += length;
  checksum += address & 0xFF;
  checksum += (address >> 8) & 0xFF;
  while ( length ) {
    int low = *data & 0xFF;
    int high = (*data >> 8) & 0xFF;
    fprintf(file,"%02X%02X",low,high);
    checksum += low + high;
    data++;
    length -= 2;
  }
  fprintf(file,"%02X\n", (-checksum & 0xFF));
}

void writeEndLine( FILE* file ) {
  fprintf(file, ":00000001FF");
}

int hexWriteFile( FILE* file ) {
  int idx = 0;
  while ( idx < mem_size ) {
    writeLine(&code[ idx ], idx*2, 16, file);
    idx += 8;
  }
  writeLine(&fuse, 0x1010, 2, file);
  writeLine(&fuse, 0x1011, 2, file);
  writeEndLine(file);
  return 0;
}


