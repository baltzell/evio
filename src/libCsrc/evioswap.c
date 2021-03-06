/*
 *  evioswap.c
 *
 *   evioswap() swaps one evio version 2 event
 *       - in place if dest is NULL
 *       - copy to dest if not NULL
 *
 *   swap_int2_t_val() swaps one int32_t, call by val
 *   swap_int32_t() swaps an array of uint32_t's
 *
 *   thread safe
 *
 *
 *   Author: Elliott Wolin, JLab, 21-nov-2003
 *
 *   Notes:
 *     simple loop in swap_xxx takes 50% longer than pointers and unrolled loops
 *     -O is over twice as fast as -g
 *
 *   To do:
 *     use in evio.c, replace swap_util.c
 *
 *   Author: Carl Timmer, JLab, jan-2012
 *      - add doxygen documentation
 *      - add comments & beautify
 *      - simplify swapping routines
 *      - make compatible with evio version 4 (padding info in data type)
 *
 */


/**
 * @file
 * <pre>
 * ################################
 * COMPOSITE DATA:
 * ################################
 *   This is a new type of data (value = 0xf) which originated with Hall B.
 *   It is a composite type and allows for possible expansion in the future
 *   if there is a demand. Basically it allows the user to specify a custom
 *   format by means of a string - stored in a tagsegment. The data in that
 *   format follows in a bank. The routine to swap this data must be provided
 *   by the definer of the composite type - in this case Hall B. The swapping
 *   function is plugged into this evio library's swapping routine.
 *   Here's what it looks like.
 * 
 * MSB(31)                          LSB(0)
 * <---  32 bits ------------------------>
 * _______________________________________
 * |  tag    | type |    length          | --> tagsegment header
 * |_________|______|____________________|
 * |        Data Format String           |
 * |                                     |
 * |_____________________________________|
 * |              length                 | \
 * |_____________________________________|  \  bank header
 * |       tag      |  type   |   num    |  /
 * |________________|_________|__________| /
 * |               Data                  |
 * |                                     |
 * |_____________________________________|
 * </pre>
 * 
 *   The beginning tagsegment is a normal evio tagsegment containing a string
 *   (type = 0x3). Currently its type and tag are not used - at least not for
 *   data formatting.
 *   The bank is a normal evio bank header with data following.
 *   The format string is used to read/write this data so that takes care of any
 *   padding that may exist. As with the tagsegment, the tag, type, & num are ignored.
 */


/* include files */
#include <stdio.h>
#include "evio.h"


/* from Sergey's composite swap library */
extern int eviofmt(char *fmt, unsigned short *ifmt, int ifmtLen);
extern int eviofmtswap(uint32_t *iarr, int nwrd, unsigned short *ifmt, int nfmt, int tolocal, int padding);

/* internal prototypes */
static void swap_bank(uint32_t *buf, int tolocal, uint32_t *dest);
static void swap_segment(uint32_t *buf, int tolocal, uint32_t *dest);
static void swap_tagsegment(uint32_t *buf, int tolocal, uint32_t *dest);
static void swap_data(uint32_t *data, int type, uint32_t length, int tolocal, uint32_t *dest);
static void copy_data(uint32_t *data, uint32_t length, uint32_t *dest);
static int  swap_composite_t(uint32_t *data, int tolocal, uint32_t *dest, uint32_t length);


/**
 * @defgroup swap swap routines
 * These routines handle swapping the endianness (little <-> big) of evio data.
 * @{
 */

/*--------------------------------------------------------------------------*/



/**
 * Is the local host big endian?
 * @return 1 if the local host is big endian, else 0.
 */
int evioIsLocalHostBigEndian() {
    int32_t i = 1;
    return (*((char *) &i) != 1);
}

/**
 * Take 2, 32 bit words and change that into a 64 bit word,
 * taking endianness and swapping into account.
 *
 * @param word1 word which occurs first in memory being read
 * @param word2 word which occurs second in memory being read
 * @param needToSwap true if word arguments are of an endian opposite to this host
 * @return 64 bit word comprised of 2, 32 bit words
 */
