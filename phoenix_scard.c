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
  prt_set_rts(fd, 0); // off RESET
  prt_set_dtr(fd, 0); // off VCC

  prt_set_dtr(fd, 1); // on VCC
  prt_set_rts(fd, 1); // on RESET
  msleep(100);
  prt_set_rts(fd, 0); // on RESET


 //exit(1);
}

