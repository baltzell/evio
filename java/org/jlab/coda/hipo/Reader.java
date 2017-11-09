/*
 *   Copyright (c) 2016.  Jefferson Lab (JLab). All rights reserved. Permission
 *   to use, copy, modify, and distribute  this software and its documentation for
 *   educational, research, and not-for-profit purposes, without fee and without a
 *   signed licensing agreement.
 */
package org.jlab.coda.hipo;

import org.jlab.coda.jevio.ByteDataTransformer;
import org.jlab.coda.jevio.EvioException;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.FileChannel;
import java.util.ArrayList;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Reader class that reads files stored in the HIPO format.<p>
 *
 * <pre>
 * File has this structure:
 *
 *    +----------------------------------+
 *    |      General File Header         |
 *    +----------------------------------+
 *    +----------------------------------+
 *    |         Index (optional)         |
 *    +----------------------------------+
 *    +----------------------------------+
 *    |     User Header (optional)       |
 *    +----------------------------------+
 *    +----------------------------------+
 *    |                                  |
 *    |            Record 1              |
 *    |                                  |
 *    |                                  |
 *    |                                  |
 *    +----------------------------------+
 *                   ...
 *    +----------------------------------+
 *    |                                  |
 *    |            Record N              |
 *    |                                  |
 *    |                                  |
 *    |                                  |
 *    +----------------------------------+
 *    +----------------------------------+
 *    |       Trailer (optional)         |
 *    +----------------------------------+
 *    +----------------------------------+
 *    |    Trailer's Index (optional)    |
 *    +----------------------------------+
 *
 *
 *
 * Buffer or streamed data has this structure:
 *
 *    +----------------------------------+
 *    |                                  |
 *    |            Record 1              |
 *    |                                  |
 *    |                                  |
 *    |                                  |
 *    +----------------------------------+
 *                   ...
 *    +----------------------------------+
 *    |                                  |
 *    |            Record N              |
 *    |                                  |
 *    |                                  |
 *    |                                  |
 *    +----------------------------------+
 *    +----------------------------------+
 *    |       Trailer (optional)         |
 *    +----------------------------------+
 *
 * The important thing with a buffer or streaming is for the last header or
 * trailer to set the "last record" bit.
 *
 * </pre>
 *
 * @version 6.0
 * @since 6.0 08/10/2017
 * @author gavalian
 * @author timmer
 * @see BufferReader
 * @see FileHeader
 * @see RecordInputStream
 */
public class Reader {
    /**
     * List of records in the file. The array is initialized
     * when the entire file is scanned to read out positions
     * of each record in the file (in constructor).
     */
    protected final List<RecordPosition>  recordPositions = new ArrayList<RecordPosition>();

    /** Fastest way to read/write files. */
    protected RandomAccessFile  inStreamRandom;

    /** Keep one record for reading in data record-by-record. */
    protected final RecordInputStream inputRecordStream = new RecordInputStream();
    
    /** Number or position of last record to be read. */
    protected int currentRecordLoaded;

    /** General file header. */
    protected FileHeader fileHeader;

    /** Files may have an xml format dictionary in the user header of the file header. */
    protected String dictionaryXML;

    /** Each file of a set of split CODA files may have a "first" event common to all. */
    protected byte[] firstEvent;

    /** Object to handle event indexes in context of file and having to change records. */
    protected FileEventIndex eventIndex = new FileEventIndex();

    /** Are we reading from file (true) or buffer? */
    protected boolean fromFile = true;
    

    /**
     * Default constructor. Does nothing.
     * The {@link #open(String)} method has to be called to open the input stream.
     * Also {@link #forceScanFile()} needs to be called to find records.
     */
    public Reader() {}

    /**
     * Constructor with file already opened.
     * The method {@link #forceScanFile()} needs to be called to find records.
     * @param file open file
     */
    public Reader(RandomAccessFile file) {
        inStreamRandom = file;
    }

    /**
     * Constructor with filename. Creates instance and opens
     * the input stream with given name. Uses existing indexes
     * in file before scanning.
     * @param filename input file name.
     */
    public Reader(String filename){
        open(filename);
        scanFile(false);
    }

