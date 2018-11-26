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
#include "serial.h"
#include "debug.h"
int version = 0;

int debug = DEBUG_NORMAL;

/* Command for establishing connection with the SX-Blitz */
unsigned char connect_str[] = {
  0x00, 0x53, 0x58, 0x2D, 0x4B, 0x65, 0x79 
};

/* Expected response from the SX-Blitz, indicating that
   a connection has been established */
unsigned char expected_rsp_str[] = {
  0x53, 0x58, 0x2D, 0x4B, 0x65, 0x79, 0x37, 0x95
}; 

/* Response codes from the SX-Blitz indicating the
   success or failure of an SX-Blitz operation */
enum response_codes {
  SX_RSP_SUCCESS = 0x00,
  SX_RSP_GEN_FAILED,
  SX_RSP_CONN_FAILED,
  SX_RSP_PROG_FAILED,
  SX_RSP_LAST
};

/* Human-readable strings for interpereting the SX-Blitz
   response codes */
unsigned char* response_strings[SX_RSP_LAST]= {
  "Operation successful",
  "Vpp generation failed",
  "Chip connection failed",
  "Programming failed"
};

unsigned char* decode_response( int response ) {
  if ( response >= 0 && response < SX_RSP_LAST )
    return response_strings[response];
  return "Unrecognized response code";
}

/* Fill a buffer with its bytewise 2's complement */
void twos_comp_buffer( unsigned char* buffer, int length ) {
  while(length--) {
    *buffer ^= 0xFF;
    (*buffer)++;
    buffer++;
  }
}

/* Attempt to recieve the two's complement of a stream of
   bytes from the SX-Blitz.  After the initial connection,
   all bytes received from the SX-Blitz should be
   complemented */
int blitz_recv_tc( unsigned char* buffer, int length ) {
  int i = blitz_recv(buffer,length);
  twos_comp_buffer(buffer,length);
  return i;
}
  
/* Macro for reading a small-endian word from a buffer */
#define READ_WORD(buffer) ( (unsigned int)*(buffer++) | ((unsigned int)*(buffer++) << 8) )

int sx_connect() {
  int i;
  unsigned char response_str[9];

  if ((i=blitz_send(connect_str, 7)) != 7) {
    msg_error("Failed to send connect string to SX-Blitz.");
    return -1;
  }
  if ((i=blitz_recv(response_str, 9)) != 9) {
    response_str[i] = 0;
    msg_error("Failed to recieve response from SX-Blitz.");
    return -1;
  }
  for ( i=0; i < 8; i++ ) {
    if ( expected_rsp_str[i] != response_str[i] ) {
      /* int j; */
      msg_error("Recieved malformed response from SX-Blitz.");
      /*
        for (j = 0; j < 8; j++ ) fprintf(stderr,"%02X",response_str[i]);
        fprintf(stderr,"\n");
      */
      return -1;
    }
  }
  version = response_str[8];
  {
    if (debug >= DEBUG_NORMAL)
      msg_debug("SX-Blitz acknowledged.");
  } 
  return 0;
}

int sx_end() {
  unsigned char command_code = 0x00;
  int i;

  if ((i=blitz_send(&command_code, 1)) != 1) {
    msg_error("Failed to send end code SX-Blitz.");
    return -1;
  }
  return 0;
}

int sx_reset() {
  unsigned char command_code = 0x04;
  unsigned char response_code;
  int i;

  if ((i=blitz_send(&command_code, 1)) != 1) {
    msg_error("Failed to send reset code to SX-Blitz.");
    return -1;
  }
  if ((i=blitz_recv_tc(&response_code, 1)) != 1) {
    msg_error("Couldn't get response code from SX-Blitz.");
    return -1;
  }
  if ( response_code != 0x00 ) {
    msg_error( "Bad response code: " );
    msg_error( decode_response(response_code) );
    return -1;
  }
  if (debug >= DEBUG_NORMAL)
    msg_debug("Reset succeeded.");
  return 0;
}