uint64_t evioToLongWord(uint32_t word1, uint32_t word2, int needToSwap) {

    uint64_t result;
    int hostIsBigEndian = evioIsLocalHostBigEndian();

    if (needToSwap) {
        // First swap each 32 bit word
        word1 = EVIO_SWAP32(word1);
        word2 = EVIO_SWAP32(word2);

        // If this is a big endian machine ... EVIO_TO_64_BITS(low, high)
        if (hostIsBigEndian) {
            result = EVIO_TO_64_BITS(word1, word2);
        }
        else {
            result = EVIO_TO_64_BITS(word2, word1);
        }
    }
    else if (hostIsBigEndian) {
        result = EVIO_TO_64_BITS(word2, word1);
    }
    else {
        // this host is little endian
        result = EVIO_TO_64_BITS(word1, word2);
    }

    return result;
}

/**
 * Swap an evio version 6 <b>file</b> header in place
 * (but not the following index arrary, etc).
 * Nothing done for NULL arg.
 * @param header pointer to header
 */
void evioSwapFileHeaderV6(uint32_t *header) {
    if (header == NULL) return;

    // Swap everything as 32 bit words
    swap_int32_t(header, EV_HDSIZ_V6, NULL);

    // Now take care of the 64 bit entries:

    // user register
    uint32_t temp = header[8];
    header[8] = header[9];
    header[9] = temp;

    // trailer position
    temp = header[10];
    header[10] = header[11];
    header[11] = temp;
}

/**
 * Swap an evio version 6 <b>record</b> header in place
 * (but not the following index arrary, etc).
 * Nothing done for NULL arg.
 * @param header pointer to header
 */
void evioSwapRecordHeaderV6(uint32_t *header) {
    if (header == NULL) return;

    // Swap everything as 32 bit words
    swap_int32_t(header, EV_HDSIZ_V6, NULL);

    // Now take care of the 64 bit entries:

    // user register 1
    uint32_t temp = header[10];
    header[10] = header[11];
    header[11] = temp;

    // user register 2
    temp = header[12];
    header[12] = header[13];
    header[13] = temp;
}



/**
 * Routine to swap the endianness of an evio event (bank).
 *
 * @param buf     buffer of evio event data to be swapped
 * @param tolocal if 0 buf contains data of same endian as local host,
 *                else buf has data of opposite endian
 * @param dest    buffer to place swapped data into.
 *                If this is NULL, then dest = buf.
 */
void evioswap(uint32_t *buf, int tolocal, uint32_t *dest) {

  swap_bank(buf, tolocal, dest);

  return;
}


/** @} */


/**
 * Routine to swap the endianness of an evio bank.
 *
 * @param buf     buffer of evio bank data to be swapped
 * @param tolocal if 0 buf contains data of same endian as local host,
 *                else buf has data of opposite endian
 * @param dest    buffer to place swapped data into.
 *                If this is NULL, then dest = buf.
 */
static void swap_bank(uint32_t *buf, int tolocal, uint32_t *dest) {

    uint32_t data_length, data_type;
    uint32_t *p=buf;

    /* Swap header to get length and contained type if buf NOT local endian */
    if (tolocal) {
        p = swap_int32_t(buf, 2, dest);
    }
    
    data_length  =  p[0]-1;
    data_type    = (p[1] >> 8) & 0x3f; /* padding info in top 2 bits of type byte */
    
    /* Swap header if buf is local endian */
    if (!tolocal) {
        swap_int32_t(buf, 2, dest);
    }
    
    /* Swap non-header bank data */
    swap_data(&buf[2], data_type, data_length, tolocal, ((dest==NULL) ? NULL: &dest[2]));

    return;
}



