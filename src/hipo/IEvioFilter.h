//
// Created by Carl Timmer on 2020-05-27.
//

#ifndef EVIO_6_0_IEVIOFILTER_H
#define EVIO_6_0_IEVIOFILTER_H



#include <memory>

#include "StructureType.h"
#include "BaseStructure.h"

namespace evio {

    /**
     * This interface allows applications to create filters so that they only receive certain structures
     * when events are being processed. Below is a filter that accepts any structure that has tag = 400.
     * <pre>
     * class MyFilter : IEvioFilter {
     *     public bool accept(StructureType & structureType, std::shared_ptr<BaseStructure> structure) {
     *         return (structure->getHeader()->getTag() == 400);
     *     }
     * };
     * MyFilter myfilter();
     * EventParser.getInstance().setEvioFilter(myFilter);
     * </pre>
     * @author heddle (Original java class)
     * @author timmer
     *
     */
    class IEvioFilter {

    public:
        /**
         * Accept or reject the given structure.
         *
         * @param structureType the structure type, a <code>StructureType</code> enum, of the structure
         *                      that was just found, e.g., <code>StructureType.BANK</code>.
         * @param structure the structure that was just found. From its header the tag, num, length,
         *                  and data type are available. The application can filter based on those
         *                  quantities or on the data itself.
         * @return <code>true</code> if the structure passes the filter and should be given to the listeners.
         * @see StructureType
         */
        virtual bool accept(StructureType structureType, std::shared_ptr<BaseStructure> structure) = 0;
    };

}


#endif //EVIO_6_0_IEVIOFILTER_H