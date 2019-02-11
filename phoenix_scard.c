#include "scard.h"
#include "../vos/vos_com_linux.c"


prt_set_rts(int F_ID, int rts) {
int status;
if (rts) { // set RTS
ioctl(F_ID, TIOCMGET, &status);
status |= TIOCM_RTS;
ioctl(F_ID, TIOCMSET, &status);
} else { // clear RTS
ioctl(F_ID, TIOCMGET, &status);
status &= ~TIOCM_RTS;
ioctl(F_ID, TIOCMSET, &status);
}
}

prt_set_dtr(int F_ID, int rts) {
int status;
if (rts) { // set DTR
ioctl(F_ID, TIOCMGET, &status);
status |= TIOCM_DTR;
ioctl(F_ID, TIOCMSET, &status);
} else { // clear DTR
ioctl(F_ID, TIOCMGET, &status);
status &= ~TIOCM_DTR;
ioctl(F_ID, TIOCMSET, &status);
}
}



void  prt_cfg3(int ttyDev,int baudrate,int hwcontrol,int parity,int stop) {
struct termios term_attr;
//printf("hwcontrol:%d\n",hwcontrol);
tcgetattr(ttyDev, &term_attr); //backup current settings

term_attr.c_iflag = 0;  ;;// IXON | IXOFF;
	term_attr.c_oflag = 0;

	term_attr.c_cflag = baudrate | CS8 | CREAD | parity;
 	  if (hwcontrol) term_attr.c_cflag|=CRTSCTS;
 	  if (stop==2) term_attr.c_cflag|=CSTOPB; // 2 stop bits
	  if (parity) term_attr.c_iflag|=parity;
	  if (parity) term_attr.c_oflag|=parity;


	//term_attr.c_iflag = 0;  ;;// IXON | IXOFF;
	//term_attr.c_oflag = 0;
      //if (parity) term_attr.c_oflag|=parity;
	term_attr.c_lflag = 0;

	memset(&term_attr.c_cc,0,sizeof(term_attr.c_cc));
	term_attr.c_cc[VMIN] = 1;
	term_attr.c_cc[VTIME] = 10;

	//term_attr.c_cc[VSTART] = 11; term_attr.c_cc[VSTOP] = 13;

	cfsetispeed(&term_attr, baudrate);
    cfsetospeed(&term_attr, baudrate);

/*
newtio.c_cflag = baudrate | CS8 | CLOCAL | CREAD;
newtio.c_cflag &= ~CRTSCTS; //disable hw flow control
newtio.c_iflag &= ~(IXON | IXOFF | IXANY); //disable flow control
newtio.c_iflag |= IGNPAR; //ignore parity
newtio.c_oflag = 0;
newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); //raw mode

newtio.c_cc[VMIN] = 1;
newtio.c_cc[VTIME] = 0;
*/

//cfsetspeed(&term_attr, getBaud(baudrate));
//tcflush(ttyDev, TCIFLUSH);
//tcsetattr(ttyDev, TCSANOW, &term_attr);
tcsetattr(ttyDev, TCSAFLUSH, &term_attr);


//clear RTS
//ioctl(ttyDev, TIOCMBIC, &mcs);
}


int prt_reset(int fd) { // ZUZUKA - need atr
  // ?
  return 0;
}


void  prt_config_phoenix(int fd) {
  //prt_cfg2(fd, B9600, 0, PARENB, 2 ); // no flow, even parity, 2 stop bit
  prt_cfg3(fd, B9600, 0, PARENB, 2 ); // no flow, even parity, 2 stop bit
  //prt_reset(fd); // need for phoenix ATR
  prt_set_rts(fd, 0); // off RESET
  prt_set_dtr(fd, 0); // off VCC

  prt_set_dtr(fd, 1); // on VCC
  prt_set_rts(fd, 1); // on RESET
  msleep(100);
  prt_set_rts(fd, 0); // on RESET


 //exit(1);
}


