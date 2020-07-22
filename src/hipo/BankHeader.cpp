//
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.
//
// EPSCI Group
// Thomas Jefferson National Accelerator Facility
// 12000, Jefferson Ave, Newport News, VA 23606
// (757)-269-7100


#include "BankHeader.h"


namespace evio {


    /**
     * Constructor.
     * @param tag the tag for the bank header.
     * @param dataType the data type for the content of the bank.
     * @param num sometimes, but not necessarily, an ordinal enumeration.
     */
    BankHeader::BankHeader(uint16_t tag, DataType const & dataType, uint8_t num) :
                    BaseStructureHeader(tag, dataType, num) {length = 1;}


    /**
      * Get the length of the structure's header in ints. This includes the first header word itself
      * (which contains the length) and in the case of banks, it also includes the second header word.
      * @return Get the length of the structure's header in ints.
      */
    uint32_t BankHeader::getHeaderLength() {return 2;}


    /**
     * Write myself out as evio format data
     * into the given byte array in the specified byte order.
     *
	 * @param dest array into which evio data is written (destination).
	 * @param order  byte order in which to write the data.
     * @return the number of bytes written, which for a BankHeader is 8.
     * @throws EvioException if destination array too small to hold data.
     */
    size_t BankHeader::write(uint8_t *dest, ByteOrder const & order) {
        // length first
        Util::toBytes(length, order, dest);

        if (order == ByteOrder::ENDIAN_BIG) {
            Util::toBytes(tag, order, dest+4);
            // lowest 6 bits are dataType, upper 2 bits are padding
            dest[6] = (uint8_t)((dataType.getValue() & 0x3f) | (padding << 6));
            dest[7] = number;
        }
        else {
            dest[4] = number;
            dest[5] = (uint8_t)((dataType.getValue() & 0x3f) | (padding << 6));
            Util::toBytes(tag, order, dest+6);
        }

        return 8;
    }


    /**
     * Write myself out a byte buffer.
     * This write is relative - i.e., it uses the current position of the buffer.
     *
     * @param byteBuffer the byteBuffer to write to.
     * @return the number of bytes written, which for a BankHeader is 8.
     */
    size_t BanktHeader::write(std::shared_ptr<ByteBuffer> & byteBuffer) {
        return write(*(byteBuffer.get()));
    }


    /**
     * Write myself out a byte buffer.
     * This write is relative - i.e., it uses the current position of the buffer.
     *
     * @param dest the byteBuffer to write to.
     * @return the number of bytes written, which for a BankHeader is 8.
     */
    size_t BankHeader::write(ByteBuffer & dest) {
        dest.putInt(length);

        if (dest.order() == ByteOrder::ENDIAN_BIG) {
            dest.putShort(tag);
            dest.put((int8_t)((dataType.getValue() & 0x3f) | (padding << 6)));
            dest.put(number);
        }
        else {
            dest.put(number);
            dest.put((int8_t)((dataType.getValue() & 0x3f) | (padding << 6)));
            dest.putShort(tag);
        }

        return 8;
    }


    /**
     * Obtain a string representation of the bank header.
     * @return a string representation of the bank header.
     */
    string BankHeader::toString() {
        stringstream ss;

        ss << "bank length: " << length << endl;
        ss << "     number: " << number << endl;
        ss << "  data type: " << getDataTypeName() << endl;
        ss << "        tag: " << tag << endl;
        ss << "    padding: " << padding << endl;

        return ss.str();
    }

}
