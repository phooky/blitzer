#!/usr/bin/python

## blitzer -- code for driving the Parallax SX-Blitz programmer
## Copyright 2001 Adam Mayer

## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.

## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.

## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#
# Adds a given intel hex file to the blitzer DB on your palm.
#

import sys
import string
import struct
from Pyrite.Database import Database
from Pyrite.Blocks import Record
from Pyrite.Store.DLP import DLPStore

def usage():
    print "pbDB.py file [file...]"
    print "path - path to blitzerDB.pdb"
    
print "Waiting for connection on /dev/pilot (press the HotSync button now)..."

store = DLPStore()

try:
    db = store.open("blitzerDB")
except IOError, ioe:
    print "Couldn't find blitzerDB on pilot-- make sure you have installed"
    print "and run blitzer! at least once"
    usage()
    exit

def readByte(line):
    byteStr = line[:2]
    try:
        return (string.atoi(byteStr,16), line[2:])
    except ValueError, v:
        print "Bad str is " + byteStr
        raise ValueError, v

def hexProcess(line):
    "Read in a line of data in inh8s format"
    if line[0] != ':':
        return (0, [])
    line = line[1:]
    print line
    (length, line) = readByte( line )
    print "length is " + str(length)
    (addressHi, line) = readByte( line )
    (addressLo, line) = readByte( line )
    (type, line) = readByte(line)
    data = []
    while length > 0:
        length = length - 2
        (dLo, line) = readByte(line)
        (dHi, line) = readByte(line)
        data.append( dLo + (dHi << 8) )
    return ( ((addressHi << 8) + addressLo), data )
    

class blitzerProgram(Record):
    "A record for the blitzer database"
    def __init__(self, filename):
        self.name = filename
        self.code = [0xFFF]*2048
        self.chipid = [0]*16
        self.fuse = 0
        self.fusex = 0
        file = open(filename)
        for line in map(string.rstrip,file.readlines()):
            (address, data) = hexProcess(line)
            if address < 0x1000:
                self.code[address/2:address/2+len(data)] = data
            elif address == 0x1000:
                self.chipid[:len(data)] = data
            elif address == 0x1010:
                self.fuse = data[0]
            elif address == 0x1011:
                self.fusex = data[0]
        Record.__init__(self)
    def pack(self):
        header = struct.pack(">32sxxHHL",self.name, self.fusex, self.fuse, 2048 )
        for c in self.chipid:
            header = header + struct.pack(">H",c)
        for c in self.code:
            header = header + struct.pack(">H",c)
        return header

db.record_class = blitzerProgram

for file in sys.argv[1:]:
    print "Adding " + str(file) + "..."
    db.append(blitzerProgram(file))
    
#print "DB records count " + str(len(db))

db.close()

print "Operation complete."
