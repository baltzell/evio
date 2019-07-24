//
// Created by timmer on 7/18/19.
//

#include "Reader.h"


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
 * Something to keep in mind is one can intersperse sequential calls
 * (getNextEvent, getPrevEvent, or getNextEventNode) with random access
 * calls (getEvent or getEventNode), and the sequence remains unchanged
 * after the random access.
 *
 * @version 6.0
 * @since 6.0 08/10/2017
 * @author gavalian
 * @author timmer
 * @see FileHeader
 * @see RecordInput
 */




///**
// * Default constructor. Does nothing.
// * The {@link #open(String)} method has to be called to open the input stream.
// * Also {@link #forceScanFile()} needs to be called to find records.
// */
//Reader::Reader() noexcept {
////    inStreamRandom;
////    nodePool;
//}

/**
 * Constructor with filename. Creates instance and opens
 * the input stream with given name. Uses existing indexes
 * in file before scanning.
 * @param filename input file name.
 * @throws IOException   if error reading file
 * @throws HipoException if file is not in the proper format or earlier than version 6
 */
Reader::Reader(string & filename) {
    // inStreamRandom & nodePool not defined
    open(filename);
    scanFile(false);
}

/**
 * Constructor with filename. Creates instance and opens
 * the input stream with given name.
 * @param filename input file name.
 * @param forceScan if true, force a scan of file, else use existing indexes first.
 * @throws IOException   if error reading file
 * @throws HipoException if file is not in the proper format or earlier than version 6
 */
Reader::Reader(string & filename, bool forceScan) :
        Reader(filename, forceScan, false) {}

/**
 * Constructor with filename. Creates instance and opens
 * the input stream with given name.
 * @param filename input file name.
 * @param forceScan if true, force a scan of file, else use existing indexes first.
 * @param checkRecordNumSeq if true, check to see if all record numbers are in order,
 *                          if not throw exception.
 * @throws IOException   if error reading file
 * @throws HipoException if file is not in the proper format or earlier than version 6;
 *                       if checkRecordNumSeq is true and records are out of sequence.
 */
Reader::Reader(string & filename, bool forceScan, bool checkRecordNumSeq) {

    checkRecordNumberSequence = checkRecordNumSeq;

    open(filename);

    if (forceScan){
        forceScanFile();
    } else {
        scanFile(forceScan);
    }
}

/**
 * Constructor for reading buffer with evio data.
 * Buffer must be ready to read with position and limit set properly.
 * @param buffer buffer with evio data.
 * @throws HipoException if buffer too small, not in the proper format, or earlier than version 6
 */
Reader::Reader(ByteBuffer & buffer) {

    this->buffer = buffer;
    bufferOffset = buffer.position();
    bufferLimit  = buffer.limit();
    fromFile = false;

    scanBuffer();
}

/**
 * Constructor for reading buffer with evio data.
 * Buffer must be ready to read with position and limit set properly.
 * @param buffer buffer with evio data.
 * @param pool pool of EvioNode objects for garbage-free operation.
 * @throws HipoException if buffer too small, not in the proper format, or earlier than version 6
 */
Reader::Reader(ByteBuffer & buffer, EvioNodeSource & pool) :
        Reader(buffer, pool, false) {
}

/**
 * Constructor for reading buffer with evio data.
 * Buffer must be ready to read with position and limit set properly.
 * @param buffer buffer with evio data.
 * @param pool pool of EvioNode objects for garbage-free operation.
 * @param checkRecordNumSeq if true, check to see if all record numbers are in order,
 *                          if not throw exception.
 * @throws HipoException if buffer too small, not in the proper format, or earlier than version 6;
 *                       if checkRecordNumSeq is true and records are out of sequence.
 */
Reader::Reader(ByteBuffer & buffer, EvioNodeSource & pool, bool checkRecordNumSeq) {
    this->buffer = buffer;
    bufferOffset = buffer.position();
    bufferLimit  = buffer.limit();

    nodePool = pool;
    fromFile = false;
    checkRecordNumberSequence = checkRecordNumSeq;
    scanBuffer();
}

/**
 * Opens an input stream in binary mode. Scans for
 * records in the file and stores record information
 * in internal array. Each record can be read from the file.
 * @param filename input file name
 * @throws IOException if file not found or error opening file
 */
void Reader::open(string & filename) {
    if (inStreamRandom != nullptr && inStreamRandom.getChannel().isOpen()) {
        try {
            //cout << "[READER] ---> closing current file : " << inStreamRandom.getFilePointer() << endl;
            inStreamRandom.close();
        }
        catch (IOException ex) {}
    }

    fileName = filename;

    //cout << "[READER] ----> opening file : " << filename << endl;
    inStreamRandom = new RandomAccessFile(filename,"r");
    fileSize = inStreamRandom.length();
    //cout << "[READER] ---> open successful, size : " << inStreamRandom.length() << endl;
}

/**
 * This closes the file.
 * @throws IOException if error accessing file
 */
void Reader::close() {
    if (closed) {
        return;
    }

    if (fromFile) {
        inStreamRandom.close();
    }

    closed = true;
}

/**
 * Has {@link #close()} been called (without reopening by calling
 * {@link #setBuffer(ByteBuffer)})?
 *
 * @return {@code true} if this object closed, else {@code false}.
 */
bool Reader::isClosed() {return closed;}

/**
 * Is a file being read?
 * @return {@code true} if a file is being read, {@code false} if it's a buffer.
 */
bool Reader::isFile() {return fromFile;}

/**
 * This method can be used to avoid creating additional Reader
 * objects by reusing this one with another buffer.
 * @param buf ByteBuffer to be read
 * @throws HipoException if buf arg is null, buffer too small,
 *                       not in the proper format, or earlier than version 6
 */
void Reader::setBuffer(ByteBuffer & buf) {
    setBuffer(buf, nullptr);
}

/**
 * This method can be used to avoid creating additional Reader
 * objects by reusing this one with another buffer. The method
 * {@link #close()} is called before anything else.  The pool is <b>not</b>
 * reset in this method. Caller may do that prior to calling method.
 *
 * @param buf ByteBuffer to be read
 * @param pool pool of EvioNode objects to use when parsing buf.
 * @throws HipoException if buffer too small,
 *                       not in the proper format, or earlier than version 6
 */
void Reader::setBuffer(ByteBuffer & buf, EvioNodeSource & pool) {

    nodePool     = pool;
    buffer       = buf;
    bufferLimit  = buffer.limit();
    bufferOffset = buffer.position();
    eventIndex   = FileEventIndex();

    eventNodes.clear();
    recordPositions.clear();

    fromFile = false;
    compressed = false;
    firstEvent = nullptr;
    dictionaryXML = nullptr;
// TODO: set to -1 ???
    sequentialIndex = 0;
    firstRecordHeader.reset();
    currentRecordLoaded = 0;

    scanBuffer();

    closed = false;
}

/**
 * This method can be used to avoid creating additional Reader
 * objects by reusing this one with another buffer. If the given buffer has
 * uncompressed data, this method becomes equivalent
 * to {@link #setBuffer(ByteBuffer, EvioNodeSource)} and its return value is just
 * the buf argument.<p>
 *
 * The given buffer may have compressed data, and if so, the data is uncompressed
 * in placed back into the same buffer. If, however, the given buffer does not have
 * enough space for the uncompressed data, a new buffer is internally allocated,
 * data is placed in the new buffer, and the new buffer is the return value.<p>
 *
 * @param buf  ByteBuffer to be read
 * @param pool pool of EvioNode objects to use when parsing buf.
 * @return buf arg if data is not compressed. If compressed and buf does not have the
 *         necessary space to contain all uncompressed data, a new buffer is allocated,
 *         filled, and returned.
 * @throws HipoException if buf arg is null, buffer too small,
 *                       not in the proper format, or earlier than version 6
 */