/* Tell the SX-Blitz that we're done with our read operation. */
int sx_end_read() {
  unsigned char count = 0x00;
  unsigned char retbuf[3];
  int i;

  if ((i=blitz_send(&count, 1)) != 1) {
    msg_error("Failed to send end read to SX-Blitz.");
    return -1;
  }
  /* There's some special goofy magical check we're supposed to do
     here.  Whatever. */
  if ( (i=blitz_recv(retbuf, 3)) != 3 ) {
    msg_error("Failed to recieve end read chunk.");
    return -1;
  }
  return 0;
}

/* Read a chunk of <= 128 bytes of data from the SX into a buffer. */
int sx_read_chunk( UInt16* buffer, unsigned char length ) {
  unsigned char chunk[257];
  unsigned char response_code;
  int bytes = length*2 + 1;
  int i;

  if ( length > 128 ) {
    msg_error("Maximum chunk read of 128 exceeded!");
    return -1;
  }
  if ((i=blitz_send(&length, 1)) != 1) {
    msg_error("Failed to send read length to SX-Blitz.");
    return -1;
  }
  if ( (i=blitz_recv_tc(chunk, bytes )) != bytes ) {
    msg_error( "Failed to read chunk.");
    return -1;
  }
  response_code = chunk[ bytes - 1 ];

  if ( response_code != 0x00 ) {
    msg_error( "Bad response code: ");
    msg_error( decode_response(response_code) );
    return -1;
  }
  /* fill buffer */
  for ( i = 0; i < length; i++ ) {
    buffer[i]=(chunk[2*i] << 8) + chunk[(2*i)+1];
  }
  return 0;
}

int sx_read( Chip* chip ) {
  unsigned char command_code = 0x03;
  unsigned char response_code;
  int i;

  if ((i=blitz_send(&command_code, 1)) != 1) {
    msg_error("Failed to send read code to SX-Blitz.");
    return -1;
  }
  if ((i=blitz_recv_tc(&response_code, 1)) != 1) {
    msg_error("Couldn't get response code from SX-Blitz.");
    return -1;
  }
  if ( response_code != 0x00 ) {
      msg_error( "Bad response code: ");
      msg_error( decode_response(response_code) );
      return -1;
  }
  sx_read_chunk( &(chip->device), 1 );
  sx_read_chunk( &(chip->fusex), 1 );
  sx_read_chunk( &(chip->fuse), 1 );
  switch( chip->device ) {
  case 0x0FCE:
    switch( chip->fusex & FX_MEMORY_BITS ) {
    case 0x0:
      chip->mem_size = 512; break;
    case 0x01:
      chip->mem_size = 1024; break;
    case 0x02:
    case 0x03:
      chip->mem_size = 2048; break;
    }
    chip->mem_size = 2048; // goofiness?
    //if (debug >= DEBUG_NORMAL)
    //fprintf( stdout, "Device is SX20/28 (code size:%d)\n", chip->mem_size );
    {
      int ms = chip->mem_size;
      UInt16* cptr = chip->code;
      while ( ms >= 128 ) {
	sx_read_chunk( cptr, 128 ); cptr += 128; ms -= 128;
        if (debug >= DEBUG_VERBOSE)
          msg_debug( "read chunk (128).");
      }
      if (ms) {
	sx_read_chunk( cptr, ms );
      }

    }
    sx_read_chunk( chip->id, 16 );
    break;
  default:
    msg_error( "Device not recognized.  Sorry; Blitzer doesn't currently support this SX chip." );
  }
  sx_end_read();
  return 0;
}

int sx_erase() {
  unsigned char message[] = { 0x01, 100 };
  unsigned char response_code;
  int i;

  if ((i=blitz_send(message, 2)) != 2) {
    msg_error( "Failed to send erase command to SX-Blitz.");
    return -1;
  }
  if ((i=blitz_recv_tc(&response_code, 1)) != 1) {
    msg_error( "Couldn't get response code from SX-Blitz.");
    return -1;
  }
  if ( response_code != 0x00 ) {
    msg_error( "Bad response code: ");
    msg_error( decode_response(response_code) );
    return -1;
  }
  if (debug >= DEBUG_NORMAL)
  msg_debug( "Erase succeeded. ");
  return 0;
}

