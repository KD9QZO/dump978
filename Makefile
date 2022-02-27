#
# Makefile
#


DEFINES += -DOUTPUT_HEX_UPPERCASE

CFLAGS  += -pipe -std=gnu11 -O2 -g -Wall -Werror -Ifec
LDFLAGS += -Wl,-Map=dump978.map -Wl,--cref -Wl,--relax
LIBS    := -lm
CC      := gcc
CXX     := g++


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
	rm -f *~ *.o fec/*.o dump978 uat2json uat2text uat2esnt fec_tests
