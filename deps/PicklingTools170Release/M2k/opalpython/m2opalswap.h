#ifndef M2OPALVALUESWAP_H_
#define M2OPALVALUESWAP_H_

#include "m2opalvalue.h"

// Code to allow us to swap OpalValues: necessary
// for constant time speed in a few situations

void CacheSwap (char lhs[M2_OPALVALUE_MAX_LENGTH], 
		char rhs[M2_OPALVALUE_MAX_LENGTH])
{
  for (int ii=0; ii<M2_OPALVALUE_MAX_LENGTH; ii++) {
    char temp = lhs[ii];
    lhs[ii] = rhs[ii];
    rhs[ii] = temp;
  }
}

void CacheMove (char dest[M2_OPALVALUE_MAX_LENGTH], 
		const char src[M2_OPALVALUE_MAX_LENGTH])
{
  for (int ii=0; ii<M2_OPALVALUE_MAX_LENGTH; ii++) {
    dest[ii] = src[ii];
  }
}

// Generic swapping function
template <class T>
void swap (T& lhs, T& rhs)
{
  T temp(lhs);
  lhs = rhs;
  rhs = temp;
}




// Sole purpose of this structure is to allow swapping of OpalValues
// in constant time.  Same layout as an OpalValue, although a bit of a hack:
// depends on OpalValue not changing

struct OpalValue_t {

  inline void swap (OpalValue& rhs) 
  {
    OpalValue_t *r = (OpalValue_t*)&rhs;
    swap(this, r);
  }

  static inline void swap (OpalValue_t* lhs, OpalValue_t* rhs)
  {
    bool lhs_cached = 
      (lhs->value_ == (OpalValueA*) &lhs->data_.buffer); // lhs cached
    bool rhs_cached = 
      (rhs->value_ == (OpalValueA*) &rhs->data_.buffer); // rhs cached

    if (lhs_cached) {
      if (rhs_cached) {
	CacheSwap(lhs->data_.buffer, rhs->data_.buffer); 
      } else { // lhs cached BUT rhs NOT cached
	CacheMove(rhs->data_.buffer, lhs->data_.buffer); 
	lhs->value_ = rhs->value_;
	rhs->value_ = (OpalValueA*)&rhs->data_.buffer[0];
      }
    } 
    else { // lhs NOT cached
      if (rhs_cached) {
	// lhs NOT cached, rhs cached
	CacheMove(lhs->data_.buffer, rhs->data_.buffer);
	rhs->value_ = lhs->value_;
	lhs->value_ = (OpalValueA*)&lhs->data_.buffer[0];
      } else {
	// neither cached, swap pointers
	::swap(lhs->value_, rhs->value_);
      }
    }
  }

  // ///// Data members
  union {
    most_restrictive_alignment_t forcealignment_;
    char buffer[M2_OPALVALUE_MAX_LENGTH];
  } data_;


  // This usually points to data_ below.  OpalValue is a "union"
  // type container, where it has enough space to hold any of the
  // canonical OpalValueTs that are constructed.  If value_ points
  // into data below, we know the value was constructed in place.
  // If it wasn't constructed in place, then it was off the heap and
  // the destructor will need to delete it from the heap.
  OpalValueA* value_;

}; // OpalValue_t


inline void Swap (OpalValue& lhs, OpalValue& rhs)
{
  OpalValue_t* l = (OpalValue_t*)&lhs;
  l->swap(rhs);
}

#endif //  M2OPALVALUESWAP_H_
