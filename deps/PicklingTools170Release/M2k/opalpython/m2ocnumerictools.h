#ifndef M2_OC_NUMERIC_TOOLS_H_
#define M2_OC_NUMERIC_TOOLS_H_

// A few helper functions/defines for help for dealing with Numeric
// (Python Numeric)

#include "m2array.h"

// Convert from an OpalValue tag to a Python Numeric Tab
inline char M2TagToNumeric (Numeric_e tag)
{
  char c;
  switch (tag) {
  case M2_BYTE: c='1'; break;
  case M2_UBYTE: c='b'; break; 
  case M2_INTEGER: c='s'; break; 
  case M2_UINTEGER: c='w'; break; 
  case M2_LONG: c='i'; break; 
  case M2_ULONG: c='u'; break; 
  case M2_XLONG: c='l'; break; 
  case M2_UXLONG: c='l'; break; 
    //case M2_BOOL: c='b'; break; 
  case M2_FLOAT: c='f'; break; 
  case M2_DOUBLE: c='d'; break; 
  case M2_CX_FLOAT: c='F'; break; 
  case M2_CX_DOUBLE: c='D'; break; 
  default: throw MidasException("No corresponding Numeric type for Number type");
  }
  return c;
}

// Convert from Numeric tag to M2 Tag
inline Numeric_e NumericTagToM2 (char tag)
{
  Numeric_e c;
  switch (tag) {
  case '1': c=M2_BYTE; break;
  case 'b': c=M2_UBYTE; break; 
  case 's': c=M2_INTEGER; break; 
  case 'w': c=M2_UINTEGER; break; 
  case 'i': c=M2_LONG; break; 
  case 'u': c=M2_ULONG; break; 
  case 'l': c=M2_XLONG; break; 
    //case 'l': c=M2_UXLONG; break; 
    //case 'b': c='b'; break; 
  case 'f': c=M2_FLOAT; break; 
  case 'd': c=M2_DOUBLE; break; 
  case 'F': c=M2_CX_FLOAT; break; 
  case 'D': c=M2_CX_DOUBLE; break; 
  default: throw MidasException("No corresponding Numeric type for OpalValue type");
  }
  return c;
}


// Convert low-level bytes to another type: this assumes both input
// and output have the right number of elements
template <class IT, class OT>
inline void BufferConvert (const IT* in, OT* out, int len)
{
  for (int ii=0; ii<len; ii++) {
    out[ii] = OT(in[ii]);
  }
} 

// Convert low-level bytes to another type: this assumes both input
// and output have the right number of elements
template <class IT, class OT>
inline void BufferCXConvert (const IT* in, OT* out, int len)
{
  for (int ii=0; ii<len; ii++) {
    out[ii] = OT(in[ii].re, in[ii].im);
  }
} 

// helper function to convert from POD array to a different type of POD array
//#define OCCVTARRT(T) { result=Array<T>(len); Array<T>& c=result; c.expandTo(len); T* cdata=c.data(); BufferConvert(a,cdata,len); }
#define OCCVTARRT(T) { T*cdata=(T*)result.writeData(); BufferConvert(a,cdata,len); }
#define OCCVTCXARRT(T) { T*cdata=(T*)result.writeData(); BufferCXConvert(a,cdata,len); }

template <class T>
inline void ConvertArrayT_ (const T* a, const int len, Numeric_e to_type,
			    Vector& result, bool is_complex=false)
{
  switch (to_type) {
  case M2_BYTE: OCCVTARRT(int_1); break;
  case M2_UBYTE: OCCVTARRT(int_u1); break; 
  case M2_INTEGER: OCCVTARRT(int_2 ); break; 
  case M2_UINTEGER: OCCVTARRT(int_u2); break; 
  case M2_LONG: OCCVTARRT(int_4); break; 
  case M2_ULONG: OCCVTARRT(int_u4); break; 
    // Take care with these ... longs and int_8s not necessarily the same
  case M2_XLONG: OCCVTARRT(int_8); break; // TODo exception?
  case M2_UXLONG: OCCVTARRT(int_u8); break; 
    //case M2_BOOL: OCCVTARRT(bool); break; 
  case M2_FLOAT: OCCVTARRT(real_4); break; 
  case M2_DOUBLE: OCCVTARRT(real_8); break; 
  case M2_CX_FLOAT: OCCVTARRT(complex_8); break; 
  case M2_CX_DOUBLE: OCCVTARRT(complex_16); break; 
  default: throw MidasException("Array not a POD type");
  }
}
template <class T>
inline void ConvertCXArrayT_ (const T* a, const int len, Numeric_e to_type,
			      Vector& result, bool is_complex=false)
{
  switch (to_type) {
  case M2_CX_FLOAT: OCCVTCXARRT(complex_8); break;
  case M2_CX_DOUBLE: OCCVTCXARRT(complex_16); break;
  default: throw MidasException("Cannot convert array of complex to anything but another complex: Use mag2? mag? re? im?"); break;
  }
}

// Convert a pre-existing array from one type to the given type: this
// completely installs the new converted array into a.
//#define OCCONVERTARRAY(T, A, TAG) { Array<T>& aa=A; ConvertArrayT_<T>(aa.data(), aa.length(), TAG, result); }
#define OCCONVERTARRAY(T, A, TAG) { T*data=(T*)a.readData(); ConvertArrayT_<T>(data, a.elements(), TAG, result); }

//#define OCCONVERTCXARRAY(T, A, TAG) { Array<T>& aa=A; ConvertCXArrayT_<T>(aa.data(), aa.length(), TAG, result); }
#define OCCONVERTCXARRAY(T, A, TAG) { T*data=(T*)a.readData(); ConvertCXArrayT_<T>(data, a.elements(), TAG, result); }
inline void ConvertArray (Vector& a, Numeric_e to_type)
{
  Numeric_e this_tag = a.type();
  if (this_tag == to_type) return;  // Both same, no work

  Vector result(to_type, a.elements());  // uninitialized vector 

  switch (this_tag) {
  case M2_BYTE: OCCONVERTARRAY(int_1,  a, to_type); break;
  case M2_UBYTE: OCCONVERTARRAY(int_u1, a, to_type);; break; 
  case M2_INTEGER: OCCONVERTARRAY(int_2,  a, to_type);; break; 
  case M2_UINTEGER: OCCONVERTARRAY(int_u2, a, to_type);; break; 
  case M2_LONG: OCCONVERTARRAY(int_4,  a, to_type);; break; 
  case M2_ULONG: OCCONVERTARRAY(int_u4, a, to_type);; break; 
  case M2_XLONG: OCCONVERTARRAY(int_8,  a, to_type);; break; 
  case M2_UXLONG: OCCONVERTARRAY(int_u8, a, to_type);; break; 
    //case M2_BOOL: OCCONVERTARRAY(bool,   a, to_type);; break; 
  case M2_FLOAT: OCCONVERTARRAY(real_4, a, to_type);; break; 
  case M2_DOUBLE: OCCONVERTARRAY(real_8, a, to_type);; break; 
  case M2_CX_FLOAT: OCCONVERTCXARRAY(complex_8, a, to_type); break; 
  case M2_CX_DOUBLE: OCCONVERTCXARRAY(complex_16, a,to_type); break; 
  default: throw MidasException("Array not a POD type");
  }
  a=result; // Install new array
}



#endif //  M2_OC_NUMERIC_TOOLS_H_
