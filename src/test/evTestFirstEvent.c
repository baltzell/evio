/*-----------------------------------------------------------------------------
 * Copyright (c) 2015 Jefferson Science Associates
 *
 * This software was developed under a United States Government license
 * described in the NOTICE file included as part of this distribution.
 *
 * TJNAF Data Acquisition Group, 12000 Jefferson Ave., Newport News, VA 23606
 *-----------------------------------------------------------------------------
 * 
 * Description:
 *	Event I/O test program
 *	
 * Author:  Carl Timmer, Data Acquisition Group
 *
 */
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "evio.h"


static char *dictionary =
        "<xmlDict>\n"
        "  <dictEntry name=\"regular event\" tag=\"1\"   num=\"1\"/>\n"
        "  <dictEntry name=\"FIRST EVENT\"   tag=\"2\"   num=\"2\"/>\n"
        "</xmlDict>\n";


static uint32_t *makeEvent()
{
    uint32_t *bank;

    bank = (uint32_t *) calloc(1, 9*sizeof(int32_t));
    bank[0] = 8;                         /* event length = 8 */
    bank[1] = 1 << 16 | 0x01 << 8 | 1;   /* tag = 1, num = 1, bank 1 contains 32 bit unsigned ints */
    bank[2] = 8;
    bank[3] = 9;
    bank[4] = 10;
    bank[5] = 11;
    bank[6] = 12;
    bank[7] = 13;
    bank[8] = 14;

    return(bank);
}

static uint32_t *makeFirstEvent()
{
    uint32_t *bank;

    bank = (uint32_t *) calloc(1, 9*sizeof(int32_t));
    bank[0] = 8;                         /* event length = 8 */
    bank[1] = 2 << 16 | 0x01 << 8 | 2;   /* tag = 2, num = 2, bank 1 contains 32 bit unsigned ints */
    bank[2] = 1;
    bank[3] = 2;
    bank[4] = 3;
    bank[5] = 4;
    bank[6] = 5;
    bank[7] = 6;
    bank[8] = 7;

    return(bank);
}

int main()
{
    int handle, status;
    uint32_t *ip, *pBuf;
    uint32_t maxEvBlk = 4;
   // uint64_t split = 230;
    uint64_t split = 100;


    printf("\nEvent I/O tests...\n");
    status = evOpen("/tmp/firstEventTestC.ev", "s", &handle);
    printf ("    Opened /tmp/firstEventTest.ev, status = %d\n", status);

    status = evIoctl(handle, "N", (void *) (&maxEvBlk));
    printf ("    Changed max events/block to %d, status = %#x\n", maxEvBlk, status);

    status = evIoctl(handle, "S", (void *) (&split));
    printf ("    Changed split to %d, status = %#x\n", split, status);

    printf ("    Write dictionary, status = %d\n",status);
    status = evWriteDictionary(handle, dictionary);

    pBuf = makeFirstEvent();

    printf ("    Write first event, status = %d\n",status);
    status = evWriteFirstEvent(handle, pBuf);

    ip = makeEvent();

    printf ("    Write event 1, status = %d\n",status);
    status = evWrite(handle, ip);

    printf ("    Write event 2, status = %d\n",status);
    status = evWrite(handle, ip);

    printf ("    Write event 3, status = %d\n",status);
    status = evWrite(handle, ip);

    status = evClose(handle);
    printf ("    Closed file, status = %d\n\n", status);

    free(pBuf);
    free(ip);
}