    /**
     * Constructor with filename. Creates instance and opens
     * the input stream with given name.
     * @param filename input file name.
     * @param forceScan if true, force a scan of file, else use existing indexes first.
     */
    public Reader(String filename, boolean forceScan){
        open(filename);
        if (forceScan){
           forceScanFile();
        } else {
            scanFile(forceScan);
        }
    }

    public Reader(ByteBuffer buffer) {
        
    }

    /**
     * Opens an input stream in binary mode. Scans for
     * records in the file and stores record information
     * in internal array. Each record can be read from the file.
     * @param filename input file name
     */
    public final void open(String filename){
        if(inStreamRandom != null && inStreamRandom.getChannel().isOpen()) {
            try {
                System.out.println("[READER] ---> closing current file : " + inStreamRandom.getFilePointer());
                inStreamRandom.close();
            } catch (IOException ex) {
                Logger.getLogger(Reader.class.getName()).log(Level.SEVERE, null, ex);
            }
        }

        try {
            System.out.println("[READER] ----> opening current file : " + filename);
            inStreamRandom = new RandomAccessFile(filename,"r");
            System.out.println("[READER] ---> open successful, size : " + inStreamRandom.length());
        } catch (FileNotFoundException ex) {
            Logger.getLogger(Reader.class.getName()).log(Level.SEVERE, null, ex);
        } catch (IOException ex) {
            Logger.getLogger(Reader.class.getName()).log(Level.SEVERE, null, ex);
        }
    }


    /**
     * Get the XML format dictionary if there is one.
     * @return XML format dictionary, else null.
     */
    public String getDictionary() {
        // Read in dictionary if necessary
        extractDictionary();
        return dictionaryXML;
    }

    /**
     * Does this evio file have an associated XML dictionary?
     * @return <code>true</code> if this evio file has an associated XML dictionary,
     *         else <code>false</code>.
     */
    public boolean hasDictionary() {return fileHeader.hasDictionary();}

    /**
     * Get a byte array representing the first event.
     * @return byte array representing the first event. Null if none.
     */
    public byte[] getFirstEvent() {
        // Read in first event if necessary
        extractDictionary();
        return firstEvent;
    }

    /**
     * Does this evio file have an associated first event?
     * @return <code>true</code> if this evio file has an associated first event,
     *         else <code>false</code>.
     */
    public boolean hasFirstEvent() {return fileHeader.hasFirstEvent();}

    /**
     * Get the number of events in file.
     * @return number of events in file.
     */
    public int getEventCount() {return eventIndex.getMaxEvents();}

    /**
     * Get the number of records read from the file.
     * @return number of records read from the file.
     */
    public int getRecordCount() {return recordPositions.size();}

    // Methods for current record

    /**
     * Get a byte array representing the next event from the file while sequentially reading.
     * @return byte array representing the next event or null if there is none.
     * @throws HipoException if file not in hipo format
     */
    public byte[] getNextEvent() throws HipoException {
        // If reached last event in file ...
        if (!eventIndex.canAdvance()) return null;
        if (eventIndex.advance()) {
            // If here, the next event is in the next record
            readRecord(eventIndex.getRecordNumber());
        }
        if(inputRecordStream.getEntries()==0){
            System.out.println("[READER] first time reading");
            readRecord(eventIndex.getRecordNumber());
        }
        return inputRecordStream.getEvent(eventIndex.getRecordEventNumber());
    }
    /**
     * Get a byte array representing the previous event from the sequential queue.
     * @return byte array representing the previous event or null if there is none.
     * @throws HipoException if the file is not in HIPO format
     */
    public byte[] getPrevEvent() throws HipoException {
        if(!eventIndex.canRetreat()) return null;
        if(eventIndex.retreat()){
            readRecord(eventIndex.getRecordNumber());
        }
        return inputRecordStream.getEvent(eventIndex.getRecordEventNumber());
    }
    /**
     * Reads user header of the file. The returned ByteBuffer also contains
     * endianness of the file.
     * @return ByteBuffer containing the user header of the file
     */
    public ByteBuffer readUserHeader(){
        int userLen = fileHeader.getUserHeaderLength();
        System.out.println("  " + fileHeader.getUserHeaderLength()
         + "  " + fileHeader.getHeaderLength() + "  " + fileHeader.getIndexLength());
        try {
            byte[] userBytes = new byte[userLen];
            inStreamRandom.getChannel().position(fileHeader.getHeaderLength() + fileHeader.getIndexLength());
            inStreamRandom.read(userBytes);
            ByteBuffer userBuffer = ByteBuffer.wrap(userBytes);
            return userBuffer;
        } catch (IOException ex) {
            Logger.getLogger(Reader.class.getName()).log(Level.SEVERE, null, ex);
        }
        return null;
    }
    /**
     * Get a byte array representing the specified event from the file.
     * If index is out of bounds, null is returned.
     * @param index index of specified event within the entire file,
     *              contiguous starting at 0.
     * @return byte array representing the specified event or null if
     *         index is out of bounds.
     * @throws HipoException if file not in hipo format
     */
    public byte[] getEvent(int index) throws HipoException {
        
        if (index < 0 || index >= eventIndex.getMaxEvents()) {
            return null;
        }
        
        if (eventIndex.setEvent(index)) {
            // If here, the event is in the next record
            readRecord(eventIndex.getRecordNumber());
        }
        if(inputRecordStream.getEntries()==0){
            System.out.println("[READER] first time reading");
            readRecord(eventIndex.getRecordNumber());
        }
        return inputRecordStream.getEvent(eventIndex.getRecordEventNumber());
    }

