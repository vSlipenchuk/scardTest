all: scardTest

CF=-funsigned-char -I /usr/include/PCSC  -I ../vos

scardTest: common.c main.c pcsc_scard.c phoenix_scard.c
		$(CC) -o scardTest  $(CF) common.c main.c pcsc_scard.c phoenix_scard.c \
		-lpscslite
		
		
iccid:
		./scardTest /dev/ttyUSB0 "A0A40000022FE2" "A0B000000A"  exit