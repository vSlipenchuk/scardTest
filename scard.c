#include "scard.h"
#include "../vos/vtypes.h"
#include <stdarg.h>

//**common scard utilites **//

int scard_errorf(scard *s, char *fmt, ...) { BUF_FMT(s->err,fmt); return 0; } // vtypes.h

int scard_apdu(scard *s, char *apdu, int len) {
if (!s->apdu) return scard_errorf(s,"apdu call undefined");
hex_dump("SEND",apdu,len);
int ok  = s->apdu(s,apdu,len);
hex_dump("RESP",s->buf,s->blen);
return ok;
}

int scard_iccid(scard *s) {
char APDU_Cmd1[] = { 0xA0, 0xA4, 0x00, 0x00, 0x02, 0x3F, 0x00 };     // select MF
char APDU_Cmd2[] = { 0xA0, 0xA4, 0x00, 0x00, 0x02, 0x2F, 0xE2 };     // Select EF ICCID
char APDU_Cmd3[] = { 0xA0, 0xB0, 0x00, 0x00, 0x0A }; // Read EF ICCID
return scard_apdu(s,APDU_Cmd1,sizeof(APDU_Cmd1)) &&
  scard_apdu(s,APDU_Cmd2,sizeof(APDU_Cmd2)) &&
   scard_apdu(s,APDU_Cmd3,sizeof(APDU_Cmd3)) ;
}