    /**
     * Get a byte array representing the specified event from the file
     * and place it in given buffer.
     * If no buffer is given (arg is null), create a buffer internally and return it.
     * If index is out of bounds, null is returned.
     * @param buffer buffer in which to place event data.
     * @param index index of specified event within the entire file,
     *              contiguous starting at 0.
     * @return buffer or null if buffer is null or index out of bounds.
     * @throws HipoException if file not in hipo format, or
     *                       if buffer has insufficient space to contain event
     *                       (buffer.capacity() < event size).
     */
    public ByteBuffer getEvent(ByteBuffer buffer, int index) throws HipoException {
        if (index < 0 || index >= eventIndex.getMaxEvents()) {
            return null;
        }

        if (eventIndex.setEvent(index)) {
            // If here, the event is in the next record
            readRecord(eventIndex.getRecordNumber());
        }
        return inputRecordStream.getEvent(buffer, eventIndex.getRecordEventNumber());
    }
    /**
     * Checks if the file has an event to read next.
     * @return true if the next event is available, false otherwise
     */
    public boolean hasNext(){
       return eventIndex.canAdvance();
    }
    /**
     * Checks if the stream has previous event to be accessed through, getPrevEvent()
     * @return true if previous event is accessible, false otherwise
     */
    public boolean hasPrev(){
       return eventIndex.canRetreat();
    }
    /**
     * Get the number of events in current record.
     * @return number of events in current record.
     */
    public int getRecordEventCount() {return inputRecordStream.getEntries();}

    /**
     * Get the index of the current record.
     * @return index of the current record.
     */
    public int getCurrentRecord() {return currentRecordLoaded;}

    /**
     * Get the current record stream.
     * @return current record stream.
     */
    public RecordInputStream getCurrentRecordStream() {return inputRecordStream;}

    /**
     * Reads record from the file at the given record index.
     * @param index record index  (starting at 0).
     * @return true if valid index and successful reading record, else false.
     * @throws HipoException if file not in hipo format
     */
    public boolean readRecord(int index) throws HipoException {
        if (index >= 0 && index < recordPositions.size()) {
            RecordPosition pos = recordPositions.get(index);
            inputRecordStream.readRecord(inStreamRandom, pos.getPosition());
            currentRecordLoaded = index;
            return true;
        }
        return false;
    }

