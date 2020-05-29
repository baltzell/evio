/**
 * Copyright (c) 2019, Jefferson Science Associates
 *
 * Thomas Jefferson National Accelerator Facility
 * Data Acquisition Group
 *
 * 12000, Jefferson Ave, Newport News, VA 23606
 * Phone : (757)-269-7100
 *
 * @date 07/05/2019
 * @author timmer
 */


#include <functional>
#include <string>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <chrono>
#include <thread>
#include <memory>
#include <regex>
#include <iterator>

#ifndef __APPLE__
#include <experimental/filesystem>
#endif

#include "ByteBuffer.h"
#include "ByteOrder.h"
#include "Writer.h"
#include "Reader.h"
#include "WriterMT.h"
#include "RecordHeader.h"
#include "EvioException.h"
#include "RecordRingItem.h"
#include "EvioNode.h"
#include "Compressor.h"
#include "EventWriter.h"
#include "TNode.h"
#include "TNodeSuper1.h"
#include "TNodeSuper2.h"

using namespace std;

#ifndef __APPLE__
namespace fs = std::experimental::filesystem;
#endif

namespace evio {


class ReadWriteTest {


public:


    static string xmlDict;


    static uint8_t* generateArray() {
        //double d = static_cast<double> (rand()) / static_cast<double> (RAND_MAX);
        int size = rand() % 35;
        size += 100;
        auto buffer = new uint8_t[size];
        for(int i = 0; i < size; i++){
            buffer[i] = (uint8_t)(rand() % 126);
        }
        return buffer;
    }

    static uint8_t* generateArray(int size) {
        uint8_t* buffer = new uint8_t[size];
        for(int i = 0; i < size; i++){
            buffer[i] =  (uint8_t) ((rand() % 125) + 1);
        }
        return buffer;
    }

    /**
     * Write ints.
     * @param size number of INTS
     * @param order byte order of ints in memory
     * @return
     */
    static uint8_t* generateSequentialInts(int size, ByteOrder & order) {
        auto buffer = new uint32_t[size];
        for(int i = 0; i < size; i++) {
            if (ByteOrder::needToSwap(order)) {
                buffer[i] = SWAP_32(i);
                //buffer[i] = SWAP_32(1);
            }
            else {
                buffer[i] = i;
                //buffer[i] = 1;
            }
        }
        return reinterpret_cast<uint8_t *>(buffer);
    }

    /**
     * Write shorts.
     * @param size number of SHORTS
     * @param order byte order of shorts in memory
     * @return
     */
    static uint8_t* generateSequentialShorts(int size, ByteOrder & order) {
        auto buffer = new uint16_t[size];
        for(int i = 0; i < size; i++) {
            if (ByteOrder::needToSwap(order)) {
                buffer[i] = SWAP_16(i);
                //buffer[i] = SWAP_16(1);
            }
            else {
                buffer[i] = i;
                //buffer[i] = 1;
            }
        }
        return reinterpret_cast<uint8_t *>(buffer);
    }

    static void print(uint8_t* array, int arrayLen) {
        int wrap = 20;
        for (int i = 0; i < arrayLen; i++) {
            cout << setw(3) << array[i];
            if ((i+1)%wrap == 0) cout << endl;
        }
        cout << endl;
    }


    static std::shared_ptr<ByteBuffer> generateEvioBuffer(ByteOrder & order) {
        // Create an evio bank of ints
        auto evioDataBuf = std::make_shared<ByteBuffer>(20);
        evioDataBuf->order(order);
        evioDataBuf->putInt(4);  // length in words, 5 ints
        evioDataBuf->putInt(0xffd10100);  // 2nd evio header word   (prestart event)
        evioDataBuf->putInt(0x1234);  // time
        evioDataBuf->putInt(0x5);  // run #
        evioDataBuf->putInt(0x6);  // run type
        evioDataBuf->flip();
        Util::printBytes(*(evioDataBuf.get()), 0, 20, "Original buffer");
        return evioDataBuf;
    }


    // Create a fake Evio Event
    static std::shared_ptr<ByteBuffer> generateEvioBuffer(ByteOrder & order, uint32_t dataWords) {

        // Create an evio bank of banks, containing a bank of ints
        auto evioDataBuf = std::make_shared<ByteBuffer>(16 + 4*dataWords);
        evioDataBuf->order(order);
        evioDataBuf->putInt(3+dataWords);  // event length in words

        uint32_t tag  = 0x1234;
        uint32_t type = 0x10;  // contains evio banks
        uint32_t num  = 0x12;
        uint32_t secondWord = tag << 16 | type << 4 | num;

        evioDataBuf->putInt(secondWord);  // 2nd evio header word

        // now put in a bank of ints
        evioDataBuf->putInt(1+dataWords);  // bank of ints length in words
        tag = 0x5678; type = 0x1; num = 0x56;
        secondWord = tag << 16 | type << 4 | num;
        evioDataBuf->putInt(secondWord);  // 2nd evio header word

        // Int data
        for (uint32_t i=0; i < dataWords; i++) {
            evioDataBuf->putInt(i);
        }

        evioDataBuf->flip();
        //Util::printBytes(*(evioDataBuf.get()), 0, 20, "Original buffer");
        return evioDataBuf;
    }


