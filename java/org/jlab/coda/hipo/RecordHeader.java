/*
 *   Copyright (c) 2016.  Jefferson Lab (JLab). All rights reserved. Permission
 *   to use, copy, modify, and distribute  this software and its documentation for
 *   educational, research, and not-for-profit purposes, without fee and without a
 *   signed licensing agreement.
 */
package org.jlab.coda.hipo;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * Generic HEADER class to compose a record header or file header
 * with given buffer sizes and paddings and compression types.<p>
 *
 * <pre>
 * GENERAL RECORD HEADER STRUCTURE ( 56 bytes, 14 integers (32 bit) )
 *
 *    +----------------------------------+
 *  1 |         Record Length            | // 32bit words, inclusive
 *    +----------------------------------+
 *  2 +         Record Number            |
 *    +----------------------------------+
 *  3 +         Header Length            | // 14 (words)
 *    +----------------------------------+
 *  4 +       Event (Index) Count        |
 *    +----------------------------------+
 *  5 +      Index Array Length          | // bytes
 *    +-----------------------+---------+
 *  6 +       Bit Info        | Version  | // version (8 bits)
 *    +-----------------------+----------+
 *  7 +      User Header Length          | // bytes
 *    +----------------------------------+
 *  8 +          Magic Number            | // 0xc0da0100
 *    +----------------------------------+
 *  9 +     Uncompressed Data Length     | // bytes
 *    +------+---------------------------+
 * 10 +  CT  |  Data Length Compressed   | // CT = compression type (4 bits)
 *    +----------------------------------+
 * 11 +        General Register 1        | // UID 1st (64 bits)
 *    +--                              --+
 * 12 +                                  |
 *    +----------------------------------+
 * 13 +        General Register 2        | // UID 2nd (64 bits)
 *    +--                              --+
 * 14 +                                  |
 *    +----------------------------------+
 *
 * -------------------
 *   Compression Type
 * -------------------
 *     0  = none
 *     1  = LZ4 fastest
 *     2  = LZ4 best
 *     3  = gzip
 *
 * -------------------
 *   Bit Info Word
 * -------------------
 *     0-7  = version
 *     8    = true if dictionary is included (relevant for first record only)
 *     9    = true if this record is the last in file or stream
 *    10-13 = type of events contained: 0 = ROC Raw,
 *                                      1 = Physics
 *                                      2 = PartialPhysics
 *                                      3 = DisentangledPhysics
 *                                      4 = User
 *                                      5 = Control
 *                                     15 = Other
 *    14-19 = reserved
 *    20-21 = pad 1
 *    22-23 = pad 2
 *    24-25 = pad 3
 *    26-27 = reserved
 *    28-31 = general header type: 0 = Evio record,
 *                                 3 = Evio file trailer
 *                                 4 = HIPO record,
 *                                 7 = HIPO file trailer
 *
 *
 *
 * ------------------------------------------------------------
 * ------------------------------------------------------------
 *
 * FILE HEADER STRUCTURE ( 56 bytes, 14 integers (32 bit) )
 *
 *    +----------------------------------+
 *  1 |              ID                  | // HIPO: 0x43455248, Evio: 0x4556494F
 *    +----------------------------------+
 *  2 +          File Number             | // split file #
 *    +----------------------------------+
 *  3 +         Header Length            | // 14 (words)
 *    +----------------------------------+
 *  4 +      Record (Index) Count        |
 *    +----------------------------------+
 *  5 +      Index Array Length          | // bytes
 *    +-----------------------+---------+
 *  6 +       Bit Info        | Version  | // version (8 bits)
 *    +-----------------------+----------+
 *  7 +      User Header Length          | // bytes
 *    +----------------------------------+
 *  8 +          Magic Number            | // 0xc0da0100
 *    +----------------------------------+
 *  9 +     Uncompressed Data Length     | // 0 (bytes)
 *    +------+---------------------------+
 * 10 +  CT  |  Data Length Compressed   | // CT = compression type (4 bits)
 *    +----------------------------------+
 * 11 +         Trailer Position         | // File offset to trailer head (64 bits).
 *    +--                              --+ // 0 = no offset available or no trailer exists.
 * 12 +                                  |
 *    +----------------------------------+
 * 13 +          User Register 1         |
 *    +----------------------------------+
 * 14 +          User Register 2         |
 *    +----------------------------------+
 *
 * -------------------
 *   Bit Info Word
 * -------------------
 *     0-7  = version
 *     8    = true if dictionary is included (relevant for first record only)
 *     9    = true if this file has "first" event (in every split file)
 *    10    = File trailer with index array exists
 *    11-19 = reserved
 *    20-21 = pad 1
 *    22-23 = pad 2
 *    24-25 = pad 3 (always 0)
 *    26-27 = reserved
 *    28-31 = general header type: 1 = Evio file
 *                                 2 = Evio extended file
 *                                 5 = HIPO file
 *                                 6 = HIPO extended file
 *
 * </pre>
 *
 * @author gavalian
 */