int sx_end_write() {
  unsigned char count = 0x00;
  unsigned char retbuf[3];
  int i;

  if ((i=blitz_send(&count, 1)) != 1) {
    msg_error( "Failed to send end read to SX-Blitz.");
    return -1;
  }
  if ( (i=blitz_recv(retbuf, 3)) != 3 ) {
    msg_error( "Failed to recieve end read chunk.");
    return -1;
  }
  return 0;
}

int sx_write_chunk( UInt16* buffer, unsigned char length ) {
  unsigned char chunk[66];
  unsigned char response_code;
  int i, j;
  //char buf[10];
  chunk[0] = length;
  chunk[1] = 100;
  /* fill buffer */
  //t = 0xFFF;
  for ( i = 0; i < length; i++ ) {
    //StrIToH(buf, t);
    //msg_debug(":");
    //msg_debug(buf);
    chunk[ (2*i) + 2 ] = buffer[i] & 0xFF;
    chunk[ (2*i) + 3 ] = (buffer[i] >> 8) & 0x0F;
  }
  
  if ((i=blitz_send(chunk, (length*2)+2)) != (length*2)+2) {
    msg_error( "Failed to send data chunk to SX-Blitz.");
    return -1;
  }
  for ( j = 0; j < 10; j++ ) {
    if ( (i=blitz_recv_tc(&response_code, 1 )) != 1 ) {
      msg_error( "Failed to read response code.");
      return -1;
    } else break;
  }
  if ( j > 9 ) return -1;

  if ( response_code != 0x00 ) {
    msg_error( "Bad response code: ");
    msg_error( decode_response(response_code) );
    return -1;
  }
  return 0;
}

int sx_program( Chip* chip ) {
  unsigned char command_code = 0x02;
  unsigned char response_code;
  int i;

  if ((i=blitz_send(&command_code, 1)) != 1) {
    msg_error( "Failed to send read code to SX-Blitz.");
    return -1;
  }
  if ((i=blitz_recv_tc(&response_code, 1)) != 1) {
    msg_error( "Couldn't get response code from SX-Blitz.");
    return -1;
  }
  if ( response_code != 0x00 ) {
    msg_error( "Bad response code: ");
    msg_error( decode_response(response_code) );
    return -1;
  }

  {
    char buf[10];
    msg_debug("FUSEX: ");
    StrIToH(buf,chip->fusex);
    msg_debug(buf);
  }
  
  if (sx_write_chunk( &(chip->fusex), 1 ) < 0) {
    msg_error("Could not write FUSEX");
    return -1;
  }
  if (sx_write_chunk( &(chip->fuse), 1 ) < 0) {
    msg_error("Could not write FUSE");
    return -1;
  }
  if (debug >= DEBUG_VERBOSE)
    msg_debug("wrote FUSE, FUSEX");
  //msg_debug" wrote FUSEX %03X FUSE %03X\n", chip->fusex, chip->fuse );
  {
    char buf[10];
    msg_debug("memsize is ");
    StrIToA(buf,chip->mem_size);
    msg_debug(buf);
  }
  //return -1;

  {
    int ms = chip->mem_size;
    UInt16* cptr = chip->code;
    int retry = 0;
    ms = 2048;
    //if (debug >= DEBUG_VERBOSE)
      //msg_debug "Writing from memory size %d\n",ms);
    while ( ms >= 32 ) {
      if (sx_write_chunk( cptr, 32 ) == -1) {
        if ( retry > 3 ) {
          msg_error( "Gave up on write.");
          return -1;
        }
        if (debug >= DEBUG_VERBOSE)
          msg_debug( "failed to write chunk, retrying");
        retry++;
      }
      else {
	cptr += 32; ms -= 32; retry = 0;
        if (debug >= DEBUG_VERBOSE)
          msg_debug( "wrote chunk (32).");
      }
    }
    if (ms) {
      sx_write_chunk( cptr, ms );
    }
    
  }
  if (sx_write_chunk( chip->id, 16 ) < 0) {
    msg_error("Could not write ID");
    return -1;
  }
  sx_end_write();
  if (debug >= DEBUG_NORMAL)
    msg_debug("Finished program.");
  return 0;
}