    static void writeFile(string finalFilename) {

        // Variables to track record build rate
        double freqAvg;
        long totalC = 0;
        long loops = 3;

        string dictionary = "This is a dictionary";
        //dictionary = "";
        uint8_t firstEvent[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        uint32_t firstEventLen = 10;
        bool addTrailerIndex = true;
        ByteOrder order = ByteOrder::ENDIAN_LITTLE;
        //Compressor::CompressionType compType = Compressor::GZIP;
        Compressor::CompressionType compType = Compressor::UNCOMPRESSED;

        // Possible user header data
        auto userHdr = new uint8_t[10];
        for (uint8_t i = 0; i < 10; i++) {
            userHdr[i] = i;
        }

        // Create files
        Writer writer(HeaderType::EVIO_FILE, order, 0, 0,
                      dictionary, firstEvent, 10, compType, addTrailerIndex);
        //writer.open(finalFilename);
        writer.open(finalFilename, userHdr, 10);
        cout << "Past creating writer1" << endl;

        //uint8_t *dataArray = generateSequentialInts(100, order);
        uint8_t *dataArray = generateSequentialShorts(13, order);
        // Calling the following method makes a shared pointer out of dataArray, so don't delete
        ByteBuffer dataBuffer(dataArray, 26);

        // Create an evio bank of ints
        auto evioDataBuf = generateEvioBuffer(order);
        // Create node from this buffer
        std::shared_ptr<EvioNode> node = EvioNode::extractEventNode(evioDataBuf,0,0,0);

        auto t1 = std::chrono::high_resolution_clock::now();

        while (true) {
            // random data array
            //writer.addEvent(dataArray, 0, 26);
            writer.addEvent(dataBuffer);

//cout << int(20000000 - loops) << endl;
            totalC++;

            if (--loops < 1) break;
        }

        cout << " node's type = " << node->getTypeObj().toString() << endl;
        writer.addEvent(*node.get());

        auto t2 = std::chrono::high_resolution_clock::now();
        auto deltaT = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);

        freqAvg = (double) totalC / deltaT.count() * 1000;

        cout << "Time = " << deltaT.count() << " msec,  Hz = " << freqAvg << endl;
        cout << "Finished all loops, count = " << totalC << endl;

        //------------------------------
        // Add entire record at once
        //------------------------------

        RecordOutput recOut(order);
        recOut.addEvent(dataArray, 0, 26);
        writer.writeRecord(recOut);

        //------------------------------

        //writer1.addTrailer(true);
        //writer.addTrailerWithIndex(addTrailerIndex);
        cout << "Past write" << endl;

        writer.close();
        cout << "Past close" << endl;

        // Doing a diff between files shows they're identical!

        cout << "Finished writing file " << finalFilename << " now read it" << endl;

        //delete[] dataArray;
        delete[] userHdr;
    }


    static void writeFileMT(string fileName) {

        // Variables to track record build rate
        double freqAvg;
        long totalC = 0;
        long loops = 3;


        string dictionary = "This is a dictionary";
        //dictionary = "";
        uint8_t firstEvent[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        uint32_t firstEventLen = 10;
        bool addTrailerIndex = true;
        ByteOrder order = ByteOrder::ENDIAN_LITTLE;
        //Compressor::CompressionType compType = Compressor::GZIP;
        Compressor::CompressionType compType = Compressor::UNCOMPRESSED;

        // Possible user header data
        auto userHdr = new uint8_t[10];
        for (uint8_t i = 0; i < 10; i++) {
            userHdr[i] = i;
        }

        // Create files
        string finalFilename1 = fileName;
        WriterMT writer1(HeaderType::EVIO_FILE, order, 0, 0,
                         dictionary, firstEvent, 10, compType, 2, addTrailerIndex, 16);
        writer1.open(finalFilename1, userHdr, 10);
        cout << "Past creating writer1" << endl;

        //uint8_t *dataArray = generateSequentialInts(100, order);
        uint8_t *dataArray = generateSequentialShorts(13, order);
        // Calling the following method makes a shared pointer out of dataArray, so don't delete
        ByteBuffer dataBuffer(dataArray, 26);

        // Create an evio bank of ints
        auto evioDataBuf = generateEvioBuffer(order);
        // Create node from this buffer
        std::shared_ptr<EvioNode> node = EvioNode::extractEventNode(evioDataBuf,0,0,0);

        auto t1 = std::chrono::high_resolution_clock::now();

        while (true) {
            // random data dataArray
            //writer1.addEvent(buffer, 0, 26);
            writer1.addEvent(dataBuffer);

//cout << int(20000000 - loops) << endl;
            totalC++;

            if (--loops < 1) break;
        }

        writer1.addEvent(*node.get());

        auto t2 = std::chrono::high_resolution_clock::now();
        auto deltaT = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);

        freqAvg = (double) totalC / deltaT.count() * 1000;

        cout << "Time = " << deltaT.count() << " msec,  Hz = " << freqAvg << endl;
        cout << "Finished all loops, count = " << totalC << endl;

        //------------------------------
        // Add entire record at once
        //------------------------------

        RecordOutput recOut(order);
        recOut.addEvent(dataArray, 0, 26);
        writer1.writeRecord(recOut);

        //------------------------------

        //writer1.addTrailer(true);
        writer1.addTrailerWithIndex(addTrailerIndex);
        cout << "Past write" << endl;

        writer1.close();
        cout << "Past close" << endl;

        // Doing a diff between files shows they're identical!

        cout << "Finished writing file " << fileName << ", now read it in" << endl;

        //delete[] dataArray;
        delete[] userHdr;
    }


//    explicit EventWriter(string & filename,
//    const ByteOrder & byteOrder = ByteOrder::nativeOrder(),
//    bool append = false);
//
//    EventWriter(string & file,
//    string & dictionary,
//    const ByteOrder & byteOrder = ByteOrder::nativeOrder(),
//    bool append = false);
//
//    EventWriter(string baseName, const string & directory, const string & runType,
//            uint32_t runNumber, uint64_t split, uint32_t maxRecordSize, uint32_t maxEventCount,
//    const ByteOrder & byteOrder, const string & xmlDictionary, bool overWriteOK,
//    bool append, EvioBank * firstEvent, uint32_t streamId, uint32_t splitNumber,
//    uint32_t splitIncrement, uint32_t streamCount, Compressor::CompressionType compressionType,
//            uint32_t compressionThreads, uint32_t ringSize, uint32_t bufferSize);
//
//    //---------------------------------------------
//    // BUFFER Constructors
//    //---------------------------------------------
//
//    EventWriter(std::shared_ptr<ByteBuffer> & buf);
//    EventWriter(std::shared_ptr<ByteBuffer> & buf, string & xmlDictionary);
//    EventWriter(std::shared_ptr<ByteBuffer> & buf, uint32_t maxRecordSize, uint32_t maxEventCount,
//    const string & xmlDictionary, uint32_t recordNumber, EvioBank *firstEvent,
//            Compressor::CompressionType compressionType);
//