public class RecordHeader {

        /** Help find number of bytes to pad data. */
        private static int[] padValue = {0,3,2,1};

        private long            position = 0L;
        private int          recordLength = 0;
        private int          recordNumber = 0;
        /** Event or index count. 4th position. */
        private int               entries = 0;
        private int          headerLength = 0;
        private int      userHeaderLength = 0;
        /** Length of index array in bytes. 5th position. */
        private int           indexLength = 0;
        private int            dataLength = 0;
        private int  compressedDataLength = 0;
        private int       compressionType = 0;
            /*
             * These quantities are updated automatically
             * when the lengths are set.
             */
        private int        userHeaderLengthPadding = 0;
        private int              dataLengthPadding = 0;
        private int    compressedDataLengthPadding = 0;
// TODO: Same as dataLengthPadding?
        private int            recordLengthPadding = 0;

        private int           dataLengthWords = 0;
        private int compressedDataLengthWords = 0;
        private int     userHeaderLengthWords = 0;
        private int         recordLengthWords = 0;
        private int         headerLengthWords = 0;
            /*
             * User registers that will be written at the end of the
             * header.
             */
        private long  recordUserRegisterFirst = 0L;
        private long recordUserRegisterSecond = 0L;

        private int         headerVersion = 6;
        private int         headerMagicWord;

        final static int   HEADER_SIZE_WORDS = 14;
        final static int   HEADER_SIZE_BYTES = 56;
        final static int   HEADER_MAGIC      = 0xc0da0100;
        final static int   HEADER_MAGIC_LE   = HEADER_MAGIC;
        final static int   HEADER_MAGIC_BE   = 0x0010dac0;

        //private ByteOrder   headerByteOrder = ByteOrder.LITTLE_ENDIAN;
        //private final int headerMagicWord = 0xc0da0100;

        public RecordHeader() {
            headerMagicWord = HEADER_MAGIC_LE;
        }
        
        public RecordHeader(long _pos, int _l, int _e){
            position = _pos; recordLength = _l; entries = _e;
        }

        /** Reset internal variables. */
        public void reset(){
            position = 0L;
            recordLength = 0;
            recordNumber = 0;
            entries = 0;

            headerLength = 0;
            userHeaderLength = 0;
            indexLength = 0;
            dataLength = 0;
            compressedDataLength = 0;
            compressionType = 0;

            userHeaderLengthPadding = 0;
            dataLengthPadding = 0;
            compressedDataLengthPadding = 0;
            recordLengthPadding = 0;

            dataLengthWords = 0;
            compressedDataLengthWords = 0;
            userHeaderLengthWords = 0;
            recordLengthWords = 0;
            headerLengthWords = 0;

            recordUserRegisterFirst = 0L;
            recordUserRegisterSecond = 0L;
        }

        /**
         * Returns length padded to 4-byte boundary for given length in bytes.
         * @param length length in bytes.
         * @return length padded to 4-byte boundary in bytes.
         */
        private int getWords(int length){
            int padding = getPadding(length);
            int   words = length/4;
            if(padding>0) words++;
            return words;
        }
        /**
         * Returns number of bytes needed to pad to 4-byte boundary for the given length.
         * @param length length in bytes.
         * @return number of bytes needed to pad to 4-byte boundary.
         */
        private int getPadding(int length) {return padValue[length%4];}
        
        public long           getPosition() { return position;}
        public int              getLength() { return recordLength;}
        public int             getEntries() { return entries;}
        public int     getCompressionType() { return compressionType;}
        public int    getUserHeaderLength() { return userHeaderLength;}
        public int    getUserHeaderLengthWords() { return userHeaderLengthWords;}
        public int             getVersion() { return headerVersion;}
        public int    getDataLength() { return dataLength;}
        public int    getDataLengthWords() { return dataLengthWords;}
        public int    getIndexLength() { return indexLength;}
        public int    getCompressedDataLength() { return compressedDataLength;}
        public int    getCompressedDataLengthWords() { return compressedDataLengthWords;}
        public int    getHeaderLength() {return headerLength;}
        public int    getRecordNumber() { return recordNumber;}
        
        
        
        public long   getUserRegisterFirst(){ return this.recordUserRegisterFirst;}
        public long   getUserRegisterSecond(){ return this.recordUserRegisterSecond;}
        
        
        public RecordHeader  setPosition(long pos) { position = pos; return this;}
        public RecordHeader  setRecordNumber(int num) { recordNumber = num; return this;}
        public RecordHeader  setVersion(int version) { headerVersion = version; return this;}

