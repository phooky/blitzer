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
#include "serial.h"

int port_handle = -1;

int blitz_recv( unsigned char* buffer, int length ) {
  int last = 1;
  int count = 0;
  while (last && count<length) {
    last = read( port_handle, buffer + count, length - count);
    count += last;
  }
  return count;
}
  
int blitz_send( unsigned char* buffer, int length ) {
  int i = write( port_handle, buffer, length );
  if ( i != length ) return i;
  return blitz_recv( buffer, length );
}
