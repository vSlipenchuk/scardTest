#ifndef SCARD_H_INCLUDED
#define SCARD_H_INCLUDED

typedef struct _scard {

   void *f; // usb-serial handle (on phoenix)
   void *hCard,*hContext; // PCSC

   int eof; // when fihished - card disconnected
   char atr[200]; // copy of it
   char buf[300]; int blen; // collected buf
   char sw[2];
   char err[80]; // text error of last command
   int (*apdu)(struct _scard *, char *apdu, int len); //
   } scard;

// Common scard staff


// PSCS stuff
int scard_pcsc_open(scard *, char *name ); // opens reader
int scard_pcsc_apdu(scard *, char *apdu, int len); // exchange apdu

// phoenix stuff


#endif // SCARD_H_INCLUDED
