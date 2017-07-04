
#include "m2opalpython.h"
#include "m2pythontools.h"

// For unpickling

// Turn a python type code into a contiguous memory piece and convert
// to appropriate byte order
OpalValue MakeVector (const string& typecode, size_t elements,
		       const char* mem, size_t bytes)
{
  // Error checking
  if (typecode.length()>1 || typecode.length()==0) {
    MakeException("Only expect 1 letter types for Depickling Numeric "
		  "arrays.  Saw:"+typecode);
  }
  
  // Lookup the m2k typecode from the python typecode
  char python_typecode[]   = "dlFD";
  char* where = strchr(python_typecode, typecode[0]);
  if (!where) {
    MakeException("Unknown depickling typecode"+typecode);
  }
  Numeric_e m2k_typecode[] = { DOUBLE, LONG, CX_FLOAT, CX_DOUBLE };
  Numeric_e m2k_type = m2k_typecode[where-python_typecode];
  
  // Create an m2k vector of the proper type and length: and populate.
  Vector final_result(m2k_type, elements);
  char* result_buff = (char*)final_result.writeData();
  size_t result_bytes  = elements*ByteLength(m2k_type);
  
  CopyPrintableBufferToVector(mem, bytes, result_buff, result_bytes);
  
  // If this machine is big-endian, we need to normalized to little endian
  if (!IsLittleEndian()) {
    InPlaceReEndianize(result_buff, elements, 
		       ByteLength(m2k_type), isComplex(m2k_type));
  }
  return OpalValue(final_result);
}


// For pickling

char ChooseNumber (const OpalValue& ov, string& arg1, string& arg2)
{
  char choose;
  Number n = UnOpalize(ov, Number);
  switch (n.type()) {
  case M2_BYTE     : // int_1
  case M2_UBYTE    : // int_u1
  case M2_INTEGER  : // int_2
  case M2_UINTEGER : // int_u2
  case M2_LONG     : // int_4
    {
      choose = 'i'; int_4 i4 = int_4(n);
      arg1 = IntToString(i4);
      break;
    }
  case M2_XLONG  : // int_8
    {
      choose = 'l'; int_8 i8 = int_8(n);
      arg1 = Stringize(i8);
      break;
    }
  case M2_ULONG  : // int_u4
  case M2_UXLONG : // int_u8
    {
      choose = 'l'; int_u8 iu8 = int_u8(n);
      arg1 = Stringize(iu8);
      break;
    }
  case M2_FLOAT  : // real_4
  case M2_DOUBLE : // real_8
    {
      choose = 'd'; real_8 d8 = real_8(n);
      arg1 = Stringize(d8);
      break;
    }
  case M2_CX_FLOAT : // complex_8
  case M2_CX_DOUBLE : // complex_16
    {
      choose = 'D'; ComplexT<real_8> c16 = ComplexT<real_8>(n);
      arg1 = Stringize(c16.re); arg2 = Stringize(c16.im);
      break;
    }
  case M2_TIME: 
  case M2_DURATION: {
      choose = 'a';
      arg1 = ov.stringValue();
      break;
  }
  default: {
      MakeWarning("Don't know how to serialize numeric type:"+
		  Stringize(n.type())+". Converting to string.\n");
      choose = 'a';
      arg1 = ov.stringValue();
      break;
    }
  }
  return choose;
}

string BuildVector (const OpalValue& ov, const string& typecode)
{  
  // Lookup the m2k typecode from the python typecode
  char python_typecode[]   = "dlFD";
  char* where = strchr(python_typecode, typecode[0]);
  if (!where) {
    MakeException("Unknown depickling typecode"+typecode);
  }
  Numeric_e m2k_typecode[] = { DOUBLE, LONG, CX_FLOAT, CX_DOUBLE };
  Numeric_e m2k_type = m2k_typecode[where-python_typecode];

  // Build the Vector of the type that maps to some Python entity
  Vector v = ov.vector();
  v.convert(m2k_type);
  char* vbuff = (char*)v.writeData();
  const int   elements = v.elements();
  const int   byte_len = ByteLength(v.format());
  if (!IsLittleEndian()) { // machine is big endian, convert!
    InPlaceReEndianize(vbuff, elements, byte_len, isComplex(m2k_type));
  }
  // Why *2?  Because about 3/5 take exactly 1 character, about half
  // *take 2/5 take 4 characters
  Array<char> a(elements*2); 
  PrintBufferToString(vbuff, elements*byte_len, a);
  return string(a.data());
}

#include "m2opalheader.h"
#include "m2opallink.h"
#include "m2tableize.h"