void  prt_config_phoenix2(int fd, int speed) {
  //prt_cfg2(fd, B9600, 0, PARENB, 2 ); // no flow, even parity, 2 stop bit
  switch(speed) {
    case 9600:
        prt_cfg3(fd, B9600, 0, PARENB, 2 ); // no flow, even parity, 2 stop bit
        break;
    case 19200:
        prt_cfg3(fd, B19200, 0, PARENB, 2 ); // no flow, even parity, 2 stop bit
        break;
    default:
        printf("Unexpected speed %d\n",speed);
        prt_cfg3(fd, B9600, 0, PARENB, 2 ); // no flow, even parity, 2 stop bit
//      return ;
        break;
  }
 //prt_reset(fd); // need for phoenix ATR
 if (0) {
  prt_set_rts(fd, 0); // off RESET
  prt_set_dtr(fd, 0); // off VCC

  prt_set_dtr(fd, 1); // on VCC
  prt_set_rts(fd, 1); // on RESET
  msleep(100);
  prt_set_rts(fd, 0); // on RESET
  } else {
  prt_set_rts(fd, 0); // off RESET
  //prt_set_dtr(fd, 0); // off VCC
  //msleep(100);
 //Ŝ prt_set_dtr(fd, 1); // on VCC
  //prt_set_dtr(fd, 0); // off VCC
  //msleep(100);
  //prt_set_dtr(fd, 1); // off VCC
  //msleep(100);
  //prt_set_rts(fd, 1); // on RESET

  //msleep(100);
  prt_set_rts(fd, 0); // on RESET
  prt_set_dtr(fd, 1); // off VCC
  }

}

// now - phoenix internal procs

int logLevel=0;

int sc_read(scard *s,int len,int ta) { // collect len bytes in timeout
s->blen = 0;
while( (!s->eof) && (len>0) && (ta>0) ) {
     int l = prt_peek(s->f,s->buf+s->blen,len+1);
    //int l = prt_peek(s->f,buf,len+1);


   if (l == 0 ) { msleep(10); ta-=10; continue; } // not yet
   if ( l<0 ) { s->eof=1; return -1;} // error

  // memcpy(s->buf+s->blen,buf,len);
   if (logLevel>5) hexdump("PEEK",s->buf+s->blen,l);
   s->blen+=l; len-=l; // and again
   }
return len == 0 ; // all read
}

int scard_phoenix_apdu(scard *s,char *apdu, int len,int expected) {
void *f = s->f;
if (logLevel>5) hexdump("APDU sending:",apdu,len);
unsigned char cla=apdu[1];
prt_write(f,apdu,5); //len=0;// ZU
 // now - wait status
int i=0;
sc_read(s,5,1000); // read echo
if (s->blen!=5 || (0!=memcmp(s->buf,apdu,5))) {
  printf("Fail eat echo code=%d!\n",5);
  return 0;
  }
if(logLevel>3) printf("==>now - wait CLA %x\n",cla);
sc_read(s,1,2000);
if ( (s->blen == 1) && (s->buf[0]==cla))  {
   if (logLevel>3) printf("command OK, send a rest %d!\n",len-5);
  } else {
  //printf("%d or %d? %x or %x?\n", (sc.blen == 6) ,(sc.buf[5]==cla),sc.buf[5],cla);
  printf("EXPECT len=%d and cla=%d!!!\n",1,cla);
  hex_dump("WrongHere",s->buf,s->blen);
  return -1;
  }
  prt_write(f,apdu+5,len-5); // send rest
{
char b[200];
sc_read(s,len-5,1000);
if ( (s->blen!=(len-5))   || (0!=memcmp(apdu+5,s->buf,len-5) ))  {
   hex_dump("Fail eat echo!",s->buf,s->blen);
   hex_dump("Expect        ",apdu+5,len-5);
   printf(" I1=%d,I2=%d\n",(s->blen!=(len-5)),memcmp(apdu+5,s->buf,len-5));
   return 0;
   }
}
//int expected = 100; // result may be here up to...
// 0x60 - wait next byte ....
sc_read(s, 2+expected,400); //  status + expected bytes
return s->blen>=2;
}


int scard_phoenix_open(scard *s,char *name) {
int speed = 9600;
if (!name) name = "/dev/ttyUSB0";
void *f = prt_open(name,speed);
if (!f) return scard_errorf(s,"cant open %s",name);
prt_config_phoenix2( (int)f, speed);
//    printf("Hello world handle=%x!\n",f);
msleep(500);;/// wait for ATR
s->alen = prt_peek(f, s->atr,sizeof(s->atr));
if (s->alen<=0) {
    prt_close(f);
    return scard_errorf(s,"fail get atr");
    }
s->f = f;
s->apdu = scard_phoenix_apdu;
return 1; // OK
}