    static string eventWriteFileMT(const string & filename) {

        // Variables to track record build rate
        double freqAvg;
        long totalC = 0;
        long loops = 6;


        string dictionary = "";

        uint8_t firstEvent[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
        uint32_t firstEventLen = 10;
        bool addTrailerIndex = true;
        ByteOrder order = ByteOrder::ENDIAN_BIG;
        //Compressor::CompressionType compType = Compressor::GZIP;
        Compressor::CompressionType compType = Compressor::UNCOMPRESSED;

        // Create files
        string directory;
        uint32_t runNum = 123;
        uint64_t split = 000000; // 2 MB
        uint32_t maxRecordSize = 0; // 0 -> use default
        uint32_t maxEventCount = 2; // 0 -> use default
        bool overWriteOK = true;
        bool append = true;
        uint32_t streamId = 3;
        uint32_t splitNumber = 2;
        uint32_t splitIncrement = 1;
        uint32_t streamCount = 2;
        uint32_t compThreads = 2;
        uint32_t ringSize = 16;
        uint32_t bufSize = 1;

            EventWriter writer(filename, directory, "runType",
                               runNum, split, maxRecordSize, maxEventCount,
                               order, dictionary, overWriteOK, append,
                               nullptr, streamId, splitNumber, splitIncrement, streamCount,
                               compType, compThreads, ringSize, bufSize);

//            string firstEv = "This is the first event";
//            ByteBuffer firstEvBuf(firstEv.size());
//            Util::stringToASCII(firstEv, firstEvBuf);
//            writer.setFirstEvent(firstEvBuf);


//            //uint8_t *dataArray = generateSequentialInts(100, order);
//            uint8_t *dataArray = generateSequentialShorts(13, order);
//            // Calling the following method makes a shared pointer out of dataArray, so don't delete
//            ByteBuffer dataBuffer(dataArray, 26);

        //  When appending, it's possible the byte order gets switched
        order = writer.getByteOrder();

        // Create an event containing a bank of ints
            auto evioDataBuf = generateEvioBuffer(order, 10);

            // Create node from this buffer
            std::shared_ptr<EvioNode> node = EvioNode::extractEventNode(evioDataBuf,0,0,0);

            auto t1 = std::chrono::high_resolution_clock::now();

            while (true) {
                cout << "Write event ~ ~ ~ " << endl;
                // event in evio format
                writer.writeEvent(*(evioDataBuf.get()));

                totalC++;

                if (--loops < 1) break;
            }

            //writer.addEvent(*node.get());

//            auto t2 = std::chrono::high_resolution_clock::now();
//            auto deltaT = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
//
//            freqAvg = (double) totalC / deltaT.count() * 1000;
//
//            cout << "Time = " << deltaT.count() << " msec,  Hz = " << freqAvg << endl;
//            cout << "Finished all loops, count = " << totalC << endl;

            //------------------------------

            //cout << "Past write, sleep for 2 sec ..., then close" << endl;
            //std::this_thread::sleep_for(2s);

            writer.close();
            cout << "Past close" << endl;

            // Doing a diff between files shows they're identical!

            cout << "Finished writing file " << writer.getCurrentFilename() << ", now read it in" << endl;
            return writer.getCurrentFilename();

    }






        static void readFile(string finalFilename) {

            Reader reader1(finalFilename);
            ByteOrder order = reader1.getByteOrder();

            int32_t evCount = reader1.getEventCount();
            cout << "Read in file " << finalFilename << ", got " << evCount << " events" << endl;

            string dict = reader1.getDictionary();
            cout << "   Got dictionary = " << dict << endl;

            shared_ptr<uint8_t> & pFE = reader1.getFirstEvent();
            if (pFE != nullptr) {
                int32_t feBytes = reader1.getFirstEventSize();
                cout << "   First Event bytes = " << feBytes << endl;
                cout << "   First Event values = " << endl << "   ";
                for (int i = 0; i < feBytes; i++) {
                    cout << (uint32_t) ((pFE.get())[i]) << ",  ";
                }
                cout << endl;
            }

            cout << "reader.getEvent(0)" << endl;
            shared_ptr<uint8_t> data = reader1.getEvent(0);
            cout << "got event" << endl;

            uint32_t wordLen = reader1.getEventLength(0)/2;
            if (data != nullptr) {
                short *pData = reinterpret_cast<short *>(data.get());
                cout <<  "   Event #0, values =" << endl << "   ";
                for (int i = 0; i < wordLen; i++) {
                    if (order.isLocalEndian()) {
                        cout << *pData << ",  ";
                    }
                    else {
                        cout << SWAP_16(*pData) <<",  ";
                    }
                    pData++;
                    if ((i+1)%5 == 0) cout << endl;
                }
                cout << endl;
            }

        }


        static void readFile2(string finalFilename) {

            Reader reader1(finalFilename);
            ByteOrder order = reader1.getByteOrder();

            int32_t evCount = reader1.getEventCount();
            cout << "Read in file " << finalFilename << ", got " << evCount << " events" << endl;

            string dict = reader1.getDictionary();
            cout << "   Got dictionary = " << dict << endl;

            shared_ptr<uint8_t> & pFE = reader1.getFirstEvent();
            if (pFE != nullptr) {
                int32_t feBytes = reader1.getFirstEventSize();
                auto feData = reinterpret_cast<char *>(pFE.get());
                string feString(feData, feBytes);

                cout << "First event = " << feString << endl;
            }

            cout << "reader.getEvent(0)" << endl;
            shared_ptr<uint8_t> data = reader1.getEvent(0);
            cout << "got event" << endl;

            uint32_t wordLen = reader1.getEventLength(0)/4;
            if (data != nullptr) {
                uint32_t *pData = reinterpret_cast<uint32_t *>(data.get());
                cout <<  "   Event #0, values =" << endl << "   ";
                for (int i = 0; i < wordLen; i++) {
                    if (order.isLocalEndian()) {
                        cout << *pData << ",  ";
                    }
                    else {
                        cout << SWAP_32(*pData) <<",  ";
                    }
                    pData++;
                    if ((i+1)%5 == 0) cout << endl;
                }
                cout << endl;
            }

        }



        static void convertor() {
        string filenameIn("/dev/shm/hipoTest1.evio");
        string filenameOut("/dev/shm/hipoTestOut.evio");
        try {
            Reader reader(filenameIn);
            uint32_t nevents = reader.getEventCount();

            cout << "     OPENED FILE " << filenameOut << " for writing " << nevents << " events to " << filenameOut << endl;
            Writer writer(filenameOut, ByteOrder::ENDIAN_LITTLE, 10000, 8*1024*1024);
            //writer.setCompressionType(Compressor::LZ4);

            for (int i = 0; i < nevents; i++) {
                cout << "     Try getting EVENT # " << i << endl;
                shared_ptr<uint8_t> pEvent = reader.getEvent(i);
                cout << "     Got event " << i << endl;
                uint32_t eventLen = reader.getEventLength(i);
                cout << "     Got event len = " << eventLen << endl;

                writer.addEvent(pEvent.get(), 0, eventLen);
            }
            cout << "     converter END" << endl;
            writer.close();
        } catch (EvioException & ex) {
            cout << ex.what() << endl;
        }

    }


//    static void disruptorTest() {
//
//        const size_t ringBufSize = 8;
//
//        std::array<int, ringBufSize> events;
//        for (size_t i = 0; i < ringBufSize; i++) events[i] = 2*i;
//
//
//        // For single threaded producer which spins to wait.
//        // This creates and contains a RingBuffer object.
//        disruptor::Sequencer<int, ringBufSize, disruptor::SingleThreadedStrategy<ringBufSize>,
//                disruptor::BusySpinStrategy> sequencer(events);
//
//        disruptor::Sequence readSequence(disruptor::kInitialCursorValue);
//        std::vector<disruptor::Sequence*> dependents = {&readSequence};
//
////        int64_t cursor = sequencer.GetCursor();
////        bool hasCap = sequencer.HasAvailableCapacity();
////
////        const disruptor::SequenceBarrier<disruptor::BusySpinStrategy> & barrier = sequencer.NewBarrier(dependents);
////        int64_t seq = barrier.get_sequence();
//
//        disruptor::Sequence & cursorSequence = sequencer.GetCursorSequence();
//        disruptor::SequenceBarrier<disruptor::BusySpinStrategy> barrier(cursorSequence, dependents);
//
//        shared_ptr<disruptor::SequenceBarrier<disruptor::BusySpinStrategy>> barrierPtr = sequencer.NewBarrier(dependents);
//        int64_t seq = barrierPtr->get_sequence();
//
//    }

};


