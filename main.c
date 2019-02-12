#include <stdio.h>
#include <stdlib.h>
#include <vos.h>
#include "scard.h"

scard sc;

// smart card - phoenix mode - 9600

void *f ;
char buf[256];
char atr[200];

//int logLevel = 1;
//extern int logLevel;

// online atr decoder: https://smartcard-atr.appspot.com/parse?ATR=3B9E943B9E94801FC78031E073FE211B66D0016BE80C008C

// MTS_ATR: 3B 9E 94 3B 9E 94 80 1F C7 80 31 E0 73 FE 21 1B 66 D0 01 6B E8 0C 00 8C
// CBT: 3B 9F 96 C0 3B 9F 96 80 1F C6 80 31 E0 73 FE 21 13 57 4A 33 05 77 33 33 00 E2

void prn_bin(char *name, char bb) {
unsigned char b = bb;
printf("%s=bx",name); int i;
for(i=7;i>=0;i--) printf("%d",((1<<i)&b)?1:0);
printf("   (h:0x%x, d:%d)\n",b,(unsigned char)b);
}

int print_atr( char *atr, int l) {
char ts = atr[0];
hex_dump("ATR_bin", atr,l);
prn_bin("TS",atr[0]);
switch( atr[0]) {
  case 0x3B:  printf("TS:direct,  0x3B\n"); break;
  case 0x3F:  printf("TS:inverse, 0x3F\n"); break;
  default:    printf("TS:unknown, 0x%d\n",atr[0]); return 0;
  }
char T0 = atr[1]; atr+=2; l-=2;
prn_bin("T0",T0);
int h_len = T0 & 0xF; // last 4 bits
printf("history_length=%d\n",h_len);
int TL = 0; //extra len (by TD)
if ( T0 & (1 << (4+0))) { // have TA
   unsigned char TA = *atr;
   //prn_bin("MS", (1 << (4+0)) );
   prn_bin("TA(1)", *atr); atr++; l--;
   // default TA=11 ->  ‘11’, corresponding to fmax = 5 MHz, Fi = 372, Di = 1.
   printf("di_idx=%d, fi_idx=%d\n",TA&0xF,TA>>4);

   }
if ( T0 & ( 1 << (4+1))) { // have TB
   prn_bin("TB(1)", *atr); atr++; l--;
   }
if ( T0 & (1 << (4+2))) { // have TC
   prn_bin("TC(1)", *atr); atr++; l--;
   }
if ( T0 & (1 << (4+3))) { // have TD
   TL = (*atr) >> 4;
   prn_bin("NextMask",TL);
   prn_bin("TD(1)", *atr); atr++; l--;
   }
if ( TL & (1 << (0+0))) { // have TA
   unsigned char TA = *atr;
   //prn_bin("MS", (1 << (4+0)) );
   prn_bin("TA(2)", *atr); atr++; l--;
   // default TA=11 ->  ‘11’, corresponding to fmax = 5 MHz, Fi = 372, Di = 1.
   printf("di_idx=%d, fi_idx=%d\n",TA&0xF,TA>>4);

   }
if ( TL & ( 1 << (0+1))) { // have TB
   prn_bin("TB(2)", *atr); atr++; l--;
   }
if ( TL & (1 << (0+2))) { // have TC
   prn_bin("TC(2)", *atr); atr++; l--;
   }
if ( TL & (1 << (0+3))) { // have TD
   //TL = (*atr) >> 4;
   prn_bin("TD(2)", *atr); atr++; l--;
   }
// now - hist bytes?
if (h_len && h_len<=l) {
   hex_dump("Histrory_Bytes:",atr,h_len);
   atr+=h_len; l-=h_len;
   }
hex_dump("REST",atr,l);
//klen = T0&

}