ByteBuffer Reader::setCompressedBuffer(ByteBuffer & buf, EvioNodeSource & pool) {
    nodePool = pool;
    setBuffer(buf);
    return buffer;
}

/**
 * Get the name of the file being read.
 * @return name of the file being read or null if none.
 */
string Reader::getFileName() {return fileName;}

/**
 * Get the size of the file being read, in bytes.
 * @return size of the file being read, in bytes, or 0 if none.
 */
long Reader::getFileSize() {return fileSize;}

/**
 * Get the buffer being read, if any.
 * @return buffer being read, if any.
 */
ByteBuffer Reader::getBuffer() {return buffer;}

/**
 * Get the beginning position of the buffer being read.
 * @return the beginning position of the buffer being read.
 */
int Reader::getBufferOffset() {return bufferOffset;}

/**
 * Get the file header from reading a file.
 * Will return null if reading a buffer.
 * @return file header from reading a file.
 */
FileHeader Reader::getFileHeader() {return fileHeader;}

/**
 * Get the first record header from reading a file/buffer.
 * @return first record header from reading a file/buffer.
 */
RecordHeader Reader::getFirstRecordHeader() {return firstRecordHeader;}

/**
 * Get the byte order of the file/buffer being read.
 * @return byte order of the file/buffer being read.
 */
ByteOrder Reader::getByteOrder() {return byteOrder;}

/**
 * Set the byte order of the file/buffer being read.
 * @param order byte order of the file/buffer being read.
 */
void Reader::setByteOrder(ByteOrder & order) {byteOrder = order;}

/**
 * Get the Evio format version number of the file/buffer being read.
 * @return Evio format version number of the file/buffer being read.
 */
int Reader::getVersion() {return evioVersion;}

/**
 * Is the data in the file/buffer compressed?
 * @return true if data is compressed.
 */
bool Reader::isCompressed() {return compressed;}

/**
 * Get the XML format dictionary if there is one.
 * @return XML format dictionary, else null.
 */
string Reader::getDictionary() {
    // Read in dictionary if necessary
    extractDictionaryAndFirstEvent();
    return dictionaryXML;
}

/**
 * Does this evio file/buffer have an associated XML dictionary?
 * @return <code>true</code> if this evio file/buffer has an associated XML dictionary,
 *         else <code>false</code>.
 */
bool Reader::hasDictionary() {
    if (fromFile) {
        return fileHeader.hasDictionary();
    }
    return firstRecordHeader.hasDictionary();
}

/**
 * Get a byte array representing the first event.
 * @return byte array representing the first event. Null if none.
 */
uint8_t* Reader::getFirstEvent() {
    // Read in first event if necessary
    extractDictionaryAndFirstEvent();
    return firstEvent;
}

/**
 * Does this evio file/buffer have an associated first event?
 * @return <code>true</code> if this evio file/buffer has an associated first event,
 *         else <code>false</code>.
 */
bool Reader::hasFirstEvent() {
    if (fromFile) {
        return fileHeader.hasFirstEvent();
    }
    return firstRecordHeader.hasFirstEvent();
}

/**
 * Get the number of events in file/buffer.
 * @return number of events in file/buffer.
 */
int Reader::getEventCount() {return eventIndex.getMaxEvents();}

/**
 * Get the number of records read from the file/buffer.
 * @return number of records read from the file/buffer.
 */
int Reader::getRecordCount() {return recordPositions.size();}

/**
 * Returns the list of record positions in the file.
 * @return
 */
List<RecordPosition> Reader::getRecordPositions() {return recordPositions;}

/**
 * Get the list of EvioNode objects contained in the buffer being read.
 * To be used internally to evio.
 * @return list of EvioNode objects contained in the buffer being read.
 */
ArrayList<EvioNode> Reader::getEventNodes() {return eventNodes;}

/**
 * Get whether or not record numbers are enforced to be sequential.
 * @return {@code true} if record numbers are enforced to be sequential.
 */
bool Reader::getCheckRecordNumberSequence() {return checkRecordNumberSequence;}

/**
 * Get the number of events remaining in the file/buffer.
 * Useful only if doing a sequential read.
 *
 * @return number of events remaining in the file/buffer
 */
int Reader::getNumEventsRemaining() {return (eventIndex.getMaxEvents() - sequentialIndex);}

// Methods for current record

/**
 * Get a byte array representing the next event from the file/buffer while sequentially reading.
 * If the previous call was to {@link #getPrevEvent}, this will get the event
 * past what that returned. Once the last event is returned, this will return null.
 * @return byte array representing the next event or null if there is none.
 * @throws HipoException if file/buffer not in hipo format
 */
uint8_t* Reader::getNextEvent() {
    bool debug = false;

    // If the last method called was getPrev, not getNext,
    // we don't want to get the same event twice in a row, so
    // increment index. Take into account if this is the
    // first time getNextEvent or getPrevEvent called.
    if (sequentialIndex < 0) {
        sequentialIndex = 0;
        if (debug) cout << "getNextEvent first time index set to " << sequentialIndex << endl;
    }
        // else if last call was to getPrevEvent ...
    else if (!lastCalledSeqNext) {
        sequentialIndex++;
        if (debug) cout << "getNextEvent extra increment to " << sequentialIndex << endl;
    }

    uint8_t * array = getEvent(sequentialIndex++);
    lastCalledSeqNext = true;

    if (array == nullptr) {
        if (debug) cout << "getNextEvent hit limit at index " << (sequentialIndex - 1) <<
                        ", set to " << (sequentialIndex - 1) << endl << endl;
        sequentialIndex--;
    }
    else {
        if (debug) cout << "getNextEvent got event " << (sequentialIndex - 1) << endl << endl;
    }

    return array;
}

/**
 * Get a byte array representing the previous event from the sequential queue.
 * If the previous call was to {@link #getNextEvent}, this will get the event
 * previous to what that returned. If this is called before getNextEvent,
 * it will always return null. Once the first event is returned, this will
 * return null.
 * @return byte array representing the previous event or null if there is none.
 * @throws HipoException if the file/buffer is not in HIPO format
 */
uint8_t* Reader::getPrevEvent() {
    bool debug = false;

    // If the last method called was getNext, not getPrev,
    // we don't want to get the same event twice in a row, so
    // decrement index. Take into account if this is the
    // first time getNextEvent or getPrevEvent called.
    if (sequentialIndex < 0) {
        if (debug) cout << "getPrevEvent first time index = " << sequentialIndex << endl;
    }
        // else if last call was to getNextEvent ...
    else if (lastCalledSeqNext) {
        sequentialIndex--;
        if (debug) cout << "getPrevEvent extra decrement to " << sequentialIndex << endl;
    }

    uint8_t* array = getEvent(--sequentialIndex);
    lastCalledSeqNext = false;

    if (array == nullptr) {
        if (debug) cout << "getPrevEvent hit limit at index " << sequentialIndex <<
                        ", set to " << (sequentialIndex + 1) << endl << endl;
        sequentialIndex++;
    }
    else {
        if (debug) cout << "getPrevEvent got event " << (sequentialIndex) << endl << endl;
    }

    return array;
}

