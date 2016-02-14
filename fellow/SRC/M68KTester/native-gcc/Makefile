CC = m68k-amigaos-gcc -noixemul -s -std=c99
CXX = m68k-amigaos-g++ -noixemul -s -std=c99
CFLAGS = -Os -Wall -fomit-frame-pointer
CXXFLAGS = -Os -Wall -fomit-frame-pointer -I /opt/m68k-amigaos/m68k-amigaos/sys-include -I /opt/m68k-amigaos/vbcc-include -I /usr/include

BINS =  m68k-tester-native.exe

all: $(BINS) $(OBJS)

m68k-tester-native: CCXFLAGS += m68000 -msmall-code
#m68k-tester-native: LDLIBS = -lnix13
m68k-tester-native: LDLIBS = -lsocket
m68k-tester-native: m68k-tester.o m68k-tester-native.o 

clean:
	rm -f $(BINS)
	rm -f *.o *~
