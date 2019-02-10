#include "scard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
//#ifdef __APPLE__
#include <PCSC/winscard.h>
#include <PCSC/wintypes.h>
//#else
//#include <winscard.h>
//#endif

SCARDCONTEXT hContext;
SCARD_READERSTATE state;

#define CHECK_ERROR(text) \
    if (err != SCARD_S_SUCCESS) \
        printf("\033[0;31m" text ": %s (0x%08x)\033[00m\n",pcsc_stringify_error(err),err); \
    else \
        timed_log(text ": OK\n");

#define CHECK_EXIT(text) \
    CHECK_ERROR(text) \
    if (err != SCARD_S_SUCCESS) return -1;

static void timed_log(const char *msg)
{
    static struct timeval old_tp;
    struct timeval tp, r;

    gettimeofday(&tp, NULL);

    r.tv_sec = tp.tv_sec - old_tp.tv_sec;
    r.tv_usec = tp.tv_usec - old_tp.tv_usec;
    if (r.tv_usec < 0)
    {
        r.tv_sec--;
        r.tv_usec += 1000000;
    }

    printf("%ld.%.6d %s", r.tv_sec, r.tv_usec, msg);
    old_tp = tp;
}

static int WaitForCardEvent(void)
{
    int insert = 0;

    DWORD err;
    while (1)
    {
        timed_log("Waiting for event...\n");
        err = SCardGetStatusChange(hContext, INFINITE, &state, 1);
        CHECK_EXIT("SCardGetStatusChange")

        timed_log("event detected\n");
        state.dwCurrentState = state.dwEventState;

        if (state.dwEventState & SCARD_STATE_PRESENT)
        {
            if (! (state.dwEventState & SCARD_STATE_MUTE))
            {
                timed_log("card inserted\n");
                if (insert)
                    return 1;
            }
            else
                timed_log("card is mute\n");
        }
        else
        {
            timed_log("card removed\n");
            insert = 1;
        }
    }

    return 0;
}

void PrintResponse(
    BYTE * bReponse,
    DWORD dwLen
    )
{
    printf("Response: ");
    for(int i=0; i<dwLen; i++)
        printf("%02X ", bReponse[i]);
    printf("\n");
}

BYTE ErrorCheck(
    char * string,
    LONG retval
    )
{
    if (SCARD_S_SUCCESS != retval)
    {
        printf("Failed ");
        printf("%s: 0x%08X\n", string, retval);
        return 1;
    }
    return 0;
}

static int is_selected = 0;

read_check2(SCARDHANDLE hCardHandle,SCARD_IO_REQUEST pioSendPci) {

    LONG lReturn = 0;
    BYTE pbRecv[258];
    DWORD dwRecv;
   // Select Applet
    BYTE APDU_Cmd1[] = { 0x00,0xA4,0x04,0x00,0x10,0xFF,0x54,0x44,0x45,0x53,0x20,0x45,0x4E,0x43,0x20,0x00,0x01,0xBF,0xFF,0xFF,0x00 };
    // Cifer
    BYTE APDU_Cmd2[] = { 0x00,0x88,0x10,0x01,0x10,0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA,0xBB,0xCC,0xDD,0xEE,0xFF  };
    // GetResp
    BYTE APDU_Cmd3[] = { 0x00,0xC0,0x00,0x00,0x10 };

    if (!is_selected) {
    printf("Select Applet is_selected=%d.\n",is_selected);
    dwRecv = sizeof(pbRecv);
    lReturn = SCardTransmit(
        hCardHandle,
        &pioSendPci,
        APDU_Cmd1,
        sizeof(APDU_Cmd1),
        NULL,
        pbRecv,
        &dwRecv );
    if(ErrorCheck("SCardTransmit", lReturn))
        return -1;
    PrintResponse(pbRecv, dwRecv);
    is_selected = 1;
    }

        // Select EF ICCID.
     //printf("Call cifer.\n");
    dwRecv = sizeof(pbRecv);
    lReturn = SCardTransmit(
        hCardHandle,
        &pioSendPci,
        APDU_Cmd2,
        sizeof(APDU_Cmd2),
        NULL,
        pbRecv,
        &dwRecv );
    if(ErrorCheck("SCardTransmit", lReturn))
        return -1;
    //PrintResponse(pbRecv, dwRecv);

    // Read EF ICCID.
    //printf("Get cifered code.\n");
    dwRecv = sizeof(pbRecv);
    lReturn = SCardTransmit(
        hCardHandle,
        &pioSendPci,
        APDU_Cmd3,
        sizeof(APDU_Cmd3),
        NULL,
        pbRecv,
        &dwRecv );
    if(ErrorCheck("SCardTransmit", lReturn))
        return -1;
    //PrintResponse(pbRecv, dwRecv);


}