    /** Extract dictionary and first event from file if possible, else do nothing. */
    protected void extractDictionary() {
        // If already read & parsed from file ...
        if (dictionaryXML != null || firstEvent != null) {
            return;
        }

        // If no dictionary or first event ...
        if (!fileHeader.hasDictionary() && !fileHeader.hasFirstEvent()) {
            return;
        }

        int userLen = fileHeader.getUserHeaderLength();
        // 8 byte min for evio event, more for xml dictionary
        if (userLen < 8) {
            return;
        }
        
        RecordInputStream record;

        try {
            // Position right before file header's user header
            inStreamRandom.getChannel().position(fileHeader.getHeaderLength() + fileHeader.getIndexLength());
            // Read user header
            byte[] userBytes = new byte[userLen];
            inStreamRandom.read(userBytes);
            ByteBuffer userBuffer = ByteBuffer.wrap(userBytes);
            // Parse user header as record
            record = new RecordInputStream(fileHeader.getByteOrder());
            record.readRecord(userBuffer, 0);
        }
        catch (IOException e) {
            // Can't read file
            return;
        }
        catch (HipoException e) {
            // Not in proper format
            return;
        }

        int eventIndex = 0;

        // Dictionary always comes first in record
        if (fileHeader.hasDictionary()) {
            // Just plain ascii, not evio format
            byte[] dict = record.getEvent(eventIndex++);
            try {dictionaryXML = new String(dict, "US-ASCII");}
            catch (UnsupportedEncodingException e) {/* never happen */}
        }

        // First event comes next
        if (fileHeader.hasFirstEvent()) {
            firstEvent = record.getEvent(eventIndex);
        }
    }

    //-----------------------------------------------------------------

    /** Scan file to find all records and store their position, length, and event count. */
    public void forceScanFile() {
        
        byte[] headerBytes = new byte[RecordHeader.HEADER_SIZE_BYTES];
        ByteBuffer headerBuffer = ByteBuffer.wrap(headerBytes);
        
        fileHeader = new FileHeader();
        RecordHeader recordHeader = new RecordHeader();
        
        try {
            // Scan file by reading each record header and
            // storing its position, length, and event count.
            long fileSize = inStreamRandom.length();
            // Don't go beyond 1 header length before EOF since we'll be reading in 1 header
            long maximumSize = fileSize - RecordHeader.HEADER_SIZE_BYTES;
            recordPositions.clear();
            int  offset;
            FileChannel channel = inStreamRandom.getChannel().position(0L);

            // Read and parse file header
            inStreamRandom.read(headerBytes);
            fileHeader.readHeader(headerBuffer);
            
            // First record position (past file's header + index + user header)
            long recordPosition = fileHeader.getLength();

            while (recordPosition < maximumSize) {
                channel.position(recordPosition);
                inStreamRandom.read(headerBytes); 
                recordHeader.readHeader(headerBuffer);
                //System.out.println(">>>>>==============================================");
                //System.out.println(recordHeader.toString());
                offset = recordHeader.getLength();
                RecordPosition pos = new RecordPosition(recordPosition, offset,
                                                        recordHeader.getEntries());
                recordPositions.add(pos);
                // Track # of events in this record for event index handling
                eventIndex.addEventSize(recordHeader.getEntries());
                recordPosition += offset;
            }
            //System.out.println("NUMBER OF RECORDS " + recordPositions.size());
        } catch (IOException ex) {
            Logger.getLogger(Reader.class.getName()).log(Level.SEVERE, null, ex);
        } catch (HipoException ex) {
            Logger.getLogger(Reader.class.getName()).log(Level.SEVERE, null, ex);
        }
    }
    /**
     * Scans the file to index all the record positions.
     * It takes advantage of any existing indexes in file.
     * @param force if true, force a file scan even if header
     *              or trailer have index info.
     */
    protected void scanFile(boolean force) {

        if (force) {
            forceScanFile();
            return;
        }

        //System.out.println("[READER] ---> scanning the file");
        byte[] headerBytes = new byte[RecordHeader.HEADER_SIZE_BYTES];
        ByteBuffer headerBuffer = ByteBuffer.wrap(headerBytes);

        fileHeader = new FileHeader();
        RecordHeader recordHeader = new RecordHeader();

        try {
            FileChannel channel = inStreamRandom.getChannel().position(0L);

            // Read and parse file header
            inStreamRandom.read(headerBytes);
            fileHeader.readHeader(headerBuffer);

            // Is there an existing record length index?
            // Index in trailer gets first priority.
            // Index in file header gets next priority.
            boolean fileHasIndex = fileHeader.hasTrailerWithIndex() || (fileHeader.hasIndex());
            /*System.out.println(" file has index = " + fileHasIndex
                    + "  " + fileHeader.hasTrailerWithIndex()
                    + "  " + fileHeader.hasIndex()); */

            // If there is no index, scan file
            if (!fileHasIndex) {
                forceScanFile();
                return;
            }

            // Take care of non-standard header size
            channel.position(fileHeader.getHeaderLength());
            //System.out.println(header.toString());

            // First record position (past file's header + index + user header)
            long recordPosition = fileHeader.getLength();
            //System.out.println("record position = " + recordPosition);

            int indexLength;
            // If we have a trailer with indexes ...
            if (fileHeader.hasTrailerWithIndex()) {
                // Position read right before trailing header
                channel.position(fileHeader.getTrailerPosition());
                // Read trailer
                inStreamRandom.read(headerBytes);
                recordHeader.readHeader(headerBuffer);
                indexLength = recordHeader.getIndexLength();
            }
            else {
                // If index immediately follows file header,
                // we're already in position to read it.
                indexLength = fileHeader.getIndexLength();
            }

            // Read indexes
            byte[] index = new byte[indexLength];
            inStreamRandom.read(index);
            int len, count;
            try {
                // Turn bytes into record lengths & event counts
                int[] intData = ByteDataTransformer.toIntArray(index, fileHeader.getByteOrder());
                // Turn record lengths into file positions and store in list
                recordPositions.clear();
                for (int i=0; i < intData.length; i += 2) {
                    len = intData[i];
                    count = intData[i+1];
                    RecordPosition pos = new RecordPosition(recordPosition, len, count);
                    recordPositions.add(pos);
                    // Track # of events in this record for event index handling
                    eventIndex.addEventSize(count);
                    recordPosition += len;
                }
            }
            catch (EvioException e) {/* never happen */}

        } catch (IOException ex) {
            Logger.getLogger(Reader.class.getName()).log(Level.SEVERE, null, ex);
        } catch (HipoException ex) {
            Logger.getLogger(Reader.class.getName()).log(Level.SEVERE, null, ex);
        }
    }
    
