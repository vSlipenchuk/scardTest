#include "scard.h"
#include "../vos/vtypes.h"
#include <stdarg.h>
#include "coders.h"

//**common scard utilites **//

int scard_errorf(scard *s, char *fmt, ...) { BUF_FMT(s->err,fmt); return 0; } // vtypes.h

int scard_apdu(scard *s, char *apdu, int len,int expected) {
if (!s->apdu) return scard_errorf(s,"apdu call undefined");
if (s->logLevel>=3) hex_dump("APDU_SEND",apdu,len);
int ok  = s->apdu(s,apdu,len,expected);
if (s->logLevel>=3) hex_dump("APDU_RESP",s->buf,s->blen);
return ok;
}

int scard_select2(scard *s,int df) {
char APDU_Cmd1[] = { 0xA0, 0xA4, 0x00, 0x00, 0x02,    (df>>8)&0xFF, df& 0xFF };     // 2 byte DF
return scard_apdu(s,APDU_Cmd1,sizeof(APDU_Cmd1),0); // read resp as 0x9????? - expect SW1&SW2
}

int scard_iccid(scard *s) {
char APDU_Cmd1[] = { 0xA0, 0xA4, 0x00, 0x00, 0x02,    0x3F, 0x00 };     // select MF
char APDU_Cmd2[] = { 0xA0, 0xA4, 0x00, 0x00, 0x02,    0x2F, 0xE2 };     // Select EF ICCID
char APDU_Cmd3[] = { 0xA0, 0xB0, 0x00, 0x00, 0x0A }; // Read 10 bytes of EF ICCID
return scard_apdu(s,APDU_Cmd1,sizeof(APDU_Cmd1),0) &&
  scard_apdu(s,APDU_Cmd2,sizeof(APDU_Cmd2),0) &&
   scard_apdu(s,APDU_Cmd3,sizeof(APDU_Cmd3),10) ;
}


int scard_imsi(scard *s) {
//char APDU_Cmd1[] = { 0xA0, 0xA4, 0x00, 0x00, 0x02,    0x3F, 0x00 };     // select MF
//char APDU_Cmd2[] = { 0xA0, 0xA4, 0x00, 0x00, 0x02,    0x7F, 0x20 };     // Select DF_GSM
//char APDU_Cmd3[] = { 0xA0, 0xA4, 0x00, 0x00, 0x02,    0x6F, 0x07};     // Select EF_IMSI
char APDU_Cmd4[] = { 0xA0, 0xB0, 0x00, 0x00, 9 }; // Read 13 bytes of EF IMSI
return scard_select2(s,0x3F00) &&
       scard_select2(s,0x7F20) && // DF_GSM
       scard_select2(s,0x6F07) && // DF_IMSI
       scard_apdu(s,APDU_Cmd4,sizeof(APDU_Cmd4),9) ; // read bynary
}


int scard_szapdu(scard *s, char *szapdu,int expected) {
char buf[256];
int l = hexstr2bin(buf,szapdu,-1);
return scard_apdu(s,buf,l,expected);
}