read_check3(SCARDHANDLE hCardHandle,SCARD_IO_REQUEST pioSendPci) {
time_t strt, now, rep;
int sec = 0,i=0;
time(&now); strt=now; rep=now;
for(i=0;i<100;i++) {
   time(&now);
   read_check2( hCardHandle, pioSendPci);
   if (now!=rep) { // do report
      sec++;
      printf("%d in %d sec   pps=%lf \n",i,sec,i*1.0/sec);
      fflush(stdout);
      rep = now; //sec++;
      }

   }
printf("Done %d ciref in %d sec\n",i,sec);
}


read_iccid2(SCARDHANDLE hCardHandle,SCARD_IO_REQUEST pioSendPci) {

    LONG lReturn = 0;
    BYTE pbRecv[258];
    DWORD dwRecv;
   // Select MF
    BYTE APDU_Cmd1[] = { 0xA0, 0xA4, 0x00, 0x00, 0x02, 0x3F, 0x00 };
    // Select EF ICCID
    BYTE APDU_Cmd2[] = { 0xA0, 0xA4, 0x00, 0x00, 0x02, 0x2F, 0xE2 };
    // Read EF ICCID
    BYTE APDU_Cmd3[] = { 0xA0, 0xB0, 0x00, 0x00, 0x0A };
    printf("Select MF.\n");
    dwRecv = sizeof(pbRecv);
    lReturn = SCardTransmit(
        hCardHandle,
        &pioSendPci,
        APDU_Cmd1,
        sizeof(APDU_Cmd1),
        NULL,
        pbRecv,
        &dwRecv );
    if(ErrorCheck("SCardTransmit", lReturn))
        return -1;
    PrintResponse(pbRecv, dwRecv);

        // Select EF ICCID.
    printf("Select EF ICCID.\n");
    dwRecv = sizeof(pbRecv);
    lReturn = SCardTransmit(
        hCardHandle,
        &pioSendPci,
        APDU_Cmd2,
        sizeof(APDU_Cmd2),
        NULL,
        pbRecv,
        &dwRecv );
    if(ErrorCheck("SCardTransmit", lReturn))
        return -1;
    PrintResponse(pbRecv, dwRecv);

    // Read EF ICCID.
    printf("Read EF ICCID.\n");
    dwRecv = sizeof(pbRecv);
    lReturn = SCardTransmit(
        hCardHandle,
        &pioSendPci,
        APDU_Cmd3,
        sizeof(APDU_Cmd3),
        NULL,
        pbRecv,
        &dwRecv );
    if(ErrorCheck("SCardTransmit", lReturn))
        return -1;
    PrintResponse(pbRecv, dwRecv);


}


static int UseCard(const char *mszReaders)
{
    DWORD dwActiveProtocol;
    SCARDHANDLE hCard = 0;

     SCARD_IO_REQUEST pioSendPci;

    timed_log("calling SCardConnect\n");
    DWORD err = SCardConnect(hContext, mszReaders, SCARD_SHARE_SHARED,
        SCARD_PROTOCOL_T0 //|         SCARD_PROTOCOL_T1
        , &hCard, &dwActiveProtocol);
    CHECK_EXIT("SCardConnect")
    timed_log("connected\n");

       switch (dwActiveProtocol)
    {
    case SCARD_PROTOCOL_T0:
        printf("Active protocol T0\n");
        pioSendPci = *SCARD_PCI_T0;
        read_iccid2( hCard,pioSendPci );
        read_check3( hCard,pioSendPci);
        break;

    case SCARD_PROTOCOL_T1:
        printf("Active protocol T1\n");
        pioSendPci = *SCARD_PCI_T1;
        break;

    case SCARD_PROTOCOL_UNDEFINED:
    default:
        printf("Active protocol unnegotiated or unknown\n");
        return -1;
    }

    sleep(3);

#if 1
    timed_log("calling SCardDisconnect\n");
    err = SCardDisconnect(hCard, SCARD_LEAVE_CARD);
    CHECK_ERROR("SCardDisconnect")
#endif

    return 1;
}