    public void show(){
        System.out.println(" ***** FILE: (info), RECORDS = "
                + recordPositions.size() + " *****");
        for(RecordPosition entry : this.recordPositions){
            System.out.println(entry);
        }
    }
    
    /**
     * Internal class to keep track of the records in the file.
     * Each entry keeps record position in the file, length of
     * the record and number of entries contained.
     */
    private static class RecordPosition {
        /** Position in file. */
        private long position;
        /** Length in bytes. */
        private int  length;
        /** Number of entries in record. */
        private int  count;

        RecordPosition(long pos) {position = pos;}
        RecordPosition(long pos, int len, int cnt) {
            position = pos; length = len; count = cnt;
        }

        public RecordPosition setPosition(long _pos){ position = _pos; return this; }
        public RecordPosition setLength(int _len)   { length = _len;   return this; }
        public RecordPosition setCount( int _cnt)   { count = _cnt;    return this; }
        
        public long getPosition(){ return position;}
        public int  getLength(){   return   length;}
        public int  getCount(){    return    count;}
        
        @Override
        public String toString(){
            return String.format(" POSITION = %16d, LENGTH = %12d, COUNT = %8d", position, length, count);
        }
    }
    
    public static void main(String[] args){
        Reader reader = new Reader("converted_000810.evio");
        
        //reader.show();
        byte[] eventarray = new byte[1024*1024];
        ByteBuffer eventBuffer = ByteBuffer.wrap(eventarray);
        
        for(int i = 0; i < reader.getRecordCount(); i++){
            try {
                reader.readRecord(i);
                Thread.sleep(100);
                int nevents = reader.getRecordCount();
                for(int k = 0; k < nevents ; k++){
                    reader.getEvent(eventBuffer,k);
                }
                System.out.println("---> read record " + i);
            } catch (Exception ex) {
                Logger.getLogger(Reader.class.getName()).log(Level.SEVERE, null, ex);
            }
        }
        
        //reader.open("test.evio");
        /*reader.readRecord(0);
        int nevents = reader.getRecordEventCount();
        System.out.println("-----> events = " + nevents);
        for(int i = 0; i < 10 ; i++){
            byte[] event = reader.getEvent(i);
            System.out.println("---> events length = " + event.length);
            ByteBuffer buffer = ByteBuffer.wrap(event);
            buffer.order(ByteOrder.LITTLE_ENDIAN);
            String data = DataUtils.getStringArray(buffer, 10,30);
            System.out.println(data);
        }*/
    }
}
