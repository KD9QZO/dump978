#
# Makefile
#
# This is the Makefile for dump978 -- it will eventually be deprecated, and replaced with the much more flexible and
# versatile CMake build system.
#


PROJ := dump978


DEFINES := -DOUTPUT_HEX_UPPERCASE


STDLVL := gnu11
OPTLVL := 2
DBGLVL := 2


MAPFILE  := $(PROJ).map

INCDIRS  := -I./fec/

CFLAGS   += -pipe
CFLAGS   += -std=$(STDLVL)
CFLAGS   += -O$(OPTLVL)
CFLAGS   += -g$(DBGLVL)
CFLAGS   += $(DEFINES)
CFLAGS   += -Wall -Werror
CFLAGS   += $(INCDIRS)

CPPFLAGS :=

LDFLAGS  += -Wl,-Map=$(MAPFILE)
LDFLAGS  += -Wl,--cref
LDFLAGS  += -Wl,--relax

LIBS     := -lm


CROSS   :=

CC      := $(CROSS)gcc
CXX     := $(CROSS)g++
#CPP    := $(CROSS)cpp
#LD     := $(CROSS)ld
#AS     := $(CROSS)as
AR      := $(CROSS)ar
NM      := $(CROSS)nm
READELF := $(CROSS)readelf
OBJCOPY := $(CROSS)objcopy
OBJDUMP := $(CROSS)objdump
SIZE    := $(CROSS)size



all: dump978 uat2json uat2text uat2esnt extract_nexrad


%.o: %.c *.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@


dump978: dump978.o fec.o fec/decode_rs_char.o fec/init_rs_char.o
	$(CC) -g -o $@ $^ $(LDFLAGS) $(LIBS)


uat2json: uat2json.o uat_decode.o reader.o
	$(CC) -g -o $@ $^ $(LDFLAGS) $(LIBS)


uat2text: uat2text.o uat_decode.o reader.o
	$(CC) -g -o $@ $^ $(LDFLAGS) $(LIBS)


uat2esnt: uat2esnt.o uat_decode.o reader.o
	$(CC) -g -o $@ $^ $(LDFLAGS) $(LIBS)


extract_nexrad: extract_nexrad.o uat_decode.o reader.o
	$(CC) -g -o $@ $^ $(LDFLAGS) $(LIBS)


fec_tests: fec_tests.o fec.o fec/decode_rs_char.o fec/init_rs_char.o
	$(CC) -g -o $@ $^ $(LDFLAGS) $(LIBS)


test: fec_tests
	./fec_tests


clean:
	rm -f *~ *.o fec/*.o dump978 uat2json uat2text uat2esnt fec_tests extract_nexrad