/**
 * Get an EvioNode representing the next event from the buffer while sequentially reading.
 * Calling this and calling {@link #getNextEvent()} have the same effect in terms of
 * advancing the same internal counter.
 * If the previous call was to {@link #getPrevEvent}, this will get the event
 * past what that returned. Once the last event is returned, this will return null.
 *
 * @return EvioNode representing the next event or null if no more events,
 *         reading a file or data is compressed.
 */
EvioNode Reader::getNextEventNode() {
    if (sequentialIndex >= eventIndex.getMaxEvents() || fromFile || compressed) {
        return null;
    }

    if (sequentialIndex < 0) {
        sequentialIndex = 0;
    }
    // else if last call was to getPrevEvent ...
    else if (!lastCalledSeqNext) {
        sequentialIndex++;
    }

    lastCalledSeqNext = true;
    return eventNodes.get(sequentialIndex++);
}

/**
 * Reads user header of the file header/first record header of buffer.
 * The returned ByteBuffer also contains endianness of the file/buffer.
 * @return ByteBuffer containing the user header of the file/buffer.
 * @throws IOException if error reading file
 */
ByteBuffer Reader::readUserHeader() {
    uint8_t* userBytes;

    if (fromFile) {
        int userLen = fileHeader.getUserHeaderLength();
        //System.out.println("  " + fileHeader.getUserHeaderLength()
        //                           + "  " + fileHeader.getHeaderLength() + "  " + fileHeader.getIndexLength());
        userBytes = new uint8_t[userLen];

        inStreamRandom.getChannel().position(fileHeader.getHeaderLength() + fileHeader.getIndexLength());
        inStreamRandom.read(userBytes);
        return ByteBuffer.wrap(userBytes).order(fileHeader.getByteOrder());
    }
    else {
        int userLen = firstRecordHeader.getUserHeaderLength();
        //System.out.println("  " + firstRecordHeader.getUserHeaderLength()
        //                           + "  " + firstRecordHeader.getHeaderLength() + "  " + firstRecordHeader.getIndexLength());
        userBytes = new uint8_t[userLen];

        buffer.position(firstRecordHeader.getHeaderLength() + firstRecordHeader.getIndexLength());
        buffer.get(userBytes, 0, userLen);
        return ByteBuffer.wrap(userBytes).order(firstRecordHeader.getByteOrder());
    }
}

/**
 * Get a byte array representing the specified event from the file/buffer.
 * If index is out of bounds, null is returned.
 * @param index index of specified event within the entire file/buffer,
 *              contiguous starting at 0.
 * @return byte array representing the specified event or null if
 *         index is out of bounds.
 * @throws HipoException if file/buffer not in hipo format
 */
uint8_t* Reader::getEvent(int index) {

    if (index < 0 || index >= eventIndex.getMaxEvents()) {
//System.out.println("getEvent: index = " + index + ", max events = " + eventIndex.getMaxEvents());
        return nullptr;
    }

    if (eventIndex.setEvent(index)) {
        // If here, the event is in the next record
        readRecord(eventIndex.getRecordNumber());
    }

    if (inputRecordStream.getEntries()==0){
        //System.out.println("[READER] first time reading");
        readRecord(eventIndex.getRecordNumber());
    }

    return inputRecordStream.getEvent(eventIndex.getRecordEventNumber());
}

/**
 * Get a byte array representing the specified event from the file/buffer
 * and place it in the given buf.
 * If no buf is given (arg is null), create a buffer internally and return it.
 * If index is out of bounds, null is returned.
 * @param buf buffer in which to place event data.
 * @param index index of specified event within the entire file/buffer,
 *              contiguous starting at 0.
 * @return buf or null if buf is null or index out of bounds.
 * @throws HipoException if file/buffer not in hipo format, or
 *                       if buf has insufficient space to contain event
 *                       (buf.capacity() < event size).
 */
ByteBuffer Reader::getEvent(ByteBuffer buf, int index) {

    if (index < 0 || index >= eventIndex.getMaxEvents()) {
        return nullptr;
    }

    if (eventIndex.setEvent(index)) {
        // If here, the event is in the next record
        readRecord(eventIndex.getRecordNumber());
    }
    if (inputRecordStream.getEntries() == 0) {
        //cout << "[READER] first time reading buffer" << endl;
        readRecord(eventIndex.getRecordNumber());
    }
    return inputRecordStream.getEvent(buf, eventIndex.getRecordEventNumber());
}

/**
 * Get an EvioNode representing the specified event from the buffer.
 * If index is out of bounds, null is returned.
 * @param index index of specified event within the entire buffer,
 *              starting at 0.
 * @return EvioNode representing the specified event or null if
 *         index is out of bounds, reading a file or data is compressed.
 */
EvioNode Reader::getEventNode(uint32_t index) {
//cout << "getEventNode: index = " << index + " >? " << eventIndex.getMaxEvents() <<
//                   ", fromFile = " << fromFile << ", compressed = " << compressed << endl;
    if (index >= eventIndex.getMaxEvents() || fromFile) {
//cout << "getEventNode: index out of range, from file or compressed so node = NULL" << endl;
        return nullptr;
    }
//cout << "getEventNode: Getting node at index = " << index << endl;
    return eventNodes.get(index);
}

/**
 * Checks if the file has an event to read next.
 * @return true if the next event is available, false otherwise
 */
bool Reader::hasNext() {return eventIndex.canAdvance();}

/**
 * Checks if the stream has previous event to be accessed through, getPrevEvent()
 * @return true if previous event is accessible, false otherwise
 */
bool Reader::hasPrev() {return eventIndex.canRetreat();}

/**
 * Get the number of events in current record.
 * @return number of events in current record.
 */
int Reader::getRecordEventCount() {return inputRecordStream.getEntries();}

/**
 * Get the index of the current record.
 * @return index of the current record.
 */
int Reader::getCurrentRecord() {return currentRecordLoaded;}

/**
 * Get the current record stream.
 * @return current record stream.
 */
RecordInput Reader::getCurrentRecordStream() {return inputRecordStream;}

/**
 * Reads record from the file/buffer at the given record index.
 * @param index record index  (starting at 0).
 * @return true if valid index and successful reading record, else false.
 * @throws HipoException if file/buffer not in hipo format
 */
bool Reader::readRecord(int index) {
    if (index >= 0 && index < recordPositions.size()) {
        RecordPosition pos = recordPositions.get(index);
        if (fromFile) {
            inputRecordStream.readRecord(inStreamRandom, pos.getPosition());
        }
        else {
            inputRecordStream.readRecord(buffer, (int)pos.getPosition());
        }
        currentRecordLoaded = index;
        return true;
    }
    return false;
}


/** Extract dictionary and first event from file/buffer if possible, else do nothing. */
void Reader::extractDictionaryAndFirstEvent() {
    // If already read & parsed ...
    if (dictionaryXML != nullptr || firstEvent != nullptr) {
        return;
    }

    if (fromFile) {
        extractDictionaryFromFile();
        return;
    }
    extractDictionaryFromBuffer();
}