/**
 * Routine to swap the endianness of an evio segment.
 *
 * @param buf     buffer of evio segment data to be swapped
 * @param tolocal if 0 buf contains data of same endian as local host,
 *                else buf has data of opposite endian
 * @param dest    buffer to place swapped data into.
 *                If this is NULL, then dest = buf.
 */
static void swap_segment(uint32_t *buf, int tolocal, uint32_t *dest) {

    uint32_t data_length,data_type;
    uint32_t *p=buf;

    /* Swap header to get length and contained type if buf NOT local endian */
    if (tolocal) {
        p = swap_int32_t(buf, 1, dest);
    }
  
    data_length  =  p[0] & 0xffff;
    data_type    = (p[0] >> 16) & 0x3f; /* padding info in top 2 bits of type byte */
  
    /* Swap header if buf is local endian */
    if (!tolocal) {
        swap_int32_t(buf, 1, dest);
    }
  
    /* Swap non-header segment data */
    swap_data(&buf[1], data_type, data_length, tolocal, ((dest==NULL) ? NULL : &dest[1]));
  
    return;
}



/**
 * Routine to swap the endianness of an evio tagsegment.
 *
 * @param buf     buffer of evio tagsegment data to be swapped
 * @param tolocal if 0 buf contains data of same endian as local host,
 *                else buf has data of opposite endian
 * @param dest    buffer to place swapped data into.
 *                If this is NULL, then dest = buf.
 */
static void swap_tagsegment(uint32_t *buf, int tolocal, uint32_t *dest) {

    uint32_t data_length,data_type;
    uint32_t *p=buf;

    /* Swap header to get length and contained type if buf NOT local endian */
    if (tolocal) {
        p = swap_int32_t(buf,1,dest);
    }
    
    data_length  =  p[0] & 0xffff;
    data_type    = (p[0] >> 16) & 0xf; /* no padding info in tagsegments */
    
    /* Swap header if buf is local endian */
    if (!tolocal) {
        swap_int32_t(buf,1,dest);
    }
    
    /* Swap non-header tagsegment data */
    swap_data(&buf[1], data_type, data_length, tolocal, ((dest==NULL)? NULL : &dest[1]));
  
    return;
}



/**
 * Routine to swap any type of evio data.
 *
 * @param data    buffer of evio data to be swapped
 * @param type    type of evio data
 * @param length  length of evio data in 32 bit words
 * @param tolocal if 0 data is of same endian as local host,
 *                else data is of opposite endian
 * @param dest    buffer to place swapped data into.
 *                If this is NULL, then dest = data.
 */