    string ReadWriteTest::xmlDict {
            "<xmlDict>\n"
            "  <bank name=\"HallD\"             tag=\"6-8\"  type=\"bank\" >\n"
            "      <description format=\"New Format\" >hall_d_tag_range</description>\n"
            "      <bank name=\"DC(%t)\"        tag=\"6\" num=\"4\" >\n"
            "          <leaf name=\"xpos(%n)\"  tag=\"6\" num=\"5\" />\n"
            "          <bank name=\"ypos(%n)\"  tag=\"6\" num=\"6\" />\n"
            "      </bank >\n"
            "      <bank name=\"TOF\"     tag=\"8\" num=\"0\" >\n"
            "          <leaf name=\"x\"   tag=\"8\" num=\"1\" />\n"
            "          <bank name=\"y\"   tag=\"8\" num=\"2\" />\n"
            "      </bank >\n"
            "      <bank name=\"BCAL\"      tag=\"7\" >\n"
            "          <leaf name=\"x(%n)\" tag=\"7\" num=\"1-3\" />\n"
            "      </bank >\n"
            "  </bank >\n"
            "  <dictEntry name=\"JUNK\" tag=\"5\" num=\"0\" />\n"
            "  <dictEntry name=\"SEG5\" tag=\"5\" >\n"
            "       <description format=\"Old Format\" >tag 5 description</description>\n"
            "  </dictEntry>\n"
            "  <bank name=\"Rangy\" tag=\"75 - 78\" >\n"
            "      <leaf name=\"BigTag\" tag=\"76\" />\n"
            "  </bank >\n"
            "</xmlDict>\n"
    };

}




class Tree : public std::enable_shared_from_this<Tree> {

protected:

    /** this node in shared pointer form */
    std::shared_ptr<Tree> parent = nullptr;

    Tree() = default;


public:

