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
#include "hex.h"
#include "rawhex.h"
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <asm/termbits.h>
#include <unistd.h>
#include <string.h>

struct format {
  char* suffix;
  char* description;
  int (*readFile)(FILE* file, Chip* chip);
  int (*writeFile)(FILE* file, Chip* chip);
};

struct format formats[] = {
  { "hex", "Intel Hex format", hexReadFile, hexWriteFile },
  { "raw", "Raw hex characters", rawhexReadFile, rawhexWriteFile },
  { NULL, NULL, NULL, NULL }
};
    
void usage( FILE* f ) {
  fprintf(f, "usage: blitzer [options] [mode]\n");
  fprintf(f, "Options:\n");
  fprintf(f, "-d, --device=name\tname of serial port [/dev/ttyS0]\n");
  fprintf(f, "-h, --help\t\tdisplay this message\n");
  fprintf(f, "-f, --format=id\t\tuse a specific file format\n");
  fprintf(f, "-q, --quiet\t\tsuppress ordinary messages\n");
  fprintf(f, "-v, --verbose\t\tprovide additional messages\n");
  fprintf(f, "-V, --verify\t\tverify writes (program operation only)\n");
  fprintf(f, "Modes:\n");
  fprintf(f, "-p, --program=file\tprogram SX from file\n");
  fprintf(f, "-R, --reset\t\treset SX\n");
  fprintf(f, "-e. --erase\t\terase SX\n");
  fprintf(f, "-r, --read=file\t\tread SX to file\n");
  fprintf(f, "Available file formats are:\n");
  {
    struct format* fmt = formats;
    while ( fmt->suffix ) {
      fprintf(f, "%s\t%s\n", fmt->suffix, fmt->description );
      fmt++;
    }
  }
  fprintf(f, "\n");
}

int main(int argc, char* argv[]) {
  struct option options[] = {
    { "device", 1, NULL, 'd' },
    { "read", 0, NULL, 'r' },
    { "program", 0, NULL, 'p' },
    { "reset", 0, NULL, 'R' },
    { "erase", 0, NULL, 'e' },
    { "help", 0, NULL, 'h' },
    { "format", 0, NULL, 'f' },
    { "quiet", 0, NULL, 'q' },
    { "verbose", 0, NULL, 'v' },
    { "verify", 0, NULL, 'V' },
    { 0, 0, 0, 0 }
  };
  int ov;
  struct termios iosOriginal, ios;
  unsigned char* device_name = "/dev/ttyS0";
  unsigned char* file_name = NULL;
  int mode = 0;
  struct format* format = NULL;
  FILE* file_handle;
  int i;
  Chip* chip;
  int verify = 0;

  while ( (ov=getopt_long(argc,argv,"Vvqr:p:Rehd:f:",options,NULL)) >= 0 ) {
    switch(ov) {
    case 'h':
      usage(stdout);
      exit(0);
      break;
    case 'q':
      debug--;
      break;
    case 'v':
      debug++;
      break;
    case 'V':
      verify++;
      break;
    case 'd':
      device_name = optarg;
      break;
    case 'f':
      {
	struct format* fmt = formats;
	while ( fmt->suffix ) {
	  if (!strcasecmp( fmt->suffix, optarg )) {
	    format = fmt; break;
	  }
	  fmt++;
	}
      }
      if ( format == NULL ) {
	fprintf(stderr,"Format %s not supported\n", optarg);
	usage(stderr);
	exit(-1);
      }
      break;
    case 'r':
    case 'p':
      file_name = optarg;
    case 'R':
    case 'e':
      mode = ov;
      break;
    }
  }

  if ( mode == 0 ) {
    fprintf( stderr, "-r, -R, -p, or -e must be selected\n" );
    usage(stderr);
    exit(-1);
  }

  if ( mode == 'r' || mode == 'p' ) {
    if (!strcmp(file_name,"-")) {
      if ( format == NULL ) {
	fprintf(stderr, "Need to specify format of stdin/stdout\n");
	usage(stderr);
        exit(-1);
      }
      file_handle = stdin;
    } else {
      if ( format == NULL ) {
	struct format* fmt = formats;
	char* suffix = strrchr( file_name, '.');
        if (suffix == NULL) {
          fprintf(stderr, "No format specified and no suffix supplied.  Aborting.\n");
          usage(stderr);
          exit(-1);
        }
        suffix++;
        if (debug >= DEBUG_VERBOSE) 
          fprintf( stderr, "No format specified, looking for suffix %s\n", suffix );
	while ( fmt->suffix ) {
	  if (!strcasecmp( fmt->suffix, suffix )) {
	    format = fmt; break;
	  }
	  fmt++;
	}
      }
      if ( format == NULL ) {
	fprintf(stderr, "Couldn't determine file format of %s\n",file_name);
	usage(stderr);
        exit(-1);
      }
      file_handle = fopen( file_name, "r" );
      if ( file_handle < 0 ) {
	perror( "Couldn't open file" );
      }
    }
  }

  {
    port_handle = open( device_name, O_RDWR | O_NOCTTY );
    if (port_handle < 0) {
      perror( "Opening device" ); return -1;
    }
    
    tcgetattr( port_handle, &iosOriginal );
    
    cfmakeraw( &ios );
    
    bzero( &ios, sizeof(ios) );
    ios.c_cflag = CS8 | CREAD | CLOCAL;
    ios.c_iflag = IXANY;
    ios.c_oflag = 0;
    ios.c_lflag = 0;
    ios.c_cc[VTIME] = 50;
    ios.c_cc[VMIN] = 0;
    cfsetospeed( &ios, B57600 );
    cfsetispeed( &ios, B57600 );
    tcflush( port_handle, TCIFLUSH );
    tcsetattr( port_handle, TCSANOW, &ios );
  }

  if (sx_connect()) {
    fprintf(stderr,"Aborting.\n");
    return -1;
  }

  switch (mode) {

  case 'r':
    chip = (Chip*)malloc( sizeof(Chip) + sizeof(short) * 2048 );
    sx_read(chip);
    sx_reset();

    if (!strcmp(file_name,"-")) {
      file_handle = stdout;
    } else {
      file_handle = fopen( file_name, "w" );
      if ( file_handle < 0 ) {
	perror( "Couldn't open file" );
      }
    }
    // write to file
    format->writeFile( file_handle, chip );
    if ( file_handle != stdout ) fclose( file_handle );
    free(chip);
    break;
  case 'R':
    sx_reset();
    break;
  case 'e':
    sx_erase();
    break;
  case 'p':
    // program
    chip = (Chip*)malloc( sizeof(Chip) + sizeof(short) * 2048 );
    // read from file
    format->readFile( file_handle, chip );
    if ( file_handle != stdout ) fclose( file_handle );
    sx_erase();
    sx_program(chip);
    sx_reset();
    if (verify) {
      Chip* verification_image = (Chip*)malloc( sizeof(Chip) + sizeof(short) * 2048 );
      int vms;
      sx_end();
      if (sx_connect()) {
        fprintf(stderr,"Failed to reconnect for verification operation.\n");
        return -1;
      }
      sx_read( verification_image );
      sx_reset();
      for (vms = 0; vms < chip->mem_size; vms++) {
        if (chip->code[vms] != verification_image->code[vms]) {
          fprintf(stderr, "Verification error: mistmatch at location %X.\n",vms);
          return -1;
        }
      }
      fprintf(stderr,"Verification succeeded-- no errors.\n");
    }
    free(chip);
    break;
  }

  sx_end();
}
