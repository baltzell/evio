//  evioBankIndex.hxx
//
// creates bank index for serialized event 
// eventually need to switch to std::tuple instead of custom struct
//
//
//  Author:  Elliott Wolin, JLab, 15-aug-2012



#ifndef _evioBankIndex_hxx
#define _evioBankIndex_hxx


#include <evioException.hxx>
#include <evioUtil.hxx>
//  #include <boost/tuple/tuple.hpp>


namespace evio {

using namespace std;
using namespace evio;


// holds bank index info
typedef struct {
  int contentType;
  const void *data;
  int length;
} bankIndex;
  

// tuple holds:  data type, pointer to bank data, length of data array
//typedef boost::tuple<int, const void*, int> bankIndex;


// compares tagNums to each other, first by tag, then by num
struct tagNumComp {
  bool operator() (const tagNum &lhs, const tagNum &rhs) const {
    if(lhs.first<rhs.first) {
      return(true);
    } else if(lhs.first>rhs.first) {
      return(false);
    } else {
      return(lhs.second<rhs.second);
    }    
  }
};


// bank index map and range
typedef multimap<tagNum,bankIndex,tagNumComp> bankIndexMap;
typedef pair< bankIndexMap::const_iterator, bankIndexMap::const_iterator > bankIndexRange;




//-----------------------------------------------------------------------
//-----------------------------------------------------------------------


/**
 * Creates bank index for serialized event.
 *
 * Note that a given tag/num may appear more than once in event and map.
 */
class evioBankIndex {

public:
  evioBankIndex();
  evioBankIndex(const uint32_t *buffer);
  virtual ~evioBankIndex();


public:
  bool parseBuffer(const uint32_t *buffer);
  bool tagNumExists(const tagNum& tn) const;
  int tagNumCount(const tagNum& tn) const;
  bankIndexRange getRange(const tagNum& tn) const;
  bankIndex getBankIndex(const tagNum &tn) const throw(evioException);


public:
  bankIndexMap tagNumMap;     /**<Holds index to one or more banks having tag/num.*/



public:
  /**
   * Returns length and pointer to data, NULL if bad tagNum or wrong type.
   *
   * @param tn tagNum
   * @param pLen Pointer to int to receive data length, set to 0 upon error
   * @return Pointer to data, NULL on error
   */
  template <typename T> const T* getData(const tagNum &tn, int *pLen) throw (evioException) {

    bankIndexMap::const_iterator iter = tagNumMap.find(tn);

    // if((iter!=tagNumMap.end()) && (boost::get<0>((*iter).second)==evioUtil<T>::evioContentType())) {
    //   *pLen=boost::get<2>((*iter).second);
    //   return(static_cast<const T*>(boost::get<1>((*iter).second)));

    if((iter!=tagNumMap.end()) && ((((*iter).second).contentType)==evioUtil<T>::evioContentType())) {
      *pLen=((*iter).second).length;
      return(static_cast<const T*>(((*iter).second).data));
    } else {
      *pLen=0;
      return(NULL);
    }
  }


  /**
   * Returns length and pointer to data, assumes valid bankIndex
   *
   * @param bi bankIndex
   * @param pLen Pointer to int to receive data length, set to 0 for bad type
   * @return Pointer to data, NULL on bad type
   */
  template <typename T> const T* getData(const bankIndex &bi, int *pLen) throw (evioException) {
    // if(boost::get<0>(bi)==evioUtil<T>::evioContentType()) {
    //   *pLen=boost::get<2>(bi);
    //   return(static_cast<const T*>(boost::get<1>(bi)));

    if(bi.contentType==evioUtil<T>::evioContentType()) {
      *pLen=(bi.length);
      return(static_cast<const T*>(bi.data));
    } else {
      *pLen=0;
      return(NULL);
    }
  }

};

//-----------------------------------------------------------------------
//-----------------------------------------------------------------------

} // namespace evio


#endif
  