/** Extract dictionary and first event from buffer if possible, else do nothing. */
void Reader::extractDictionaryFromBuffer() {

    // If no dictionary or first event ...
    if (!firstRecordHeader.hasDictionary() && !firstRecordHeader.hasFirstEvent()) {
        return;
    }

    int userLen = firstRecordHeader.getUserHeaderLength();
    // 8 byte min for evio event, more for xml dictionary
    if (userLen < 8) {
        return;
    }

    RecordInput record;

    try {
        // Position right before record header's user header
        buffer.position(bufferOffset +
                        firstRecordHeader.getHeaderLength() +
                        firstRecordHeader.getIndexLength());
        // Read user header
        auto userBytes = new uint8_t[userLen];
        buffer.get(userBytes, 0, userLen);
        ByteBuffer userBuffer = ByteBuffer.wrap(userBytes);

        // Parse user header as record
        record = new RecordInput(firstRecordHeader.getByteOrder());
        record.readRecord(userBuffer, 0);
    }
    catch (HipoException & e) {
        // Not in proper format
        return;
    }

    int eventIndex = 0;

    // Dictionary always comes first in record
    if (firstRecordHeader.hasDictionary()) {
        // Just plain ascii, not evio format
        uint8_t * dict = record.getEvent(eventIndex++);
        try {dictionaryXML = string(dict, "US-ASCII");}
        catch (UnsupportedEncodingException e) {/* never happen */}
    }

    // First event comes next
    if (firstRecordHeader.hasFirstEvent()) {
        firstEvent = record.getEvent(eventIndex);
    }
}