int write_echo(void *f,char *buf, int len) { // write and get echoed results
hexdump("WRITE_BEGIN:",buf,len);
while( len>0 ) { // do for all
   char ch = *buf, rch;
   prt_write(f,buf,3); len-=3; buf+=3;
   printf("<%x>\n",ch);
   // now - wait for this byte
   while ( 1) {
       int l = prt_peek(f, &rch, 1);
       printf("<L=%d\n>",l);
       if ( l == 0) msleep(10000);
       if ( l< 0 ) return -1;
       if (l == 1) {
             if (rch!=ch) { printf("WR ANS!\n"); return 0;}
             printf("{%x}",ch);
             break; // OK, next one
             }
       }

   }
return 1; // ALL OK
}





int sc_read2(scard *s,int len,int ta) { // collect len bytes in timeout
s->blen = 0;
while( (!s->eof) && (len>0) && (ta>0) ) {
     int l = prt_peek(s->f,s->buf+s->blen,len+1);
    //int l = prt_peek(s->f,buf,len+1);


   if (l == 0 ) { msleep(10); ta-=10; continue; } // not yet
   if ( l<0 ) { s->eof=1; return -1;} // error

  // memcpy(s->buf+s->blen,buf,len);
   if (s->logLevel>5) hexdump("PEEK",s->buf+s->blen,l);
   s->blen+=l; len-=l; // and again
   }
return len == 0 ; // all read
}


int apdu_send2(char *apdu, int len,int expected) {
//if (logLevel>5)
hexdump("APDU sending:",apdu,len);
//int c = write_echo(f,apdu,2);
//printf("write_echo code=%d\n",c);

//
unsigned char cla=apdu[1];
prt_write(f,apdu,5); //len=0;// ZU
 // now - wait status
int i=0;
sc_read(&sc,5,1000); // read echo
if (sc.blen!=5 || (0!=memcmp(sc.buf,apdu,5))) {
  printf("Fail eat echo code=%d!\n",5);
  return 0;
  }
//if(logLevel>3) printf("==>now - wait CLA %x\n",cla);
sc_read(&sc,1,1000); // wait result exec
//msleep(200);

//sc_read(&sc,1,1000); // read echo
//hex_dump("try1",sc.buf,sc.blen);

//sc_read(&sc,1,1000); // read echo
//hex_dump("try2",sc.buf,sc.blen);

//if (0) { // ignore code?
if ( (sc.blen == 1) && (sc.buf[0]==cla))  {
//   if (logLevel>3) printf("command OK, send a rest %d!\n",len-5);
  } else {
  //printf("%d or %d? %x or %x?\n", (sc.blen == 6) ,(sc.buf[5]==cla),sc.buf[5],cla);
  printf("EXPECT len=%d and cla=%d!!!\n",1,cla);
  hex_dump("WrongHere",sc.buf,sc.blen);
  return -1;
  }
//}
//printf("ok, - now send rest %d bytes\n",len-5);
/*
msleep(100);
while (i<100) {
       int l = prt_peek( f, buf, sizeof(buf) );
       if (l < 0 ) break;
         else if (  l ==0) msleep(10);
          else hex_dump("in1:",buf,l);
          i++;
          msleep(10);
       }
*/
//msleep(100);
prt_write(f,apdu+5,len-5); // send rest
{
char b[200];
sc_read(&sc,len-5,1000);
if ( (sc.blen!=(len-5))   || (0!=memcmp(apdu+5,sc.buf,len-5) ))  {
   hex_dump("Fail eat echo!",sc.buf,sc.blen);
   hex_dump("Expect        ",apdu+5,len-5);
   printf(" I1=%d,I2=%d\n",(sc.blen!=(len-5)),memcmp(apdu+5,sc.buf,len-5));
   return 0;
   }
// expect - we eat all echo?
// ok echo eat
}
//int expected = 100; // result may be here up to...
// 0x60 - wait next byte ....
 sc_read(&sc, 2+expected,400); //  one seconds expects data?
if (sc.blen>=2) { // good one
//   if (logLevel>2) hex_dump("OK resp",sc.buf,sc.blen);
   sc.sw[0]=sc.buf[sc.blen-2];
   sc.sw[1]=sc.buf[sc.blen-1];
   sc.blen-=2; // remove them from result
   //memmove(sc.buf,sc.buf+2,sc.blen);
//   if (logLevel>1) hex_dump("SW",sc.sw,2);
   return 1;
  }
else {
  hex_dump("wrong resp",sc.buf,sc.blen);
  return 0;
  }
/*
if (len>5) {
   printf("SendRest:%d\n",len-5);
   //prt_write(f,apdu+5,len-5); // send rest
   i =0;
   while (i<100) {
       int l = prt_peek( f, buf, sizeof(buf) );
       if (l < 0 ) break;
         else if (  l ==0) msleep(10);
          else { printf("i=%d ",i); hex_dump("in1:",buf,l); }
          i++;
          msleep(10);
       }
  }
*/
//prt_write(f,apdu+2,len-2);

return 1 ;
printf("NowWrite rest\n");
//write_echo(f,apdu+2,len-2);

}

