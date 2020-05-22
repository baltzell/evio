//
// Copyright (c) 2020, Jefferson Science Associates
//
// Thomas Jefferson National Accelerator Facility
// EPSCI Group
//
// 12000, Jefferson Ave, Newport News, VA 23606
// Phone : (757)-269-7100
//

#ifndef EVIO_6_0_TAGSEGMENTHEADER_H
#define EVIO_6_0_TAGSEGMENTHEADER_H

#include "Util.h"
#include "DataType.h"
#include "ByteOrder.h"
#include "ByteBuffer.h"
#include "BaseStructureHeader.h"

namespace evio {

    /**
     * This the header for an evio tagsegment structure (<code>EvioTagSegment</code>).
     * It does not contain the raw data, just the header.
     * Copied from the java class of identical name.
     *
     * @author heddle (original Java version)
     * @author timmer
     * @date 4/27/2020
     */
    class TagSegmentHeader : public BaseStructureHeader {

    public:

        TagSegmentHeader() = default;
        TagSegmentHeader(uint16_t tag, DataType const & dataType);
        TagSegmentHeader(uint16_t tag, string const & format);

        uint32_t getHeaderLength() override;
        string   toString() override;

        size_t write(ByteBuffer & dest) override;
        size_t write(uint8_t *dest, ByteOrder const & order) override;
    };

}


#endif //EVIO_6_0_TAGSEGMENTHEADER_H