        /**
         * Set the record length in bytes & words and the padding.
         * @param length  length of record in bytes.
         * @return this object.
         */
        public RecordHeader  setLength(int length) {
            recordLength        = length;
            recordLengthWords   = getWords(length);
            recordLengthPadding = getPadding(length);
            
            //System.out.println(" LENGTH = " + recordLength + "  WORDS = " + this.recordLengthWords
            //+ "  PADDING = " + this.recordLengthPadding + " SIZE = " + recordLengthWords*4 );
            return this;
        }
        
        public RecordHeader  setDataLength(int length) { 
            dataLength = length; 
            dataLengthWords = getWords(length);
            dataLengthPadding = getPadding(length);
            return this;
            
        }
        
        public RecordHeader  setCompressedDataLength(int length) { 
            compressedDataLength = length;
            compressedDataLengthWords = getWords(length);
            compressedDataLengthPadding = getPadding(length);
            return this;
        }
        
        public RecordHeader  setIndexLength(int length) { 
            indexLength = length; 
            //indexLengthWords = getWords(length);            
            return this;
        }
        
        public RecordHeader  setCompressionType(int type) { compressionType = type; return this;}
        public RecordHeader  setEntries(int n) { entries = n; return this;}
        
        public RecordHeader  setUserHeaderLength(int length) { 
            userHeaderLength = length; 
            userHeaderLengthWords   = getWords(length);
            userHeaderLengthPadding = getPadding(length);
            return this;
        }
        
        public RecordHeader  setHeaderLength(int length) { 
            headerLength = length; 
            headerLengthWords = length/4;
            return this;
        }
        
        public RecordHeader setUserRegisterFirst(long reg){
            recordUserRegisterFirst = reg; return this;
        }
        
        public RecordHeader setUserRegisterSecond(long reg){
            recordUserRegisterSecond = reg; return this;
        }

        private int getBitInfoWord() {
            return (userHeaderLengthPadding << 20) |
                   (dataLengthPadding << 22) |
                   (compressedDataLengthPadding << 24) |
                   (headerVersion & 0x000000FF);
        }

        /**
         * Writes the header information to the byte buffer provided.
         * The offset provides the offset in the byte buffer.
         * @param buffer byte buffer to write header to.
         * @param offset position of first word to be written.
         */
        public void writeHeader(ByteBuffer buffer, int offset){
            
            buffer.putInt( 0*4 + offset, recordLengthWords);
            buffer.putInt( 1*4 + offset, recordNumber);
            buffer.putInt( 2*4 + offset, headerLength);
            buffer.putInt( 3*4 + offset, entries);
            buffer.putInt( 4*4 + offset, indexLength);
            buffer.putInt( 5*4 + offset, getBitInfoWord());
            buffer.putInt( 6*4 + offset, userHeaderLength);
            buffer.putInt( 7*4 + offset, headerMagicWord); // word number 8
            
            int compressedWord = ( compressedDataLengthWords & 0x0FFFFFFF) |
                                 ((compressionType & 0x0000000F) << 28);
            buffer.putInt( 8*4 + offset, dataLength);
            buffer.putInt( 9*4 + offset, compressedWord);

            buffer.putLong( 10*4 + offset, recordUserRegisterFirst);
            buffer.putLong( 12*4 + offset, recordUserRegisterSecond);
        }

        /**
         * Writes the header information to the byte buffer provided
         * starting at beginning.
         * @param buffer byte buffer to write header to.
         */
        public void writeHeader(ByteBuffer buffer){
            writeHeader(buffer,0);
        }
        
