## Makefile for palm-blitzer

TARGET = blitzer
APPNAME = "Blitzer"
APPID = "pho4"

OBJS = $(TARGET).o serial.o debug.o blitz.o 
LIBS =
IMAGES = img/blitzer.pbm img/blitzersmall.pbm

CC = m68k-palmos-gcc

CFLAGS = -Wall -g -O2 -I.

PILRC = pilrc
OBJRES = m68k-palmos-obj-res
NM = m68k-palmos-nm
BUILDPRC = build-prc
PILOTXFER = pilot-xfer

all: $(TARGET).prc

.S.o:
	$(CC) $(TARGETFLAGS) -c $<

.c.s:
	$(CC) $(CSFLAGS) $<

$(TARGET).prc: code0000.$(TARGET).grc code0001.$(TARGET).grc data0000.$(TARGET).grc pref0000.$(TARGET).grc rloc0000.$(TARGET).grc bin.res
	$(BUILDPRC) $(TARGET).prc $(APPNAME) $(APPID) code0001.$(TARGET).grc code0000.$(TARGET).grc data0000.$(TARGET).grc *.bin pref0000.$(TARGET).grc rloc0000.$(TARGET).grc

code0000.$(TARGET).grc: $(TARGET)
	$(OBJRES) $(TARGET)

code0001.$(TARGET).grc: code0000.$(TARGET).grc

data0000.$(TARGET).grc: code0000.$(TARGET).grc

pref0000.$(TARGET).grc: code0000.$(TARGET).grc

rloc0000.$(TARGET).grc: code0000.$(TARGET).grc

bin.res: $(TARGET).rcp $(IMAGES)
	$(PILRC) $(TARGET).rcp .
	touch bin.res

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LIBS)
	! $(NM) -u $(TARGET) | grep .

send: $(TARGET).prc
	$(PILOTXFER) -i $(TARGET).prc

depend:
	makedepend -Y -I. *.c

install: all
	pilot-xfer -i $(TARGET).prc

clean:
	-rm -f $(OBJS) $(TARGET) *.bin bin.res *.grc Makefile.bak *~ *.pyc

veryclean: clean
	-rm -f $(TARGET).prc pilot.ram pilot.scratch