/** Extract dictionary and first event from file if possible, else do nothing. */
void Reader::extractDictionaryFromFile() {
    // cout << "extractDictionaryFromFile: IN, hasFirst = " << fileHeader.hasFirstEvent() << endl;

    // If no dictionary or first event ...
    if (!fileHeader.hasDictionary() && !fileHeader.hasFirstEvent()) {
        return;
    }

    int userLen = fileHeader.getUserHeaderLength();
    // 8 byte min for evio event, more for xml dictionary
    if (userLen < 8) {
        return;
    }

    RecordInput record;

    try {
//            System.out.println("extractDictionaryFromFile: Read");
        // Position right before file header's user header
        inStreamRandom.getChannel().position(fileHeader.getHeaderLength() + fileHeader.getIndexLength());
        // Read user header
        byte[] userBytes = new byte[userLen];
        inStreamRandom.read(userBytes);
        ByteBuffer userBuffer = ByteBuffer.wrap(userBytes);
        // Parse user header as record
        record = RecordInput(fileHeader.getByteOrder());
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


/**
 * Reads data from a record header in order to determine things
 * like the bitInfo word, various lengths, etc.
 * Does <b>not</b> change the position or limit of buffer.
 *
 * @param  buf     buffer containing evio header.
 * @param  offset  byte offset into buffer.
 * @param  info    array in which to store header info. Elements are:
 *  <ol start="0">
 *      <li>bit info word</li>
 *      <li>record length in bytes (inclusive)</li>
 *      <li>compression type</li>
 *      <li>header length in bytes</li>
 *      <li>index array length in bytes</li>
 *      <li>user header length in bytes</li>
 *      <li>uncompressed data length in bytes (w/o record header)</li>
 *  </ol>
 * @throws BufferUnderflowException if not enough data in buffer.
 * @throws HipoException null info arg or info.length &lt; 7.
 */
void Reader::findRecordInfo(ByteBuffer & buf, uint32_t offset, int* info, uint32_t infoLen) {

    if (info == nullptr || infoLen < 7) {
        throw HipoException("null info arg or info length < 7");
    }
//        if (buf.capacity() - offset < 1000) {
//            System.out.println("findRecInfo: buf cap = " + buf.capacity() + ", offset = " + offset +
//                                       ", lim = " + buf.limit());
//        }
    // Have enough bytes to read 10 words of header?
    if ((buf.capacity() - offset) < 40) {
        cout << "findRecInfo: buf cap = " << buf.capacity() << ", offset = " << offset << ", lim = " <<  buf.limit() << endl;
        throw new BufferUnderflowException();
    }

    info[0] = buf.getInt(offset + RecordHeader::BIT_INFO_OFFSET);
    info[1] = buf.getInt(offset + RecordHeader::RECORD_LENGTH_OFFSET) * 4;
    info[2] = buf.getInt(offset + RecordHeader::COMPRESSION_TYPE_OFFSET) >>> 28;
    info[3] = buf.getInt(offset + RecordHeader::HEADER_LENGTH_OFFSET) * 4;
    info[4] = buf.getInt(offset + RecordHeader::INDEX_ARRAY_OFFSET);
    info[5] = buf.getInt(offset + RecordHeader::USER_LENGTH_OFFSET);
    info[6] = buf.getInt(offset + RecordHeader::UNCOMPRESSED_LENGTH_OFFSET);
}

/**
 * This method gets the total number of evio/hipo format bytes in
 * the given buffer, both compressed and uncompressed. Results are
 * stored in the given int array. First element is compressed length,
 * second is uncompressed length.
 *
 * @param buf   ByteBuffer containing evio/hipo data
 * @param info  integer array containing evio/hipo data. Elements are:
 *  <ol start="0">
 *      <li>compressed length in bytes</li>
 *      <li>uncompressed length in bytes</li>
  *  </ol>
 * @return total compressed hipo/evio data in bytes.
 * @throws BufferUnderflowException if not enough data in buffer.
 * @throws HipoException null arg(s), negative offset, or info.length &lt; 7.
 */
int Reader::getTotalByteCounts(ByteBuffer & buf, int[] info) {

//        if (buf == null || info == null || info.length < 7) {
//            throw new HipoException("bad arg or info.length < 7");
//        }

    int offset = buf.position();
    int totalCompressed = 0;
    int recordBytes, totalBytes = 0;

    do {
        // Look at the record
        findRecordInfo(buf, offset, info);

        // Total uncompressed length of record
        recordBytes = info[3] + info[4] + info[5] + info[6];

        // Track total uncompressed & compressed sizes
        totalBytes += recordBytes;
        totalCompressed += info[1];

        // Hop over record
        offset += info[1];

    } while (!RecordHeader.isLastRecord(info[0])); // Go to the next record if any

    // No longer input, we now use array for output
    info[0] = totalCompressed;
    info[1] = totalBytes;

    return totalCompressed;
}

/**
 * This method scans a buffer to find all records and store their position, length,
 * and event count. It also finds all events and creates & stores their associated
 * EvioNode objects.<p>
 *
 * The difficulty with doing this is that the buffer may contain compressed data.
 * It must then be uncompressed into a different buffer. There are 2 possibilities.
 * First, if the buffer being parsed is too small to hold its uncompressed form,
 * then a new, larger buffer is created, filled with the uncompressed data and then
 * given as the return value of this method. Second, if the buffer being parsed is
 * large enough to hold its uncompressed form, the data is uncompressed into a
 * temporary holding buffer. When all decompresssion and parsing is finished, the
 * contents of the temporary buffer are copied back into the original buffer which
 * then becomes the return value.
 *
 * @return buffer containing uncompressed data. This buffer may be different than the
 *         one originally scanned if the data was compressed and the uncompressed length
 *         is greater than the original buffer could hold.
 * @throws HipoException if buffer not in the proper format or earlier than version 6;
 *                       if checkRecordNumberSequence is true and records are out of sequence.
 * @throws BufferUnderflowException if not enough data in buffer.
 */
ByteBuffer Reader::scanBuffer() {

    // Quick check to see if data in buffer is compressed (pos/limit unchanged)
    if (RecordHeader.isCompressed(buffer, bufferOffset)) {
        // Since data is not compressed ...
        scanUncompressedBuffer();
        return buffer;
    }

    // The previous method call will set the endianness of the buffer properly.
    // Hop through ALL RECORDS to find their total lengths. This does NOT
    // change pos/limit of buffer.
    int totalCompressedBytes = getTotalByteCounts(buffer, headerInfo);
    int totalUncompressedBytes = headerInfo[1];

    ByteBuffer bigEnoughBuf;
    bool useTempBuffer = false;

//System.out.println("scanBuffer: total Uncomp bytes = " + totalUncompressedBytes +
//                " >? cap - off = " + (buffer.capacity() - bufferOffset));

    // If the buffer is too small to hold the expanded data, create one that isn't
    if (totalUncompressedBytes > (buffer.capacity() - bufferOffset)) {
        // Time for a bigger buffer. Give buffer an extra 4KB
        if (buffer.isDirect()) {
            bigEnoughBuf = ByteBuffer.allocateDirect(totalUncompressedBytes + bufferOffset + 4096);
            //bigEnoughBuf.order(buffer.order()).limit(bufferOffset + totalUncompressedBytes);
            bigEnoughBuf.order(buffer.order());

            // Copy in stuff up to offset
            buffer.limit(bufferOffset).position(0);
            bigEnoughBuf.put(buffer);
            // At this point, bigEnoughBuf.position() = bufferOffset.
            // Put stuff starting there.

            // Need to reset the limit & position from just having messed it up
            buffer.limit(totalCompressedBytes + bufferOffset).position(bufferOffset);
        }
        else {
            // Backed by array
            bigEnoughBuf = ByteBuffer.allocate(totalUncompressedBytes + bufferOffset + 4096);
            // Put stuff starting at bigEnoughBuf.position() = bufferOffset
            //bigEnoughBuf.order(buffer.order()).limit(bufferOffset + totalUncompressedBytes).position(bufferOffset);
            bigEnoughBuf.order(buffer.order()).position(bufferOffset);

            // Copy in stuff up to offset
            System.arraycopy(buffer.array(), buffer.arrayOffset(),
                             bigEnoughBuf.array(), 0, bufferOffset);
        }
    }
    else {
        // "buffer" is big enough to hold everything. However, we need another buffer
        // in which to temporarily decompress data which will then be copied back into
        // buffer. Don't bother to copy stuff from buffer.pos = 0 - bufferOffset, since
        // we'll be copying stuff back into buffer anyway.
        useTempBuffer = true;
        if ((tempBuffer == null) ||
            (tempBuffer.capacity() < totalUncompressedBytes + bufferOffset)) {
            tempBuffer = ByteBuffer.allocate(totalUncompressedBytes + bufferOffset + 4096);
        }
        tempBuffer.order(buffer.order()).limit(tempBuffer.capacity()).position(0);

        bigEnoughBuf = tempBuffer;
        // Put stuff starting at bigEnoughBuf.position() = 0.
    }

    bool isArrayBacked = (bigEnoughBuf.hasArray() && buffer.hasArray());
    bool haveFirstRecordHeader = false;

    RecordHeader recordHeader = new RecordHeader(HeaderType.EVIO_RECORD);

    // Start at the buffer's initial position
    int position  = bufferOffset;
    int recordPos = bufferOffset;
    int bytesLeft = totalUncompressedBytes;

//System.out.println("  scanBuffer: buffer pos = " + bufferOffset + ", bytesLeft = " + bytesLeft);
    // Keep track of the # of records, events, and valid words in file/buffer
    int eventCount = 0, byteLen, recordBytes, eventsInRecord;
    eventNodes.clear();
    recordPositions.clear();
    eventIndex.clear();
    // TODO: this should NOT change in records in 1 buffer, only BETWEEN buffers!!!!!!!!!!!!
    recordNumberExpected = 1;

    // Go through data record-by-record
    do {
        // If this is not the first record anymore, then the limit of bigEnoughBuf,
        // set above, may not be big enough.


        // Uncompress record in buffer and place into bigEnoughBuf
        int origRecordBytes = inputRecordStream.uncompressRecord(
                buffer, recordPos, bigEnoughBuf,
                isArrayBacked, recordHeader);

        // The only certainty at this point about pos/limit is that
        // bigEnoughBuf.position = after header/index/user, just before data.
        // What exactly the decompression methods do is unknown.

        // uncompressRecord(), above, read the header. Save the first header.
        if (!haveFirstRecordHeader) {
            // First time through, save byte order and version
            byteOrder = recordHeader.getByteOrder();
            buffer.order(byteOrder);
            evioVersion = recordHeader.getVersion();
            firstRecordHeader = new RecordHeader(recordHeader);
            compressed = recordHeader.getCompressionType() != 0;
            haveFirstRecordHeader = true;
        }

        //System.out.println("read header ->\n" + recordHeader);
        lastRecordNum = recordHeader.getRecordNumber();

        if (checkRecordNumberSequence) {
            if (recordHeader.getRecordNumber() != recordNumberExpected) {
                //System.out.println("  scanBuffer: record # out of sequence, got " + recordHeader.getRecordNumber() +
                //                   " expecting " + recordNumberExpected);
                throw new HipoException("bad record # sequence");
            }
            recordNumberExpected++;
        }

        // Check to see if the whole record is there
        if (recordHeader.getLength() > bytesLeft) {
            System.out.println("    record size = " + recordHeader.getLength() + " >? bytesLeft = " + bytesLeft +
                               ", pos = " + buffer.position());
            throw new HipoException("Bad hipo format: not enough data to read record");
        }

        // Header is now describing the uncompressed buffer, bigEnoughBuf
        recordBytes = recordHeader.getLength();
        eventsInRecord = recordHeader.getEntries();
        RecordPosition rp = new RecordPosition(position, recordBytes, eventsInRecord);
        recordPositions.add(rp);
        // Track # of events in this record for event index handling
        eventIndex.addEventSize(eventsInRecord);

        // Next record position
        recordPos += origRecordBytes;

        // How many bytes left in the newly expanded buffer
        bytesLeft -= recordHeader.getUncompressedRecordLength();

        // After calling uncompressRecord(), bigEnoughBuf will be positioned
        // right before where the events start.
        position = bigEnoughBuf.position();

        // For each event in record, store its location
        for (int i=0; i < eventsInRecord; i++) {
            EvioNode node;
            try {
//System.out.println("      try extracting event " + i + ", pos = " + position +
//                                               ", place = " + (eventCount + i));
                node = EvioNode.extractEventNode(bigEnoughBuf, nodePool, 0,
                                                 position, eventCount + i);
            }
            catch (EvioException e) {
                throw new HipoException("Bad evio format: not enough data to read event (bad bank len?)", e);
            }
//System.out.println("      event " + i + ", pos = " + node.getPosition() +
//                           ", dataPos = " + node.getDataPosition() + ", ev # = " + (eventCount + i + 1));
            eventNodes.add(node);

            // Hop over event
            byteLen   = node.getTotalBytes();
            position += byteLen;

            if (byteLen < 8) {
                throw new HipoException("Bad evio format: bad bank length");
            }
//System.out.println("        hopped event " + i + ", bytesLeft = " + bytesLeft + ", pos = " + position + "\n");
        }

        bigEnoughBuf.position(position);
        eventCount += eventsInRecord;

        // If the evio buffer was written with the DAQ Java evio library,
        // there is only 1 record with event per buffer -- the first record.
        // It's followed by a trailer.

        // Read the next record if this is not the last one and there's enough data to
        // read a header.

    } while (!recordHeader.isLastRecord() && bytesLeft >= RecordHeader.HEADER_SIZE_BYTES);


    // At this point we have an uncompressed buffer in bigEnoughBuf.
    // If that is our temporary buf, we now copy it back into buffer
    // which we know will be big enough to handle it.
    if (useTempBuffer) {
        // Since we're using a temp buffer, it does NOT contain buffer's data
        // from position = 0 to bufferOffset.
        if (isArrayBacked) {
            System.arraycopy(bigEnoughBuf.array(), 0,
                             buffer.array(), bufferOffset + buffer.arrayOffset(),
                             totalUncompressedBytes);
        }
        else {
            bigEnoughBuf.limit(totalUncompressedBytes).position(0);
            buffer.position(bufferOffset);
            buffer.put(bigEnoughBuf);
        }

        // Restore the original position and set new limit
        buffer.limit(bufferOffset + totalUncompressedBytes).position(bufferOffset);

        // We've copied data from one buffer to another,
        // so adjust the nodes to compensate.
        for (EvioNode n : eventNodes) {
            n.shift(bufferOffset).setBuffer(buffer);
        }
    }
    else {
        // We had to allocate memory in this method since buffer was too small,
        // so return the new larger buffer.
        bigEnoughBuf.position(bufferOffset);
        buffer = bigEnoughBuf;
    }

    return buffer;
}

/**
  * Scan buffer to find all records and store their position, length, and event count.
  * Also finds all events and creates & stores their associated EvioNode objects.
  * @throws HipoException if buffer too small, not in the proper format, or earlier than version 6;
  *                       if checkRecordNumberSequence is true and records are out of sequence.
  */
void Reader::scanUncompressedBuffer() {

// TODO: This uses memory & garbage collection
    byte[] headerBytes = new byte[RecordHeader.HEADER_SIZE_BYTES];
    ByteBuffer headerBuffer = ByteBuffer.wrap(headerBytes);

// TODO: This uses memory & garbage collection
    RecordHeader recordHeader = new RecordHeader();


    bool haveFirstRecordHeader = false;

    // Start at the buffer's initial position
    int position  = bufferOffset;
    int bytesLeft = bufferLimit - bufferOffset;

    //System.out.println("  scanBuffer: buffer pos = " + bufferOffset + ", bytesLeft = " + bytesLeft);
    // Keep track of the # of records, events, and valid words in file/buffer
    int eventCount = 0, byteLen, recordBytes, eventsInRecord, recPosition;
    eventNodes.clear();
    recordPositions.clear();
    eventIndex.clear();
    // TODO: this should NOT change in records in 1 buffer, only BETWEEN buffers!!!!!!!!!!!!
    recordNumberExpected = 1;

    while (bytesLeft >= RecordHeader.HEADER_SIZE_BYTES) {
        // Read record header
        buffer.position(position);
        // This moves the buffer's position
        buffer.get(headerBytes, 0, RecordHeader.HEADER_SIZE_BYTES);
        // Only sets the byte order of headerBuffer
        recordHeader.readHeader(headerBuffer);
        //System.out.println("read header ->\n" + recordHeader);
        lastRecordNum = recordHeader.getRecordNumber();

        if (checkRecordNumberSequence) {
            if (recordHeader.getRecordNumber() != recordNumberExpected) {
                //System.out.println("  scanBuffer: record # out of sequence, got " + recordHeader.getRecordNumber() +
                //                   " expecting " + recordNumberExpected);
                throw new HipoException("bad record # sequence");
            }
            recordNumberExpected++;
        }

        // Save the first record header
        if (!haveFirstRecordHeader) {
            // First time through, save byte order and version
            byteOrder = recordHeader.getByteOrder();
            buffer.order(byteOrder);
            evioVersion = recordHeader.getVersion();
            firstRecordHeader = new RecordHeader(recordHeader);
            compressed = recordHeader.getCompressionType() != 0;
            haveFirstRecordHeader = true;
        }

        // Check to see if the whole record is there
        if (recordHeader.getLength() > bytesLeft) {
            System.out.println("    record size = " + recordHeader.getLength() + " >? bytesLeft = " + bytesLeft +
                               ", pos = " + buffer.position());
            throw new HipoException("Bad hipo format: not enough data to read record");
        }

        //System.out.println(">>>>>==============================================");
        //System.out.println(recordHeader.toString());
        recordBytes = recordHeader.getLength();
        eventsInRecord = recordHeader.getEntries();
        RecordPosition rp = new RecordPosition(position, recordBytes, eventsInRecord);
        recPosition = position;
        //System.out.println(" RECORD HEADER ENTRIES = " + eventsInRecord);
        recordPositions.add(rp);
        // Track # of events in this record for event index handling
        eventIndex.addEventSize(eventsInRecord);

        // Hop over record header, user header, and index to events
        byteLen = recordHeader.getHeaderLength() +
                  recordHeader.getUserHeaderLength() +
                  recordHeader.getIndexLength();
        position  += byteLen;
        bytesLeft -= byteLen;

        // Do this because extractEventNode uses the buffer position
        buffer.position(position);
        //System.out.println("    hopped to data, pos = " + position);

        // For each event in record, store its location
        for (int i=0; i < eventsInRecord; i++) {
            EvioNode node;
            try {
                //System.out.println("      try extracting event "+i+" in record pos = " + recPosition + ", pos = " + position +
                //                                               ", place = " + (eventCount + i));
                node = EvioNode.extractEventNode(buffer, nodePool, recPosition,
                                                 position, eventCount + i);
            }
            catch (EvioException e) {
                throw new HipoException("Bad evio format: not enough data to read event (bad bank len?)", e);
            }
            //System.out.println("      event "+i+" in record: pos = " + node.getPosition() +
            //                           ", dataPos = " + node.getDataPosition() + ", ev # = " + (eventCount + i + 1));
            eventNodes.add(node);

            // Hop over event
            byteLen    = node.getTotalBytes();
            position  += byteLen;
            bytesLeft -= byteLen;

            if (byteLen < 8 || bytesLeft < 0) {
                throw new HipoException("Bad evio format: bad bank length");
            }

            //cout << "        hopped event " << i << ", bytesLeft = " << bytesLeft << ", pos = " << position << endl << endl;
        }

        eventCount += eventsInRecord;
    }

    buffer.position(bufferOffset);
}


/**
 * Scan file to find all records and store their position, length, and event count.
 * Safe to call this method successively.
 * @throws IOException   if error reading file
 * @throws HipoException if file is not in the proper format or earlier than version 6;
 *                       if checkRecordNumberSequence is true and records are out of sequence.
 */
void Reader::forceScanFile() {

    FileChannel channel;
    auto headerBytes = new char[RecordHeader::HEADER_SIZE_BYTES];
    ByteBuffer headerBuffer = ByteBuffer(headerBytes, RecordHeader::HEADER_SIZE_BYTES);

    // Read and parse file header if we have't already done so in scanFile()
    if (fileHeader == nullptr) {
        fileHeader = FileHeader();
        // Go to file beginning
        channel = inStreamRandom.getChannel().position(0L);
        inStreamRandom.read(headerBytes);
        fileHeader.readHeader(headerBuffer);
        byteOrder = fileHeader.getByteOrder();
        evioVersion = fileHeader.getVersion();
//cout << "forceScanFile: file header -->" << endl << fileHeader << endl;
    }
    else {
//cout << "forceScanFile: already read file header, so set position to read" << endl;
        // If we've already read in the header, position file to read what follows
        channel = inStreamRandom.getChannel().position(RecordHeader::HEADER_SIZE_BYTES);
    }

    int recordLen;
    eventIndex.clear();
    recordPositions.clear();
    recordNumberExpected = 1;
    RecordHeader recordHeader = RecordHeader();
    bool haveFirstRecordHeader = false;

    // Scan file by reading each record header and
    // storing its position, length, and event count.
    uint64_t fileSize = inStreamRandom.length();

    // Don't go beyond 1 header length before EOF since we'll be reading in 1 header
    uint64_t maximumSize = fileSize - RecordHeader::HEADER_SIZE_BYTES;

    // First record position (past file's header + index + user header)
    uint64_t recordPosition = fileHeader.getHeaderLength() +
                              fileHeader.getUserHeaderLength() +
                              fileHeader.getIndexLength();
//cout << "forceScanFile: record 1 pos = 0" << endl;
    int recordCount = 0;
    while (recordPosition < maximumSize) {
        channel.position(recordPosition);
//cout << "forceScanFile: record " << recordCount <<  " @ pos = " << recordPosition <<
//                   ", maxSize = " << maximumSize << endl;
        recordCount++;
        inStreamRandom.read(headerBytes);
        recordHeader.readHeader(headerBuffer);
//cout << "forceScanFile: record header " << recordCount << " -->" << endl << recordHeader << endl;

// TODO: checking record # sequence does NOT make sense when reading a file!
// It only makes sense when reading from a stream and checking to see
// if the record id, set by the sender, is sequential.

        if (checkRecordNumberSequence) {
            if (recordHeader.getRecordNumber() != recordNumberExpected) {
                cout << "forceScanFile: record # out of sequence, got " << recordHeader.getRecordNumber() <<
                                   " expecting " << recordNumberExpected << endl;

                throw HipoException("bad record # sequence");
            }
            recordNumberExpected++;
        }

        // Save the first record header
        if (!haveFirstRecordHeader) {
            firstRecordHeader = RecordHeader(recordHeader);
            compressed = firstRecordHeader.getCompressionType() != 0;
            haveFirstRecordHeader = true;
        }

        recordLen = recordHeader.getLength();
        RecordPosition pos = RecordPosition(recordPosition, recordLen,
                                                recordHeader.getEntries());
        recordPositions.add(pos);
        // Track # of events in this record for event index handling
        eventIndex.addEventSize(recordHeader.getEntries());
        recordPosition += recordLen;
    }
//eventIndex.show();
//cout << "NUMBER OF RECORDS " << recordPositions.size() << endl;
}

/**
 * Scans the file to index all the record positions.
 * It takes advantage of any existing indexes in file.
 * @param force if true, force a file scan even if header
 *              or trailer have index info.
 * @throws IOException   if error reading file
 * @throws HipoException if file is not in the proper format or earlier than version 6
 */
void Reader::scanFile(bool force) {
    //cout << endl << "scanFile IN:" << endl;

    if (force) {
        forceScanFile();
        return;
    }

    eventIndex.clear();
    recordPositions.clear();
//        recordNumberExpected = 1;

    //cout << "[READER] ---> scanning the file" << endl;
    auto headerBytes = new char[RecordHeader::HEADER_SIZE_BYTES];
    ByteBuffer headerBuffer = ByteBuffer(headerBytes, RecordHeader::HEADER_SIZE_BYTES);

    fileHeader = FileHeader();
    RecordHeader recordHeader = RecordHeader();

    // Go to file beginning
    FileChannel channel = inStreamRandom.getChannel().position(0L);

    // Read and parse file header
    inStreamRandom.read(headerBytes);
    fileHeader.readHeader(headerBuffer);
    byteOrder = fileHeader.getByteOrder();
    evioVersion = fileHeader.getVersion();

    // Is there an existing record length index?
    // Index in trailer gets first priority.
    // Index in file header gets next priority.
    bool fileHasIndex = fileHeader.hasTrailerWithIndex() || (fileHeader.hasIndex());
//cout << " file has index = " << fileHasIndex <<
//        "  " << fileHeader.hasTrailerWithIndex() <<
//        "  " << fileHeader.hasIndex() << endl;

    // If there is no index, scan file
    if (!fileHasIndex) {
        forceScanFile();
        return;
    }

    // If we're using the trailer, make sure it's position is valid,
    // (ie 0 is NOT valid).
    bool useTrailer = fileHeader.hasTrailerWithIndex();
    if (useTrailer) {
        // If trailer position is NOT valid ...
        if (fileHeader.getTrailerPosition() < 1) {
            cout << "scanFile: bad trailer position, " << fileHeader.getTrailerPosition() << endl;
            if (fileHeader.hasIndex()) {
                // Use file header index if there is one
                useTrailer = false;
            }
            else {
                // Scan if no viable index exists
                forceScanFile();
                return;
            }
        }
    }

    // First record position (past file's header + index + user header)
    uint32_t recordPosition = fileHeader.getLength();
    //cout << "record position = " << recordPosition << endl;

    // Move to first record and save the header
    channel.position(recordPosition);
    inStreamRandom.read(headerBytes);
    firstRecordHeader = RecordHeader(recordHeader);
    firstRecordHeader.readHeader(headerBuffer);
    compressed = firstRecordHeader.getCompressionType() != 0;

    int indexLength;

    // If we have a trailer with indexes ...
    if (useTrailer) {
        // Position read right before trailing header
        channel.position(fileHeader.getTrailerPosition());
//cout << "position file to trailer = " << fileHeader.getTrailerPosition() << endl;
        // Read trailer
        inStreamRandom.read(headerBytes);
        recordHeader.readHeader(headerBuffer);
        indexLength = recordHeader.getIndexLength();
    }
    else {
        // Move back to immediately past file header
        // while taking care of non-standard size
        channel.position(fileHeader.getHeaderLength());
        // Index immediately follows file header in this case
        indexLength = fileHeader.getIndexLength();
    }

    // Read indexes
    auto index = new uint8_t[indexLength];
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
//System.out.println("record pos = " + recordPosition + ", len = " + len + ", count = " + count);
            RecordPosition pos = RecordPosition(recordPosition, len, count);
            recordPositions.add(pos);
            // Track # of events in this record for event index handling
//System.out.println("scanFile: add record's event count (" + count + ") to eventIndex");
//Utilities.printStackTrace();
            eventIndex.addEventSize(count);
            recordPosition += len;
        }
    }
    catch (HipoException & e) {/* never happen */}
}