    static std::shared_ptr<Tree> getInstance() {
        std::shared_ptr<Tree> pNode(new Tree());
        return pNode;
    }

    void setParent(const std::shared_ptr<Tree> &par) {
        if (par == nullptr) {
            parent = this->shared_from_this();
            cout << "Set parent to this->shared_from_this()" << endl;
        }
        else {
            parent = par;
            cout << "Setting parent to arg" << endl;
        }
    }

    ~Tree() {
        cout << "In destructor" << endl
        ;
    };

};


int main(int argc, char **argv) {
    //auto t1 = TNode::getInstance(5);
    auto t2 = TNodeSuper1::getInstance(22);
    auto t3 = TNodeSuper1::getInstance(33);
    auto t4 = TNodeSuper2::getInstance(44);

    t2->add(t3);
    t2->add(t4);

    // Now pull t3 out of the child vector of t2 ...
    auto t10 = t2->children[0];
    // Now pull t4 out of the child vector of t2 ...
    auto t11 = t2->children[1];

    std::cout << "main: call t10's myOverrideMethod " << endl;
    t10->myOverrideMethod();
    t10->whoAmI();
    std::cout << "main: t3's use count = " << t3.use_count() << endl;
    {
        auto t5 = t10->shared_from_this();
        std::shared_ptr<TNodeSuper1> t6 = static_pointer_cast<TNodeSuper1>(t10->shared_from_this());
        std::cout << "main: t3's use count after copy of 2x t10->shared_from_this() = " << t3.use_count() << endl;

        t6->sharedPtrBaseClassArg(t5);
        t6->baseClassArg(t5.get());
        std::cout << "main: t2 iterate over kids:" << endl;
        t2->iterateKids();

        std::shared_ptr<TNode> t7 = t6->shared_from_this();
        std::cout << "main: t3's use count after calling t6->shared_from_this() = " << t3.use_count() << endl;
    }
    std::cout << "main: t3's use count after copyies out-of-scope = " << t3.use_count() << endl;


    std::cout << "main: call t11's myOverrideMethod " << endl;
    t11->myOverrideMethod();

}
//
//int main75(int argc, char **argv) {
//
//    {
//        int i = 11;
//        auto t1 = evio::TreeNode<int>::getInstance(i);
//        auto t2 = evio::TreeNode<int>::getInstance(22);
//        auto t2_1 = evio::TreeNode<int>::getInstance(221);
//        auto t2_2 = evio::TreeNode<int>::getInstance(222);
//        auto t3 = evio::TreeNode<int>::getInstance(33);
//        auto t4 = evio::TreeNode<int>::getInstance(44);
//        auto t5 = evio::TreeNode<int>::getInstance(55);
//        auto t6 = evio::TreeNode<int>::getInstance(66);
//        auto t7 = evio::TreeNode<int>::getInstance(77);
//        auto t8 = evio::TreeNode<int>::getInstance(88);
//        auto t9 = evio::TreeNode<int>::getInstance(99);
//
//
//        //t1->setParent(nullptr);
//        t1->setUserObject(12);
//        t1->add(t2);
//        t1->add(t2_1);
//        t1->add(t2_2);
//        t2->add(t3);
//        t2->add(t4);
//        t2->add(t5);
//        t5->add(t6);
//        t5->add(t7);
//        t5->add(t8);
//        t5->add(t9);
//
//        cout << "t1 has myInt = " << t1->getUserObject() << ", t2 = " << t2->getUserObject() << endl;
//
//        cout << "t1 has # children = " << t1->getChildCount() << endl;
//        cout << "t2 has # children = " << t2->getChildCount() << endl;
//
//        //cout << "t1 leaf count = " << t1->getLeafCount() << endl;
//        cout << "t1 first leaf id = " << t1->getFirstLeaf()->getUserObject() << endl;
//
//        cout << "t2 parent id = " << t2->getParent()->getUserObject() << endl;
//
//        cout << "t1 next node id = " << t1->getNextNode()->getUserObject() << endl;
//        cout << "t2 next node id = " << t2->getNextNode()->getUserObject() << endl;
//        cout << "t3 next node id = " << t3->getNextNode()->getUserObject() << endl;
//        cout << "t4 next node id = " << t4->getNextNode()->getUserObject() << endl;
//        auto nextNode = t5->getNextNode();
//        if (nextNode == nullptr) {
//            cout << "t5 has NO next node" << endl;
//        }
//        else {
//            cout << "t5 next node id = " << t5->getNextNode()->getUserObject() << endl;
//        }
//        cout << "t5 prev node id = " << t5->getPreviousNode()->getUserObject() << endl;
//
//        cout << "t3 sibling count = " << t3->getSiblingCount() << endl;
//        cout << "t2 get child at 2 = " << t2->getChildAt(2)->getUserObject() << endl;
//        //cout << "t2 get child at 3 = " << t2->getChildAt(3)->getUserObject() << endl;
//
//        cout << "t4 next leaf = " << t4->getNextLeaf()->getUserObject() << endl;
//        cout << "t4 prev leaf = " << t4->getPreviousLeaf()->getUserObject() << endl;
//
//        std::vector<std::shared_ptr<evio::TreeNode<int>>> path = t5->getPath();
//        cout << "Path to t5 is (size = " << path.size() << ") :" << endl;
//        for (auto const & shptr : path) {
//            cout << shptr->getUserObject() << " -> ";
//        }
//        cout << endl;
//
//        cout << "t4 get root = " << t4->getRoot()->getUserObject() << endl;
//
//        cout << "t3 level = " << t3->getLevel() << endl;
//
//        cout << "t3 & t4 shared ancestor = " << t3->getSharedAncestor(t4)->getUserObject() << endl;
//        cout << "t1 & t5 shared ancestor = " << t1->getSharedAncestor(t5)->getUserObject() << endl;
//
//        cout << "t2 child after t3 = " << t2->getChildAfter(t3)->getUserObject() << endl;
//        auto kid = t2->getChildAfter(t5);
//        if (kid == nullptr) {
//            cout << "t2 child after t5 = nullptr" << endl;
//        }
//        else {
//            cout << "t2 child after t5 = " << t2->getChildAfter(t5)->getUserObject() << endl;
//        }
//
//        cout << "t2 child before t4 = " << t2->getChildBefore(t4)->getUserObject() << endl;
//
//        cout << "t4 prev sibling = " <<t4->getPreviousSibling()->getUserObject() << endl;
//        cout << "t4 next sibling = " <<t4->getNextSibling()->getUserObject() << endl;
//
//        cout << "Is t3 ancestor of t2? " << t2->isNodeAncestor(t3) << endl;
//        cout << "Is t2 ancestor of t3? " << t3->isNodeAncestor(t2) << endl;
//
//        cout << "Is t3 descendant of t2? " << t2->isNodeDescendant(t3) << endl;
//        cout << "Is t2 descendant of t3? " << t3->isNodeDescendant(t2) << endl;
//
//        cout << "Is t3 related to t2? " << t2->isNodeRelated(t3) << endl;
//        cout << "Is t2 related to t3? " << t3->isNodeRelated(t2) << endl;
//
//        cout << "Is t3 child of t2? " << t2->isNodeChild(t3) << endl;
//        cout << "Is t2 child of t3? " << t3->isNodeChild(t2) << endl;
//
//        cout << "Is t3 sibling of t2? " << t2->isNodeSibling(t3) << endl;
//        cout << "Is t3 sibling of t3? " << t3->isNodeSibling(t3) << endl;
//        cout << "Is t3 sibling of t4? " << t3->isNodeSibling(t4) << endl;
//
//        cout << "Is t1 root? " << t1->isRoot() << endl;
//        cout << "Is t2 root? " << t2->isRoot() << endl;
//
//        cout << "Is t2 leaf? " << t1->isLeaf() << endl;
//        cout << "Is t3 leaf? " << t3->isLeaf() << endl;
//
//
//        cout << "t1 last child? " << t1->getLastChild()->getUserObject() << endl;
//        cout << "t1 last leaf? " << t1->getLastLeaf()->getUserObject() << endl;
//        cout << "t2 last leaf? "  << t2->getLastLeaf()->getUserObject() << endl;
//
//        cout << "t2 get index of child(t4) = " << t2->getIndex(t4) << endl;
//        cout << "t1 get index of child(t4) = " << t1->getIndex(t4) << endl;
//
//
//        cout << "Iterator over the tree depth-first" << endl;
//
//        auto iter = t1->begin();
//        auto endIter = t1->end();
//
//        for (; iter != endIter;) {
//            // User needs to check if ++iter is the END before using it!
//            // This will skip the first node!
//            auto nextIt = ++iter;
//            if (nextIt == endIter) {
//                cout << "pre-increment hit iterator end" << endl;
//                break;
//            }
//            cout << "pre-increment my id is " << (*nextIt)->getUserObject() << endl;
//        }
//
//        cout << "Iterator over the tree breadth-first" << endl;
//
//        auto biter = t1->bbegin();
//        auto bendIter = t1->bend();
//
//        for (; biter != bendIter; ++biter) {
//            cout << "pre-increment my id is " << (*biter)->getUserObject() << endl;
//        }
//
//
//        // Now change the tree structure
//        cout << "Now make t5 child of t1 ......." << endl;
//        t1->insert(t5, 0);
//
//        for (iter = t1->begin(); iter != endIter; iter++) {
//            cout << "my id is " << (*iter)->getUserObject() << endl;
//        }
//
//        cout << "t1 has # children = " << t1->getChildCount() << endl;
//
//        cout << "t5 next sibling = " << t5->getNextSibling()->getUserObject() << endl;
//        cout << "t4 prev sibling = " << t4->getPreviousSibling()->getUserObject() << endl;
//
//        cout << "t5 get child at 0 = " << t5->getChildAt(0)->getUserObject() << endl;
//        cout << "t5 # kids = " << t5->getChildCount() << endl;
//
//
//        // Now change the tree structure
//        cout << "Now remove 0th child of t5 ......." << endl;
//        t5->remove(0);
//        cout << "t5 get child at 0 = " << t5->getChildAt(0)->getUserObject() << endl;
//        cout << "t5 # kids = " << t5->getChildCount() << endl;
//
//        // Now change the tree structure
//        cout << "Now remove t8 child of t5 ......." << endl;
//        t5->remove(t8);
//        cout << "t5 get child at 0 = " << t5->getChildAt(0)->getUserObject() << endl;
//        cout << "t5 get child at 1 = " << t5->getChildAt(1)->getUserObject() << endl;
//        cout << "t5 # kids = " << t5->getChildCount() << endl;
//
//
//
//        cout << "Now remove all children of t5 ......." << endl;
//        t5->removeAllChildren();
//        cout << "t5 # kids = " << t5->getChildCount() << endl;
//
//        cout << "Now remove t5 from parent ......." << endl;
//        t5->removeFromParent();
//        cout << "t1 # kids = " << t1->getChildCount() << endl;
//        cout << "t1 get child at 0 = " << t1->getChildAt(0)->getUserObject() << endl;
//
//
//        for (iter = t1->begin(); iter != endIter; iter++) {
//            cout << "my id is " << (*iter)->getUserObject() << endl;
//        }
//
//
//    }
//
//
//    cout << endl << "Past the scope" << endl;
//
////cout << "Start" << endl;
////
//// //   auto nodeD1 = evio::TreeNodeLiteDerived();
////    auto sptr1 = evio::TreeNodeLite::getInstance();
////    auto sptr2 = evio::TreeNodeLite::getInstance();
////    cout << "0" << endl;
////    int i = sptr1->getInt();
////    cout << "Got int = " << i << " from TNLite" << endl;
////
////    sptr1->insert(sptr2, 0);
////
////    cout << "Added child in TreeNodeLite" << endl;
////
////    auto sptrD1 = evio::TreeNodeLiteDerived::getInstance();
////    auto sptrD2 = evio::TreeNodeLiteDerived::getInstance();
////
////    cout << "2" << endl;
////
////    i = sptrD1->getInt();
////
////    cout << "Got int = " << i << " from TNLDervied" << endl;
////
////    sptrD1->insert(sptrD2, 0);
////
////    cout << "Added child in TreeNodeLiteDerived" << endl;
////
////    std::shared_ptr<evio::TreeNodeLite> child = sptrD1->getChildAt(0);
////    i = child->getInt();
////    cout << "Got int = " << i << " from child" << endl;
////    cout << "End" << endl;
////
////
//
//    return 0;
//}

//int mainAA(int argc, char **argv) {
//    auto b1 = std::make_shared<int>(1);
//    auto b2 = std::make_shared<int>(2);
//    auto b3 = std::make_shared<int>(2);
//    auto b4 = std::make_shared<int>(2);
//    auto b5 = std::make_shared<int>(2);
//    auto b6 = std::make_shared<int>(2);
//    auto b7 = std::make_shared<int>(2);
//    auto b8 = std::make_shared<int>(2);
//
//    std::shared_ptr<evio::TreeNode<int>> n1 = evio::TreeNode<int>::getInstance(b1);
//    auto n2 = evio::TreeNode<int>::getInstance(b2);
//    auto n3 = evio::TreeNode<int>::getInstance(b3);
//    auto n4 = evio::TreeNode<int>::getInstance(b4);
//    auto n5 = evio::TreeNode<int>::getInstance(b5);
//    auto n6 = evio::TreeNode<int>::getInstance(b6);
//    auto n7 = evio::TreeNode<int>::getInstance(b7);
//    auto n8 = evio::TreeNode<int>::getInstance(b8);
//
//    n1->insert(n2,0);
//    n2->add(n3);
//    n2->add(n4);
//    n2->add(n5);
//    n1->add(n6);
//    n6->add(n7);
//    n7->add(n8);
//
//    cout << "Root # children = " << n1->getChildCount() << endl;
//    cout << "Root # leaves = " << n1->getLeafCount() << endl;
//
//    n2->begin();
//    n2->end();
//
//    n3->bbegin();
//    n3->bend();
//
//    return 0;
//}


int mainA(int argc, char **argv) {
//    try {
        string filename   = "/dev/shm/EventWriterTest.evio";
        cout << endl << "Try writing " << filename << endl;

        // Write files
        string actualFilename = evio::ReadWriteTest::eventWriteFileMT(filename);
        cout << endl << "Finished writing, now try reading " << actualFilename << endl;

        // Read files just written
        evio::ReadWriteTest::readFile2(actualFilename);
        cout << endl << endl << "----------------------------------------" << endl << endl;
//    }
//    catch (std::exception & e) {
//        cout << e.what() << endl;
//    }

    return 0;
}


// For testing new ByteBuffer code
int main0(int argc, char **argv) {
    evio::ByteBuffer b(5);
    b[0] = 10; b[1] = 11; b[2] = 12; b[3] = 13; b[4] = 14;

    evio::Util::printBytes(b, 0, 5, "Byte subscript operator trial, b");

    evio::ByteBuffer d(5);
    d[0] = b[0]; d[1] = b[1]; d[2] = b[2]; d[3] = b[3]; d[4] = b[4];
    evio::Util::printBytes(d, 0, 5, "Byte subscript operator trial, d");

    const evio::ByteBuffer q(d);
    evio::Util::printBytes(q, 0, 5, "Byte subscript operator trial, q");

    cout << "access b[0] = " << ((int)b[0]) << ", q[2] = " << ((int)q[2]) << endl;

    return 0;
}



int main1(int argc, char **argv) {
    string filename   = "/dev/shm/hipoTest.evio";
    string filenameMT = "/dev/shm/hipoTestMT.evio";
//    string filename   = "/dev/shm/hipoTest-j.evio";
//    string filenameMT = "/dev/shm/hipoTestMT-j.evio";

    // Write files
    evio::ReadWriteTest::writeFile(filename);
    evio::ReadWriteTest::writeFileMT(filenameMT);

    // Read files just written
    evio::ReadWriteTest::readFile(filename);
    cout << endl << endl << "----------------------------------------" << endl << endl;
    evio::ReadWriteTest::readFile(filenameMT);

    return 0;
}


//static void expandEnvironmentalVariables(string & text) {
//    static std::regex env("\\$\\(([^)]+)\\)");
//    std::smatch match;
//    while ( std::regex_search(text, match, env) ) {
//        const char * s = getenv(match[1].str().c_str());
//        const std::string var(s == nullptr ? "" : s);
//        text.replace(match[0].first, match[0].second, var);
//    }
//}
//
//static uint32_t countAndFixIntSpecifiers1(string & text) {
//    static std::regex specifier("%(\\d*)([xd])");
//    uint32_t specifierCount = 0;
//    std::smatch match;
//
//    while ( std::regex_search(text, match, specifier) ) {
//        specifierCount++;
//        cout << "spec count = " << specifierCount << endl;
//        // Make sure any number preceding "x" or "d" starts with a 0 or else
//        // there will be empty spaces in the resulting string (i.e. file name).
//        std::string specWidth = match[1].str();
//        if (specWidth.length() > 0 && specWidth[0] != '0') {
//            cout << "in fix it loop" << endl;
//            text.replace(match[1].first, match[1].second, "0" + specWidth);
//        }
//    }
//
//    return specifierCount;
//}
//
//static uint32_t countAndFixIntSpecifiers(string text, string & returnString) {
//    static std::regex specifier("%(\\d*)([xd])");
//
//    auto begin = std::sregex_iterator(text.begin(), text.end(), specifier);
//    auto end   = std::sregex_iterator();
//    uint32_t specifierCount = std::distance(begin, end);
//    std::cout << "countAndFixIntSpecifiers: Found " << specifierCount << " specs" << endl;
//
//    // Make sure any number preceding "x" or "d" starts with a 0 or else
//    // there will be empty spaces in the resulting string (i.e. file name).
//    // We have to fix, at most, specifierCount number of specs.
//
//    std::sregex_iterator i = begin;
//
//    // Go thru all specifiers in text, only once
//    for (int j = 0; j < specifierCount; j++) {
//        if (j > 0) {
//            // skip over specifiers previously dealt with (text can change each loop)
//            i = std::sregex_iterator(text.begin(), text.end(), specifier);
//            int k=j;
//            while (k-- > 0) i++;
//        }
//
//        std::smatch match = *i;
//        std::string specWidth = match[1].str();
//        if (specWidth.length() > 0 && specWidth[0] != '0') {
//            text.replace(match[1].first, match[1].second, "0" + specWidth);
//        }
//    }
//
//    returnString = text;
//
//    return specifierCount;
//}
//
//
//int main2(int argc, char **argv) {
//
//    // TEST experimental file stuff
//    string fileName = "/daqfs/home/timmer/coda/evio-6.0/README";
//    cout << "orig file name = " << fileName << endl;
//    fs::path currentFilePath(fileName);
//    //currentFile = currentFilePath.toFile();
//    fs::path filePath = currentFilePath.filename();
//    string file_name = filePath.generic_string();
//    cout << "file name from path = " << file_name << endl;
//    cout << "dir  name from path = " << currentFilePath.parent_path() << endl;
//
//    bool fileExists = fs::exists(filePath);
//    bool isRegFile = fs::is_regular_file(filePath);
//
//    cout << "file is really there? = " << fileExists << endl;
//    cout << "file is regular file? = " << isRegFile << endl;
//
//    cout << "file " << currentFilePath.parent_path() << " is dir ? " <<
//                       fs::is_directory(currentFilePath.parent_path()) << endl;
//
//    fs::space_info dirInfo = fs::space(currentFilePath);
//
//    cout << "free space for dir in bytes is " << dirInfo.free << endl;
//    cout << "available space for dir in bytes is " << dirInfo.available << endl;
//    cout << "capacity of file system in bytes is " << dirInfo.capacity << endl;
//
//    cout << "size of file in bytes = " << fs::file_size(filePath) << endl;
//
//    string s  = "myfilename.$(ENV_1).junk$(ENV_2).otherJunk";
//    cout << s << endl << endl;
//
//    regex regExp("\\$\\((.*?)\\)");
//
//
//    // reset string
//    s = "myfilename.$(ENV_1).junk$(ENV_2).otherJunk";
//    regex rep("\\$\\((.*?)\\)");
//    cout << regex_replace(s, rep, "Sub1") << endl;
//
//    s = "myfilename.$(ENV_1).junk$(ENV_2).otherJunk";
//    expandEnvironmentalVariables(s);
//
//    cout << "call function, final string = " << s << endl;
//
//    s = "myfilename.%3d.junk%7x.otherJunk--%0123d---%x--%d";
//    cout << "fix specs, orig string = " << s << endl;
//    string another = s;
//    string returnStr("blah blah");
//    countAndFixIntSpecifiers(another, returnStr);
//    cout << "fix specs, final string = " << returnStr << endl;
//    cout << "fix specs, arg string = " << another << endl;
//
//
//    //fileName = string.format(fileName, runNumber);
//    int runNumber = 123;
//    int splitNumber = 45;
//    int streamId = 6;
//
//    s = "myfilename--%3d--0x%x--%012d--%03d";
//    char *temp = new char[1000];
//    cout << "string = " << s << endl;
//
//    std::sprintf(temp, s.c_str(), runNumber, splitNumber, streamId, streamId, streamId);
//    cout << "sub runNumber temp, string = " << temp << endl;
//
//    string fileName(temp);
//    cout << "string version of temp = " << fileName << endl;
//
//    fileName += "." + std::to_string(streamId) +
//                "." + std::to_string(splitNumber);
//
//    cout << "tack onto string = " << fileName << endl;
//
//
//
//    string a("twoSpecs%03d-----%4d~~~~~");
//    cout << endl << "try remove 2nd os spec into = " << a << endl;
//
//    static std::regex specifier("(%\\d*[xd])");
//    auto begin = std::sregex_iterator(a.begin(), a.end(), specifier);
//
//    // Go to 2nd match
//    begin++;
//
//    std::smatch match = *begin;
//    a.replace(match[0].first, match[0].second, "");
//
////    static std::regex specifier("(%\\d*[xd])");
////    auto begin = std::sregex_iterator(a.begin(), a.end(), specifier);
////    //auto end   = std::sregex_iterator();
////    // uint32_t specifierCount = std::distance(begin, end);
////
////    // Go to 2nd match
////    begin++;
////
////    std::smatch match = *begin;
////    a.replace(match[0].first, match[0].second, "%d." + match.str());
//
//    cout << "final tacked string = " << a << endl;
//
//
//    return 0;
//}