static void swap_data(uint32_t *data, int type, uint32_t length, int tolocal, uint32_t *dest) {
    uint32_t fraglen, l=0;

    /* Swap the data or call swap_fragment */
    switch (type) {

        /* unknown type ... no swap */
        case 0x0:
            copy_data(data, length, dest);
            break;


        /* 32-bit types: uint, float, or int */
        case 0x1:
        case 0x2:
        case 0xb:
            swap_int32_t(data, (unsigned int)length, dest);
            break;


        /* 8-bit types: string array, char, or uchar ... no swap */
        case 0x3:
        case 0x6:
        case 0x7:
            copy_data(data, length, dest);
            break;


        /* 16-bit types: short or ushort */
        case 0x4:
        case 0x5:
            swap_int16_t((uint16_t *) data, length * 2, (uint16_t *) dest);
            break;


        /* 64-bit types: double, int, or uint */
        case 0x8:
        case 0x9:
        case 0xa:
            swap_int64_t((uint64_t*)data, length/2, (uint64_t*)dest);
            break;


        /* Composite type */
        case 0xf:
            swap_composite_t(data, tolocal, dest, length);
            break;
            

        /* bank */
        case 0xe:
        case 0x10:
            while (l < length) {
                /* data is opposite local endian */
                if (tolocal) {
                    /* swap bank */
                    swap_bank(&data[l], tolocal, (dest==NULL) ? NULL : &dest[l]);
                    /* bank was this long (32 bit words) including header */
                    fraglen = (dest==NULL) ? data[l]+1: dest[l]+1;
                } else {
                    fraglen = data[l] + 1;
                    swap_bank(&data[l], tolocal, (dest==NULL) ? NULL : &dest[l]);
                }
                l += fraglen;
            }
            break;


        /* segment */
        case 0xd:
        case 0x20:
            while (l < length) {
                if (tolocal) {
                    swap_segment(&data[l], tolocal, (dest==NULL) ? NULL : &dest[l]);
                    fraglen = (dest==NULL) ? (data[l]&0xffff)+1: (dest[l]&0xffff)+1;
                } else {
                    fraglen = (data[l] & 0xffff) + 1;
                    swap_segment(&data[l], tolocal, (dest==NULL) ? NULL : &dest[l]);
                }
                l += fraglen;
            }
            break;


        /* tagsegment, NOTE: val of 0x40 is no longer used for tagsegment - needed for padding */
        case 0xc:
            while (l < length) {
                if (tolocal) {
                    swap_tagsegment(&data[l], tolocal, (dest==NULL) ? NULL : &dest[l]);
                    fraglen = (dest==NULL)?(data[l]&0xffff)+1:(dest[l]&0xffff)+1;
                } else {
                    fraglen = (data[l] & 0xffff) + 1;
                    swap_tagsegment(&data[l], tolocal, (dest==NULL) ? NULL : &dest[l]);
                }
                l += fraglen;
            }
            break;


        /* unknown type, just copy */
        default:
            copy_data(data, length, dest);
            break;
    }

    return;
}

/* Do we need this for backwards compatibility??? *?
 **
 * This routine swaps the bytes of a 32 bit integer and
 * returns the swapped value. Rewritten to be a wrapper
 * for EVIO_SWAP32(x) macro. Keep it for backwards compatibility.
 *
 * @param val int val to be swapped
 * @return swapped value
 * @deprecated Use the EVIO_SWAP32(x) macro instead of this routine.
 */
/*
int32_t swap_int32_t_value(int32_t val) {
  return EVIO_SWAP32(val);
}
*/


/**
 * @addtogroup swap
 * @{
 */


/**
 * This routine swaps a buffer of 32 bit integers.
 *
 * @param data   pointer to data to be swapped
 * @param length number of 32 bit ints to be swapped
 * @param dest   pointer to where swapped data is to be copied to.
 *               If NULL, the data is swapped in place.
 * @return pointer to beginning of swapped data
 */
uint32_t *swap_int32_t(uint32_t *data, unsigned int length, uint32_t *dest) {

    unsigned int i;

    if (dest == NULL) {
        dest = data;
    }

    for (i=0; i < length; i++) {
        dest[i] = EVIO_SWAP32(data[i]);
    }
    
    return(dest);
}


/**
 * This routine swaps a buffer of 64 bit integers.
 *
 * @param data   pointer to data to be swapped
 * @param length number of 64 bit ints to be swapped
 * @param dest   pointer to where swapped data is to be copied to.
 *               If NULL, the data is swapped in place.
 * @return pointer to beginning of swapped data
 */
uint64_t *swap_int64_t(uint64_t *data, unsigned int length, uint64_t *dest) {

    unsigned int i;

    if (dest == NULL) {
        dest = data;
    }

    for (i=0; i < length; i++) {
        dest[i] = EVIO_SWAP64(data[i]);
    }

    return(dest);
}

 
/**
 * This routine swaps a buffer of 16 bit integers.
 *
 * @param data   pointer to data to be swapped
 * @param length number of 16 bit ints to be swapped
 * @param dest   pointer to where swapped data is to be copied to.
 *               If NULL, the data is swapped in place.
 * @return pointer to beginning of swapped data
 */