/**
 * This method removes the data, represented by the given node, from the buffer.
 * It also marks all nodes taken from that buffer as obsolete.
 * They must not be used anymore.<p>
 *
 * @param  removeNode  evio structure to remove from buffer
 * @return ByteBuffer updated to reflect the node removal
 * @throws HipoException if object closed;
 *                       if node was not found in any event;
 *                       if internal programming error;
 *                       if buffer has compressed data;
 */
ByteBuffer Reader::removeStructure(EvioNode removeNode) {

    // If we're removing nothing, then DO nothing
    if (removeNode == nullptr) {
        return buffer;
    }

    if (closed) {
        throw HipoException("object closed");
    }
    else if (removeNode.isObsolete()) {
        //cout << "removeStructure: node has already been removed" << endl;
        return buffer;
    }

    if (firstRecordHeader.getCompressionType() != 0) {
        throw HipoException("cannot remove node from buffer of compressed data");
    }

    bool foundNode = false;

    // Locate the node to be removed ...
    outer:
    for (EvioNode ev : eventNodes) {
        // See if it's an event ...
        if (removeNode == ev) {
            foundNode = true;
            break;
        }

        for (EvioNode n : ev.getAllNodes()) {
            // The first node in allNodes is the event node,
            if (removeNode == n) {
                foundNode = true;
                break outer;
            }
        }
    }

    if (!foundNode) {
        throw HipoException("removeNode not found in any event");
    }

    // The data these nodes represent will be removed from the buffer,
    // so the node will be obsolete along with all its descendants.
    removeNode.setObsolete(true);

    //---------------------------------------------------
    // Remove structure. Keep using current buffer.
    // We'll move all data that came after removed node
    // to where removed node used to be.
    //---------------------------------------------------

    // Amount of data being removed
    int removeDataLen = removeNode.getTotalBytes();

    // Just after removed node (start pos of data being moved)
    int startPos = removeNode.getPosition() + removeDataLen;

    // Duplicate backing buffer
    ByteBuffer moveBuffer = buffer.duplicate().order(buffer.order());
    // Prepare to move data currently sitting past the removed node
    moveBuffer.position(startPos).limit(bufferLimit);

    // Set place to put the data being moved - where removed node starts
    buffer.position(removeNode.getPosition());
    // Copy it over
    buffer.put(moveBuffer);

    // Reset some buffer values
    buffer.position(bufferOffset);
    bufferLimit -= removeDataLen;
    buffer.limit(bufferLimit);

    // Reduce lengths of parent nodes
    EvioNode parent = removeNode.getParentNode();
    parent.updateLengths(-removeDataLen);

    // Reduce containing record's length
    int pos = removeNode.getRecordPosition();
    // Header length in words
    int oldLen = 4*buffer.getInt(pos);
    buffer.putInt(pos, (oldLen - removeDataLen)/4);
    // Uncompressed data length in bytes
    oldLen = buffer.getInt(pos + RecordHeader::UNCOMPRESSED_LENGTH_OFFSET);
    buffer.putInt(pos + RecordHeader::UNCOMPRESSED_LENGTH_OFFSET, oldLen - removeDataLen);

    // Invalidate all nodes obtained from the last buffer scan
    for (EvioNode ev : eventNodes) {
        ev.setObsolete(true);
    }

    // Now the evio data in buffer is in a valid state so rescan buffer to update everything
    scanBuffer();

    return buffer;
}