int main1(void)  {
    LPSTR mszReaders;
    DWORD err, cchReaders;

    err = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &hContext);
    CHECK_EXIT("SCardEstablishContext")
    cchReaders = 0;

    err = SCardListReaders(hContext, NULL, NULL, &cchReaders);
    CHECK_EXIT("SCardListReaders")
    mszReaders = calloc(cchReaders, sizeof(char));
    if (!mszReaders)
    {
        printf("calloc\n");
        return -1;
    }
    err = SCardListReaders(hContext, NULL, mszReaders, &cchReaders);
    CHECK_EXIT("SCardListReaders")

    printf("Using Reader: %s\n", mszReaders);

    memset(&state, 0, sizeof state);
    state.szReader = mszReaders;
    err = SCardGetStatusChange(hContext, 0, &state, 1);
    CHECK_EXIT("SCardGetStatusChange")

    while (1)
    {
        WaitForCardEvent();

        UseCard(mszReaders);
    }

    SCardReleaseContext(hContext);


    return 0;
}

int scard_pcsc_apdu(scard *s,char *apdu, int len) { // flash apdu?
  LONG err;
  SCARD_IO_REQUEST pioSendPci = *SCARD_PCI_T0;

  DWORD dwRecv = sizeof(s->buf);
    err = SCardTransmit(
        s->hCard,
        &pioSendPci,
        apdu, len,
        NULL,
        s->buf,
        &dwRecv );
    if (err != SCARD_S_SUCCESS) return scard_errorf(s,"SCardTransmit");
    s->blen = dwRecv;
    return 1; // OK
}

int scard_pcsc_open(scard *s,char *name)  {
    LPSTR mszReaders;
    DWORD err, cchReaders;

    err = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, &s->hContext);
    if (err != SCARD_S_SUCCESS) return scard_errorf(s,"SCardExtablishContext");

    cchReaders = 0;
    err = SCardListReaders(s->hContext, NULL, name, &cchReaders);
    if (err != SCARD_S_SUCCESS) return scard_errorf(s,"SCardListReaders");

    mszReaders = calloc(cchReaders, sizeof(char));
    if (!mszReaders) return scard_errorf(s,"calloc failed");
    err = SCardListReaders(s->hContext, NULL, mszReaders, &cchReaders);
    if (err != SCARD_S_SUCCESS) return scard_errorf(s,"SCardListReaders");

    printf("Using Reader: %s\n", mszReaders);
    DWORD dwActiveProtocol;
    //SCARDHANDLE hCard = 0;

    // SCARD_IO_REQUEST pioSendPci;

   // timed_log("calling SCardConnect\n");
    err = SCardConnect(s->hContext, mszReaders, SCARD_SHARE_SHARED,
        SCARD_PROTOCOL_T0 // |         SCARD_PROTOCOL_T1
        , &s->hCard, &dwActiveProtocol);

    if (err != SCARD_S_SUCCESS) return scard_errorf(s,"SCardConnect");

s->apdu = scard_pcsc_apdu; // set a handler
printf("Connected OK Card=%x!\n",s->hCard);
return 1; // ok



/*
    memset(&state, 0, sizeof state);
    state.szReader = mszReaders;
    err = SCardGetStatusChange(s->hContext, 0, &state, 1);
    CHECK_EXIT("SCardGetStatusChange")

    while (1)
    {
        WaitForCardEvent();

        UseCard(mszReaders);
    }

    SCardReleaseContext(hContext);
*/

    return 0;
}