uint16_t *swap_int16_t(uint16_t *data, unsigned int length, uint16_t *dest) {

    unsigned int i;

    if (dest == NULL) {
        dest = data;
    }

    for (i=0; i < length; i++) {
        dest[i] = (uint16_t) EVIO_SWAP16(data[i]);
    }

    return(dest);
}


/** @} */


/**
 * This routine swaps nothing, it just copies the given number of 32 bit ints.
 *
 * @param data   pointer to data to be copied
 * @param length number of 32 bit ints to be copied
 * @param dest   pointer to where data is to be copied to.
 *               If NULL, nothing is done.
 */
static void copy_data(uint32_t *data, uint32_t length, uint32_t *dest) {

    uint32_t i;

    if (dest == NULL) {
        return;
    }

    for (i=0; i<length; i++) {
        dest[i] = data[i];
    }
}


/**
 * This routine swaps a single buffer of an array of composite type.
 *
 * @param data    pointer to data to be swapped
 * @param tolocal if 0 data is of same endian as local host,
 *                else data is of opposite endian
 * @param dest    pointer to where swapped data is to be copied to.
 *                If NULL, the data is swapped in place.
 * @param length  length of evio data in 32 bit words
 *
 * @return S_SUCCESS if successful
 * @return S_FAILURE if error swapping data
 */
static int swap_composite_t(uint32_t *data, int tolocal, uint32_t *dest, uint32_t length) {

    char *formatString;
    uint32_t *d, *pData, formatLen, dataLen;
    int nfmt, inPlace, wordLen;
    unsigned short ifmt[1024];
    int64_t len = length;  /* the algorithm below does not guarantee positive length */

    /* swap in place or copy ? */
    inPlace = (dest == NULL);
    d = inPlace ? data : dest;

    /* location of incoming data */
    pData = data;

    /* The data is actually an array of composite data elements,
     * so be sure to loop through all of them. */
    while (len > 0) {
        /* swap format tagsegment header word */
        if (tolocal) {
            pData = swap_int32_t(data, 1, dest);
        }

        /* get length of format string (in 32 bit words) */
        formatLen = pData[0] & 0xffff;

        if (!tolocal) {
            swap_int32_t(data, 1, dest);
        }

        /* copy if needed */
        if (!inPlace) {
            copy_data(&data[1], formatLen, &dest[1]);
        }

        /* set start of format string */
        formatString = (char*)(&pData[1]);

        /* swap data bank header words */
        pData = &data[formatLen+1];
        if (tolocal) {
            pData = swap_int32_t(&data[formatLen+1], 2, &d[formatLen+1]);
        }

        /* get length of composite data (bank's len - 1)*/
        dataLen = pData[0] - 1;

        if (!tolocal) {
            swap_int32_t(&data[formatLen+1], 2, &d[formatLen+1]);
        }

        /* copy if needed */
        if (!inPlace) {
            copy_data(&data[formatLen+3], dataLen, &d[formatLen+3]);
        }

        /* get start of composite data, will be swapped in place */
        pData = &(d[formatLen+3]);

        /* swap composite data: convert format string to internal format, then call formatted swap routine */
        if ((nfmt = eviofmt(formatString, ifmt, 1024)) > 0 ) {
            if (eviofmtswap(pData, dataLen, ifmt, nfmt, tolocal, 0)) {
                printf("swap_composite_t: eviofmtswap returned error, bad arg(s)\n");
                return S_FAILURE;
            }
        }
        else {
            printf("swap_composite_t: error %d in eviofmt\n", nfmt);
            return S_FAILURE;
        }

        /* # of words in this composite data array element */
        wordLen = 1 + formatLen + 2 + dataLen;

        /* go to the next composite data array element */
        len -= wordLen;
        dest += wordLen;
        pData = data += wordLen;
        d = inPlace ? data : dest;

        /* oops, things aren't coming out evenly */
        if (len < 0) {
            return S_FAILURE;
        }
    }

    return S_SUCCESS;
}