        /**
         * Reads the header information from a byte buffer and validates
         * it by checking the magic word (which is in position #8, starting 
         * with #1). This magic word determines also the byte order.
         * LITTLE_ENDIAN or BIG_ENDIAN.
         * @param buffer buffer to read from
         * @param offset position of first word to be read.
         */
        public void readHeader(ByteBuffer buffer, int offset){

            // First read the magic word to establish endianness
            headerMagicWord = buffer.getInt( 7*4 + offset);

            // If it's NOT in the proper byte order ...
            if (headerMagicWord != HEADER_MAGIC_LE) {
                // If it needs to be switched ...
                if (headerMagicWord == HEADER_MAGIC_BE) {
                    ByteOrder bufEndian = buffer.order();
                    if (bufEndian == ByteOrder.BIG_ENDIAN) {
                        buffer.order(ByteOrder.LITTLE_ENDIAN);
                    }
                    else {
                        buffer.order(ByteOrder.BIG_ENDIAN);
                    }
                }
                else {
                    // ERROR condition, bad magic word
// TODO: Need to throw exception
                }
            }

            // Next look at the version #
            int bitInoWord = buffer.getInt(  5*4 + offset);
            headerVersion  = (bitInoWord & 0x000000FF);
            if (headerVersion < 6) {
                // ERROR condition, wrong version
// TODO: Need to throw exception
            }

            recordLengthWords   = buffer.getInt(    0 + offset );
            recordLength        = recordLengthWords * 4;
// TODO: Is this always 0, even in HIPO?
            recordLengthPadding = 0;
            
            recordNumber = buffer.getInt(  1*4 + offset );
            headerLength = buffer.getInt(  2*4 + offset);
            entries      = buffer.getInt(  3*4 + offset);
            
            indexLength  = buffer.getInt( 4*4 + offset);
            setIndexLength(indexLength);

            userHeaderLength = buffer.getInt( 6*4 + offset);
            setUserHeaderLength(userHeaderLength);
            
            dataLength       = buffer.getInt( 8*4 + offset);
            setDataLength(dataLength);
            
            int compressionWord   = buffer.getInt( 9*4 + offset);
            compressionType      = (compressionWord >>> 28);
            compressedDataLength = (compressionWord & 0x0FFFFFFF);
            setCompressedDataLength(compressedDataLength);
                        
            recordUserRegisterFirst  = buffer.getLong( 10*4 + offset);
            recordUserRegisterSecond = buffer.getLong( 12*4 + offset);
        }
        
        public void readHeader(ByteBuffer buffer){
            readHeader(buffer,0);
        }
        
        /**
         * returns string representation of the record data.
         * @return 
         */
        @Override
        public String toString(){
            
            StringBuilder str = new StringBuilder();
            str.append(String.format("%24s : %d\n","version",headerVersion));
            str.append(String.format("%24s : %d\n","record #",recordNumber));
            str.append(String.format("%24s : %8d / %8d / %8d\n","user header length",
                    userHeaderLength, userHeaderLengthWords, userHeaderLengthPadding));
            str.append(String.format("%24s : %8d / %8d / %8d\n","   data length",
                    dataLength, dataLengthWords, dataLengthPadding));
            str.append(String.format("%24s : %8d / %8d / %8d\n","record length",
                    recordLength, recordLengthWords, recordLengthPadding));
            str.append(String.format("%24s : %8d / %8d / %8d\n","compressed length",
                    compressedDataLength, compressedDataLengthWords,
                    compressedDataLengthPadding));
            str.append(String.format("%24s : %d\n","header length",headerLength));
            str.append(String.format("%24s : 0x%X\n","magic word",headerMagicWord));
            Integer bitInfo = getBitInfoWord();
            str.append(String.format("%24s : %s\n","bit info word",Integer.toBinaryString(bitInfo)));
            str.append(String.format("%24s : %d\n","record entries",entries));
            str.append(String.format("%24s : %d\n","   compression type",compressionType));
            
            str.append(String.format("%24s : %d\n","  index length",indexLength));
            
            str.append(String.format("%24s : %d\n","user register #1",recordUserRegisterFirst));
            str.append(String.format("%24s : %d\n","user register #2",recordUserRegisterSecond));

            return str.toString();
        }
        
        private String padLeft(String original, String pad, int upTo){
            int npadding = upTo - original.length();
            StringBuilder str = new StringBuilder();
            if(npadding>0) for(int i = 0;i < npadding; i++) str.append(pad);
            str.append(original);
            return str.toString();
        }
        
        public void byteBufferBinaryString(ByteBuffer buffer){
            int nwords = buffer.array().length/4;
            for(int i = 0; i < nwords; i++){
                int value = buffer.getInt(i*4);
                System.out.println(String.format("  word %4d : %36s  0x%8s : %18d", i,
                        padLeft(Integer.toBinaryString(value),"0",32),
                        padLeft(Integer.toHexString(value),"0",8),
                        value));
            }
        }
        
        public static void main(String[] args){
            RecordHeader header = new RecordHeader();
            
            header.setCompressedDataLength(861);
            header.setDataLength(12457);
            header.setUserHeaderLength(459);
            header.setIndexLength(324);
            header.setLength(16 + header.getCompressedDataLengthWords());
            header.setUserRegisterFirst(1234567L);
            header.setUserRegisterSecond(4567890L);
            header.setRecordNumber(23);
            header.setEntries(3245);
            header.setHeaderLength(14);
            header.setCompressionType(1);
            System.out.println(header);
            
            byte[] array = new byte[14*4];
            ByteBuffer buffer = ByteBuffer.wrap(array);
            buffer.order(ByteOrder.LITTLE_ENDIAN);
            
            
            header.writeHeader(buffer);
            
            //header.byteBufferBinaryString(buffer);
            
            RecordHeader header2 = new RecordHeader();
            header2.readHeader(buffer);
            System.out.println(header2);
        }
}