/**
 * This method adds an evio container (bank, segment, or tag segment) as the last
 * structure contained in an event. It is the responsibility of the caller to make
 * sure that the buffer argument contains valid evio data (only data representing
 * the structure to be added - not in file format with record header and the like)
 * which is compatible with the type of data stored in the given event.<p>
 *
 * To produce such evio data use {@link EvioBank#write(ByteBuffer)},
 * {@link EvioSegment#write(ByteBuffer)} or
 * {@link EvioTagSegment#write(ByteBuffer)} depending on whether
 * a bank, seg, or tagseg is being added.<p>
 *
 * The given buffer argument must be ready to read with its position and limit
 * defining the limits of the data to copy.
 *
 * @param eventNumber number of event to which addBuffer is to be added
 * @param addBuffer buffer containing evio data to add (<b>not</b> evio file format,
 *                  i.e. no record headers)
 * @return a new ByteBuffer object which is created and filled with all the data
 *         including what was just added.
 * @throws HipoException if eventNumber &lt; 1;
 *                       if addBuffer arg is empty or has non-evio format;
 *                       if addBuffer is opposite endian to current event buffer;
 *                       if added data is not the proper length (i.e. multiple of 4 bytes);
 *                       if the event number does not correspond to an existing event;
 *                       if there is an internal programming error;
 *                       if object closed
 */