int apdu_send(char *apdu, int len,int expected) {
return apdu_send(apdu,len,1000); // read all expected data
}


/*
reader -> closer to USB is 7Mhz, far is 3Mhz (9600).

 Voron -> 9600...
 3B 3B 94 00 91 38 11 20 01 1B 40 33 33 90 00
ICCID:89 70 03 93 53 46 81 41 11 8

ATR: on 19200 - good
3B 3B 94 00 91 38 11 20 01 1B 40 33 33 90 00
ICCID:8970039353468141118
*/

int szapdu2(char *szapdu,int expected) {
return scard_szapdu(&sc,szapdu,expected);
/*
char buf[256];

// 01 21 51 97 11 68 11 11 90 00
// 10 12 15 79 11 86 11 11 09 00
//#include "../vos/coders.h"
//
//A0 A4 00 00 02
int l = hexstr2bin(buf,szapdu,-1);
apdu_send2(buf,l,expected);
*/
}

int szapdu(char *szapdu) { return szapdu2(szapdu,1000);} // read all expected data

read_iccid( ) {
     // Select EF ICCID
    //BYTE APDU_Cmd2[] = { 0xA0, 0xA4, 0x00, 0x00, 0x02, 0x2F, 0xE2 };
    // Read EF ICCID
    //BYTE APDU_Cmd3[] = { 0xA0, 0xB0, 0x00, 0x00, 0x0A };
    szapdu("A0A4000002 2FE2"); // select ICCID
    //szapdu("A0A4000002 6F40"); // select IMSI
    //printf("  ===== now read it ! ====\n");
    szapdu("A0B000000A"); // }; // read binary, 10 byte
  //  szapdu("A0A4000002 2FE2"); // select ICCID")

}

int sel_once=0;

read_me() {

if (!sel_once) {
  szapdu2("00A4040010FF5444455320454E43200001BFFFFF00",0); // select?
  sel_once=1;
  }
//printf(" --- select OK -----\n");

if (1) { // one 16byte code encryption - can be up to 256 in total, ->? max?
  szapdu2("008810011000112233445566778899AABBCCDDEEFF",0); // ?? encrypt 16 bytes??
  // expect 10 bytes in aout 606110
  szapdu2("00C0000010",16); // get responce
  } else { // 32b
  szapdu2("008810012000112233445566778899AABBCCDDEEFF00112233445566778899AABBCCDDEEFF",1); // ?? encrypt 16 bytes??
  // expect 10 bytes in aout 606110
  szapdu2("00C0000020",32); // get responce*2

  }

  //hex_dump("OK?",sc.sw,2);
return sc.sw[0]=0x90 && sc.sw[1]==0x00; // done OK
  //return sc.sw[0]=
  //szapdu("008810011000112233445566778899AABBCCDDEEFF"); // ?? encrypt??
}