void UnknownType (PythonPicklerA<OpalValue>& pickler, const OpalValue& ov)
{
  if (ov.type()==OpalValueA_MULTIVECTOR) { // dump mv as table of vectors
    if (pickler.warn()) {
      MakeWarning("Found MultiVector during Python Pickling, turning into List of Vectors");
    }       
    pickler.dumpList(MultiVectorToList(ov));
    return; 

  } else if (ov.type()==OpalValueA_HEADER) { // dump as a table
    if (pickler.warn()) { 
      MakeWarning("Found OpalHeader during Python Pickling,turning into table");
    }
    pickler.dumpTable(OpalHeaderToTable(ov));
    return;

  } else if (ov.type()==OpalValueA_LINK) {  // dump as a copy of the table?
    if (pickler.warn()) {
      MakeWarning("Found OpalLink during Python Pickling, turning into string");
    }
    pickler.dumpString(OpalLinkToString(ov));
    return;

  } else if (ov.type() == OpalValueA_EVENTDATA) {
    if (pickler.warn()) {
      MakeWarning("Found EventData during Python Pickling, turning into Table");
    }
    pickler.dumpTable(EventDataToTable(ov));
    return;

  } else if (ov.type() == OpalValueA_TIMEPACKET) {
    if (pickler.warn()) {
      MakeWarning("Found TimePacket during Python Pickling, turning into a table");
    }
    pickler.dumpTable(TimePacketToTable(ov));
    return;

  } else if (ov.type() == OpalValueA_USERDEFINED) {
    if (pickler.warn()) {
      MakeWarning("Found User Defined data during Python Pickling, turning into string");
    }
    pickler.dumpString(ov.stringValue());
    return;

  } else {
    // if (pickler.warn()) {
    MakeWarning("Found Unknown type:'"+Stringize(ov.type())+"' during Python Pickling, turning into string");
    pickler.dumpString(ov.stringValue());
    return;

  }
}


// Helper: convert Val type codes into NumPy type strings
string OBJToNumPyCode (const OpalValue& ov)
{
  Vector v = UnOpalize(ov, Vector);
  switch (v.type()) {
  case M2_BYTE: return "i1";
  case M2_UBYTE: return "u1";
  case M2_INTEGER: return "i2";
  case M2_UINTEGER: return "u2";
  case M2_LONG: return "i4";
  case M2_ULONG: return "u4";
  case M2_XLONG: return "i8";
  case M2_UXLONG: return "u8";
  case M2_FLOAT: return "f4";
  case M2_DOUBLE: return "f8";
  case M2_CX_FLOAT: return "c8";
  case M2_CX_DOUBLE: return "c16";
  case M2_BIT: return "b1";

  case M2_TIME: return "u8";
  case M2_DURATION: return "f8";

  case M2_UNDEFINED: return "f8"; 

  default: {
    string ty=Stringize(v.type()); throw MidasException("Cannot handle arrays of "+ty);
  }
  }
}

#define NUMPY_NUMERIC_MAP(T) { return NumericTypeLookup((T*)0); }
static Numeric_e NumPyCodeToM2k (const string& numpy_typecode)
{
  char type_tag = numpy_typecode[0];
  int  type_len = atoi(numpy_typecode.substr(1).c_str());
  switch (type_tag) {
  case 'i': 
    switch(type_len) {
    case 1: NUMPY_NUMERIC_MAP(int_1); break;
    case 2: NUMPY_NUMERIC_MAP(int_2); break;
    case 4: NUMPY_NUMERIC_MAP(int_4); break;
    case 8: NUMPY_NUMERIC_MAP(int_8); break;
    default: throw MidasException("Unknown type string on NumPy:"+ numpy_typecode);
    }
    break; // integers 
  case 'u':
    switch(type_len) {
    case 1: NUMPY_NUMERIC_MAP(int_u1); break;
    case 2: NUMPY_NUMERIC_MAP(int_u2); break;
    case 4: NUMPY_NUMERIC_MAP(int_u4); break;
    case 8: NUMPY_NUMERIC_MAP(int_u8); break;
    default: throw MidasException("Unknown type string on NumPy:"+ numpy_typecode);
    } 
    break; // unsigned integers
  case 'f': 
    switch(type_len) {
    case 4: NUMPY_NUMERIC_MAP(real_4); break;
    case 8: NUMPY_NUMERIC_MAP(real_8); break;
    default: throw MidasException("Unknown type string on NumPy:"+ numpy_typecode);
    }
    break; // floats
  case 'c':
    switch(type_len) {
    case 8: NUMPY_NUMERIC_MAP(complex_8); break;
    case 16: NUMPY_NUMERIC_MAP(complex_16); break;
    default: throw MidasException("Unknown type string on NumPy:"+ numpy_typecode);
    } 
    break; // complexes
  case 'b': 
    switch(type_len) {
    case 1: NUMPY_NUMERIC_MAP(bool); break;
    default: throw MidasException("Unknown type string on NumPy:"+ numpy_typecode);
    }
    break; // bool
  case 'S': 
    break; // string, ie., numpy.character
  default: throw MidasException("Unknown type string on NumPy:"+ numpy_typecode);
  }

  return DOUBLE;
}


string BuildNumPyVector (const OpalValue& ov, const string& endian)
{
  // Build the Vector of the type that maps to some Python entity
  Vector v = ov.vector();
  Numeric_e m2k_type = v.type();
  char* vbuff = (char*)v.writeData();
  const int   elements = v.elements();
  const int   byte_len = ByteLength(v.format());
  if (endian==">" && !IsLittleEndian()) { // machine is big endian, convert!
    InPlaceReEndianize(vbuff, elements, byte_len, isComplex(m2k_type));
  }
  // Why *2?  Because about 3/5 take exactly 1 character, about half
  // *take 2/5 take 4 characters
  Array<char> a(elements*2); 
  PrintBufferToString(vbuff, elements*byte_len, a);
  return string(a.data());
}
