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
#include "rawhex.h"

unsigned short hexchar( unsigned char c ) {
  if ( c >= '0' && c <= '9' )
    return c - '0';
  else if ( c >= 'A' && c <= 'F' )
    return (c - 'A') + 10;
  else if ( c >= 'a' && c <= 'f' )
    return (c - 'a') + 10;
  return 0;
}

unsigned short fgetword( FILE* file ) {
  unsigned char buf[3];
  fread( buf, 3, 1, file );  
  return hexchar(buf[0])*256 +
    hexchar(buf[1])*16 +
    hexchar(buf[2]);
}

int rawhexReadFile( FILE* file, Chip* chip ) {
  int i;
  chip->mem_size =2048;
  chip->fusex = fgetword( file );
  chip->fuse = fgetword( file );
  for ( i = 0; i < chip->mem_size; i++ ) chip->code[i] = fgetword( file );
  for ( i = 0; i < 16; i++ ) chip->id[i] = fgetword( file );
  return 0;
}

int rawhexWriteFile( FILE* file, Chip* chip ) {
  int i;
  fprintf(file,"%03X", chip->fusex);
  fprintf(file,"%03X", chip->fuse);
  for ( i = 0; i < chip->mem_size; i++ )
    fprintf(file,"%03X", chip->code[i]);
  for ( i = 0; i < 16; i++ )
      fprintf(file,"%03X", chip->id[i]);
  return 0;
}