int stress_test() {
time_t strt, now, rep;
int cnt = 0, sec=0;
time(&now); strt = rep = now;
printf("Start stress test now\n");
while( cnt< 1000) {
   time(&now);
   sec = (now-strt);
   if (now!=rep) {
       printf("Done:  %d in %d sec,   avg=%0.2f pps  \r",cnt, sec,(cnt*1.0)/sec);
       fflush(stdout);
       rep = now;
      }
    if (!read_me()) { // do a step
      printf("Failed call:%d on %d sec\n",cnt,sec);
      }
    cnt++;
    //printf("CNT=%d\n",cnt);
   }
return 0;
}


readimsi() {


}

int speed = 9600; /* 19200 close to USB, or 9600 far from USB */

int main2() {
 char APDU_Cmd1[] = { 0xA0, 0xA4, 0x00, 0x00, 0x02, 0x3F, 0x00 };
 scard *s = &sc;
//if (! scard_pcsc_open(s,0)) { // open any card reader
if (!scard_phoenix_open(s,"/dev/ttyUSB0")) { //
  printf("error while scard_open: %s\n",s->err);
  return 0;
  }
/*
printf("card  opened ok alen=%d\n",s->alen);
hexdump("atr",s->atr,s->alen);
print_atr(s->atr,s->alen);
printf("Normal Done\n");
*/
//return 0;



if (!scard_iccid(s)) { //}, APDU_Cmd1, sizeof(APDU_Cmd1))) {
  printf("iccid failed %s\n",s->err);
  return 0;
  }
hexdump("ICCID OK",s->buf,s->blen); // print results

if (!scard_imsi(s)) { //}, APDU_Cmd1, sizeof(APDU_Cmd1))) {
  printf("imsi failed %s\n",s->err);
  return 0;
  }
hexdump("IMSI OK",s->buf,s->blen); // print results

//s->logLevel=5;
 //read_me(); once
 stress_test();

prt_close( s->f);
return 1;
}


int  do_cmd(scard *s,char *c) {
if (lcmp(&c,"logLevel")) {
  s->logLevel=atoi(c);
  return 1;
  }
if (lcmp(&c,"exit")) {
  s->eof=1;
  return 1;
  }
int  ok = scard_szapdu(s,c,100); // till timeout
  if (!ok) printf("ERR:%s\n",s->err);
      else hex_dump("+OK, res:",s->buf,s->blen); // just print results
return 1;
}

int main(int npar,char **par) {
char *dev;
scard *s = &sc;
if (npar<2) {
   fprintf(stderr,"usage: <dev> [command]\n");
   return 1;
   }
dev = par[1];
int ok,i;
ok =  (dev[0]=='/')? scard_phoenix_open(s,dev) : scard_pcsc_open(s,dev);
if (!ok) {
  fprintf(stderr,"dev:%s open error:%s\n",dev,s->err);
  return 2;
  }
//s->logLevel=5;
for(i=2;i<npar;i++) do_cmd(s,par[i]);
char buf[1024];
while(!s->eof  && !feof(stdin)) {
  buf[0]=0;
  gets(buf);
  //printf("<<<%s>>>\n",buf);
  if (!buf[0]) break;
  do_cmd(s,buf);
  }
return 0; // DONE

return main2();

   int l;
//   char buf[256];
    f = prt_open("/dev/ttyUSB0",speed);
    if (!f) return 0; // no port
    prt_config_phoenix2( (int)f, speed);
    printf("Hello world handle=%x!\n",f);
    msleep(500);;/// wait for ATR
    l = prt_peek(f, atr,sizeof(atr));
    if (l<=0) {
       printf("fail get atr\n");
       return 0;
       }
    sc.f=f; // init scard
    print_atr(atr,l);
     //read_iccid();
    //logLevel=10 ; read_me();
    //stress_test();
    szapdu("A0 A4 00 00 02"); // just select
    while (1) {
       int l = prt_peek( f, buf, sizeof(buf) );
       if (l < 0 ) break;
         else if (  l ==0) msleep(10);
          else {
           if (l == 1 && buf[0]==0x60) ; // just wait
            else  hex_dump("in:",buf,l);
          }
       }
    return 0;
}
