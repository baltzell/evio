# To use this, do the following the first time:
#   % mkdir build
#   % mkdir build/cmake
#   % cd build/cmake
#   % cmake ../..
#   % make (install)
#
#
# After doing this once can do the following to recompile
#  % cd <top level evio dir>
#  % cmake --build build/cmake (--target install)
#
#
# Set the path in which to install.
# This can be overridden on command line (in build/cmake dir) with:
#   % cmake -DCMAKE_INSTALL_PREFIX=<my_dir> ../..
#   % make install
# (This call must be placed BEFORE "project" command).
set(CMAKE_INSTALL_PREFIX ./)

cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(evio VERSION 6.0)

# Place libs & binaries in build/cmake/lib and bin
#set(CMAKE_INSTALL_PREFIX   /Users/timmer/coda/codaIn1)
set(LIBRARY_OUTPUT_PATH    ${CMAKE_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

message(STATUS "CMAKE_BINARY_DIR = " ${CMAKE_BINARY_DIR})
message(STATUS "CMAKE_INSTALL_PREFIX = " ${CMAKE_INSTALL_PREFIX})
message(STATUS "CMAKE_INSTALL_LIBDIR = " ${CMAKE_INSTALL_LIBDIR})
message(STATUS "CMAKE_INSTALL_INCLUDEDIR = " ${CMAKE_INSTALL_INCLUDEDIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

set(C_HEADER_FILES
        src/libCsrc/evio.h
        src/libCsrc/msinttypes.h
        )

set(C_LIB_FILES
        src/libCsrc/evio.c
        src/libCsrc/evioswap.c
        src/libCsrc/eviofmt.c
        src/libCsrc/eviofmtdump.c
        src/libCsrc/eviofmtswap.c
        src/libCsrc/eviocopy.c
        )

set(CPP_HEADER_FILES_NEW
        src/libsrc/FileEventIndex.h
        src/libsrc/EvioException.h
        src/libsrc/ByteOrder.h
        src/libsrc/ByteBuffer.h
        src/libsrc/HeaderType.h
        src/libsrc/Compressor.h
        src/libsrc/FileHeader.h
        src/libsrc/RecordHeader.h
        src/libsrc/RecordInput.h
        src/libsrc/RecordOutput.h
        src/libsrc/Writer.h
        src/libsrc/WriterMT.h
        src/libsrc/EvioNode.h
        src/libsrc/DataType.h
        src/libsrc/StructureType.h
        src/libsrc/RecordNode.h
        src/libsrc/Reader.h
        src/libsrc/RecordSupply.h
        src/libsrc/RecordRingItem.h
        src/libsrc/Util.h
        src/libsrc/EventWriter.h
        src/libsrc/RecordCompressor.h
        src/libsrc/BaseStructure.h
        src/libsrc/BaseStructureHeader.h
        src/libsrc/CompositeData.h
        src/libsrc/BankHeader.h
        src/libsrc/SegmentHeader.h
        src/libsrc/TagSegmentHeader.h
        src/libsrc/EvioBank.h
        src/libsrc/EvioSegment.h
        src/libsrc/EvioTagSegment.h
        src/libsrc/EvioEvent.h
        src/libsrc/EventParser.h
        src/libsrc/IEvioFilter.h
        src/libsrc/IEvioListener.h
        src/libsrc/EventParser.h
        src/libsrc/EventHeaderParser.h
        src/libsrc/StructureTransformer.h
        src/libsrc/IBlockHeader.h
        src/libsrc/IEvioReader.h
        src/libsrc/EvioReader.h
        src/libsrc/EvioReaderV4.h
        src/libsrc/BlockHeaderV2.h
        src/libsrc/BlockHeaderV4.h
        src/libsrc/EvioReaderV6.h
        src/libsrc/EvioCompactReader.h
        src/libsrc/IEvioCompactReader.h
        src/libsrc/EvioXMLDictionary.h
        src/libsrc/EvioDictionaryEntry.h
        src/libsrc/pugixml.hpp
        src/libsrc/pugiconfig.hpp
        src/libsrc/EvioCompactReaderV4.h
        src/libsrc/EvioCompactReaderV6.h
        src/libsrc/EventBuilder.h
        src/libsrc/CompactEventBuilder.h
        src/libsrc/EvioSwap.h
        src/libsrc/StructureFinder.h
        src/libsrc/eviocc.h
        )

set(CPP_LIB_FILES_NEW
        src/libsrc/FileEventIndex.cpp
        src/libsrc/ByteOrder.cpp
        src/libsrc/ByteBuffer.cpp
        src/libsrc/HeaderType.cpp
        src/libsrc/Compressor.cpp
        src/libsrc/FileHeader.cpp
        src/libsrc/RecordHeader.cpp
        src/libsrc/RecordInput.cpp
        src/libsrc/RecordOutput.cpp
        src/libsrc/Writer.cpp
        src/libsrc/WriterMT.cpp
        src/libsrc/EvioNode.cpp
        src/libsrc/DataType.cpp
        src/libsrc/StructureType.cpp
        src/libsrc/Reader.cpp
        src/libsrc/RecordSupply.cpp
        src/libsrc/RecordRingItem.cpp
        src/libsrc/EventWriter.cpp
        src/libsrc/BaseStructure.cpp
        src/libsrc/BaseStructureHeader.cpp
        src/libsrc/CompositeData.cpp
        src/libsrc/BankHeader.cpp
        src/libsrc/SegmentHeader.cpp
        src/libsrc/TagSegmentHeader.cpp
        src/libsrc/EventParser.cpp
        src/libsrc/EvioReader.cpp
        src/libsrc/EvioReaderV4.cpp
        src/libsrc/EvioReaderV6.cpp
        src/libsrc/EvioCompactReader.cpp
        src/libsrc/EvioXMLDictionary.cpp
        src/libsrc/pugixml.cpp
        src/libsrc/EvioCompactReaderV4.cpp
        src/libsrc/EvioCompactReaderV6.cpp
        src/libsrc/EventBuilder.cpp
        src/libsrc/CompactEventBuilder.cpp)


set(TEST
        src/test/CompactBuilder_Test.cpp
        src/test/Dict_FirstEv_Test.cpp
        src/test/Hipo_Test.cpp
        src/test/ReadWriteTest.cpp
        src/test/RecordSupplyTest.cpp
        src/test/RingBufferTest.cpp
        src/test/Tree_Buf_Composite_Builder_Test.cpp
        src/test/Writer_Reader_Compression_Test.cpp
        src/test/Writer_Reader_Test.cpp
        )


set(TESTC
        src/testC/evReadPipe.c
        src/testC/evtest.c
        src/testC/evtest2.c
        src/testC/evtest3.c
        src/testC/evtestAppend.c
        src/testC/evtestBuf.c
        src/testC/evtestBuf2.c
        src/testC/evTestFile.c
        src/testC/evtestWriteFile.c
        src/testC/evtestRead.c
        src/testC/evtestRio.c
        src/testC/evtestSock.c
        src/testC/evWritePipe.c
        src/testC/splitTest.c
        )


include(FindPackageHandleStandardArgs)

# search for boost libs
find_package(Boost REQUIRED system thread chrono)

if (Boost_FOUND)
    message(STATUS "Boost Found: libs = " ${Boost_LIBRARIES} ", include dirs = " ${Boost_INCLUDE_DIRS})
    link_directories(${Boost_LIBRARY_DIRS})
elseif(NOT Boost_FOUND)
    error("Boost Not Found")
endif()

# search for lz4 libs
find_package_handle_standard_args( LZ4 REQUIRED_VARS LZ4_LIBRARY LZ4_INCLUDE_DIR HANDLE_COMPONENTS)

if( LZ4_FOUND )
  message(STATUS "LZ4 Found")
  message(STATUS "LZ4 include directory = ${LZ4_INCLUDE_DIR}")
  message(STATUS "LZ4 library = ${LZ4_LIBRARY}")
  set( CMAKE_REQUIRED_INCLUDES ${LZ4_INCLUDE_DIRS} )
  set( CMAKE_REQUIRED_LIBRARIES ${LZ4_LIBRARY})
else()
    message("LZ4 NOT Found, try again ...")

    # On Linux lz4 cannot be found for some reason.
    # This is where it is on my system.
    find_path(LZ4_INCLUDE_DIR
            NAMES lz4.h
            PATHS /usr/include
            )

    find_library(LZ4_LIBRARY
            NAMES lz4
            PATHS /usr/lib64
            )

    if(NOT LZ4_INCLUDE_DIR)
        message(FATAL_ERROR "lz4.h cannot be found, cmake will exit." )
    endif()

    if (NOT LZ4_LIBRARY)
        message(FATAL_ERROR "lz4 library cannot be found, cmake will exit." )
    endif()

    message("Found LZ4 include directory = ${LZ4_INCLUDE_DIR}")
    message("Found LZ4 library = ${LZ4_LIBRARY}")
endif()

# Remove from cache so new search done each time
unset(DISRUPTOR_INCLUDE_DIR CACHE)
unset(DISRUPTOR_LIBRARY CACHE)

# Search for disruptor includes & lib
find_path(DISRUPTOR_INCLUDE_DIR
          NAMES Disruptor.h
          PATHS $ENV{DISRUPTOR_CPP_HOME}/Disruptor
          NO_DEFAULT_PATH
         )

find_library(DISRUPTOR_LIBRARY
             NAMES Disruptor
             PATHS $ENV{DISRUPTOR_CPP_HOME}/build/Disruptor
             NO_DEFAULT_PATH
            )


if( DISRUPTOR_INCLUDE_DIR )
    message(STATUS "Disruptor.h found in path = " $ENV{DISRUPTOR_CPP_HOME}/Disruptor)
    # Reset this to one directory upstream since these files are included as
    # include "Disruptor/xxx.h
    set (DISRUPTOR_INCLUDE_DIR  $ENV{DISRUPTOR_CPP_HOME})
    message(STATUS "Disruptor include directory now set to " ${DISRUPTOR_INCLUDE_DIR})
else()
    message(FATAL_ERROR "Disruptor.h NOT found, cmake will exit." )
endif()


if( DISRUPTOR_LIBRARY )
    message(STATUS "Disruptor library found at = " ${DISRUPTOR_LIBRARY})
else()
    message(FATAL_ERROR "Disruptor library NOT found, cmake will exit." )
endif()


find_package(Doxygen)

if( DOXYGEN_FOUND )
    message(STATUS "Doxygen found")

    # Change these values from cmake defaults in the Doxyfile
    set(DOXYGEN_GENERATE_TREEVIEW YES)
    set(DOXYGEN_REFERENCED_BY_RELATION YES)
    set(DOXYGEN_EXCLUDE_PATTERNS */.* */CMakeFiles/* */_CPack_Packages/* DartConfiguration.tcl CMakeLists.txt CMakeCache.txt)
    set(DOXYGEN_EXCLUDE src/libsrc/pugixml.cpp src/libsrc/pugixml.hpp)
    set(DOXYGEN_FILE_PATTERNS *.cpp *.h)
    #set(DOXYGEN_WARN_LOGFILE doxyerrors.log)
    set(DOXYGEN_SORT_MEMBERS_CTORS_1ST YES)
    set(DOXYGEN_SHOW_GROUPED_MEMB_INC YES)
    set(DOXYGEN_INLINE_IHERITED_MEMB YES)
    set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
    set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/doc/doxygen/CC)

    doxygen_add_docs(docs src/libsrc )
else()
    message(FATAL_ERROR "DOxygen NOT found, cmake will exit." )
endif()


# Find include files here
#include_directories(src/libsrc src/libCsrc /usr/local/include ${Boost_INCLUDE_DIRS} ${LZ4_INCLUDE_DIR} $ENV{DISRUPTOR_CPP_HOME}/Disruptor)

# Shared evio C library
add_library(evio SHARED ${C_LIB_FILES})
include_directories(evio PUBLIC src/libCsrc /usr/local/include)


# Shared evio C++ library
add_library(eviocc SHARED ${CPP_LIB_FILES_NEW})
target_link_libraries(eviocc ${LZ4_LIBRARY} ${Boost_LIBRARIES} ${DISRUPTOR_LIBRARY})
include_directories(eviocc PUBLIC src/libsrc /usr/local/include
                    ${Boost_INCLUDE_DIRS} ${LZ4_INCLUDE_DIRS} ${DISRUPTOR_INCLUDE_DIR})


# Main Executables
add_executable(ReadWriteTest src/test/ReadWriteTest.cpp)
target_link_libraries(ReadWriteTest pthread ${Boost_LIBRARIES} expat dl z m ${LZ4_LIBRARY} eviocc)


add_executable(RingBufferTest src/test/RingBufferTest.cpp)
target_link_libraries(RingBufferTest pthread ${Boost_LIBRARIES}  expat dl z m ${LZ4_LIBRARY} eviocc)


# Test programs
foreach(fileName ${TESTC})
    # Get file name with no directory or extension as executable name
    get_filename_component(execName ${fileName} NAME_WE)
    # Create executable from file
    add_executable(${execName} ${fileName})
    # Needs these libs
    target_link_libraries(${execName} evio)
    # Install into bin/testC dir
    install(TARGETS ${execName} RUNTIME DESTINATION testC)
endforeach(fileName)


# Installation defaulting to ${CMAKE_INSTALL_PREFIX}/lib
install(TARGETS evio LIBRARY)
install(TARGETS eviocc ARCHIVE)
# Installation defaulting to ${CMAKE_INSTALL_PREFIX}/include
install(DIRECTORY src/libsrc/ DESTINATION include FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
# This works too
#install(FILES ${CPP_HEADER_FILES_NEW} DESTINATION include)
# This works too
# foreach(headerFile ${CPP_HEADER_FILES_NEW})
#     install(FILES ${headerFile} PUBLIC_HEADER DESTINATION include)
# endforeach(headerFile)