ByteBuffer Reader::addStructure(uint32_t eventNumber, ByteBuffer & addBuffer) {

    if (addBuffer.remaining() < 8) {
        throw HipoException("empty or non-evio format buffer arg");
    }

    if (addBuffer.order() != byteOrder) {
        throw HipoException("trying to add wrong endian buffer");
    }

    if (eventNumber < 1) {
        throw HipoException("event number must be > 0");
    }

    if (closed) {
        throw HipoException("object closed");
    }

    EvioNode eventNode;
    try {
        eventNode = eventNodes.get(eventNumber - 1);
    }
    catch (IndexOutOfBoundsException e) {
        throw new HipoException("event " + eventNumber + " does not exist", e);
    }

    // Position in byteBuffer just past end of event
    int endPos = eventNode.getDataPosition() + 4*eventNode.getDataLength();

    // How many bytes are we adding?
    int appendDataLen = addBuffer.remaining();

    // Make sure it's a multiple of 4
    if (appendDataLen % 4 != 0) {
        throw new HipoException("data added is not in evio format");
    }

    //--------------------------------------------
    // Add new structure to end of specified event
    //--------------------------------------------

    // Create a new buffer
    ByteBuffer newBuffer = ByteBuffer(bufferLimit - bufferOffset + appendDataLen);
    newBuffer.order(byteOrder);

    // Copy beginning part of existing buffer into new buffer
    buffer.limit(endPos).position(bufferOffset);
    newBuffer.put(buffer);

    // Copy new structure into new buffer
    newBuffer.put(addBuffer);

    // Copy ending part of existing buffer into new buffer
    buffer.limit(bufferLimit).position(endPos);
    newBuffer.put(buffer);

    // Get new buffer ready for reading
    newBuffer.flip();
    buffer = newBuffer;
    bufferOffset = 0;
    bufferLimit  = newBuffer.limit();

    // Increase lengths of parent nodes
    EvioNode addToNode = eventNodes.get(eventNumber);
    EvioNode parent = addToNode.getParentNode();
    parent.updateLengths(appendDataLen);

    // Increase containing record's length
    int pos = addToNode.getRecordPosition();
    // Header length in words
    int oldLen = 4*buffer.getInt(pos);
    buffer.putInt(pos, (oldLen + appendDataLen)/4);
    // Uncompressed data length in bytes
    oldLen = buffer.getInt(pos + RecordHeader::UNCOMPRESSED_LENGTH_OFFSET);
    buffer.putInt(pos + RecordHeader::UNCOMPRESSED_LENGTH_OFFSET, oldLen + appendDataLen);

    // Invalidate all nodes obtained from the last buffer scan
    for (EvioNode ev : eventNodes) {
        ev.setObsolete(true);
    }

    // Now the evio data in buffer is in a valid state so rescan buffer to update everything
    scanBuffer();

    return buffer;
}


void Reader::show() {
    cout << " ***** FILE: (info), RECORDS = " << recordPositions.size() << " *****" << endl;
    for (RecordPosition entry : this->recordPositions) {
        cout << entry;
    }
}

int Reader::main(int argc, char **argv) {
    try {
        Reader reader = Reader("/Users/gavalian/Work/Software/project-3a.0.0/Distribution/clas12-offline-software/coatjava/clas_000810_324.hipo",true);

        int icounter = 0;
        //reader.show();
        while(reader.hasNext()==true){
            cout << " reading event # " << icounter << endl;
            try {
                uint8_t* event = reader.getNextEvent();
            } catch (HipoException ex) {
                //Logger.getLogger(Reader.class.getName()).log(Level.SEVERE, null, ex);
            }

            icounter++;
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
    catch (Exception e) {
        e.printStackTrace();
    }

}