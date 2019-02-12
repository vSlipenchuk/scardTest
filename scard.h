#ifndef SCARD_H_INCLUDED
#define SCARD_H_INCLUDED

typedef struct _scard {

   int logLevel ;

   void *f;  int taCommand;  // usb-serial handle (on phoenix)
   void *hCard,*hContext; // PCSC

   int eof; // when fihished - card disconnected
   char atr[200]; int alen; // copy of it
   char buf[300]; int blen; // collected buf
   char sw[2];
   char err[80]; // text error of last command
   int (*apdu)(struct _scard *, char *apdu, int len, int expected); //
   } scard;

// Common scard staff


int scard_apdu(scard *, char *apdu, int len,int expected); // exchange apdu
int scard_szapdu(scard *s, char *apdu, int expected); // convert string and run it


// PSCS stuff
int scard_pcsc_open(scard *, char *name ); // opens reader
int scard_pcsc_apdu(scard *, char *apdu, int len,int expected); // exchange apdu

// phoenix stuff

int scard_phoenix_open(scard *s,char *name);
int scard_phoenix_apdu(scard *s,char *apdu, int len,int expected);


#endif // SCARD_H_INCLUDED
