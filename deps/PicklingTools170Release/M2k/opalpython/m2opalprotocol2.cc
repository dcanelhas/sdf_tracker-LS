
#include "m2opalprotocol2.h"
#include "m2pythontools.h"

inline int p2error_ (string s)
{
  throw MidasException(s);
  return 0;
}

#include "p2common.h"
#include "m2opalgrapharray.h"
#include "m2opalgraphhash.h"

#include "m2tableize.h"

// MITE option tree builds slightly differetly
#if defined(M2KLITE)   
using std::ostream; using std::istream; using std::cerr;
#endif


// Forwards
inline bool P2LoadValue (OpalValue& ov, LoadContext_& lc);

inline void P2DumpValue (const OpalValue& ov, DumpContext_& dc);
inline void P2DumpTable (const OpalTable& t, DumpContext_& dc);
inline void P2DumpList (const OpalTable& a, DumpContext_& dc);

inline int BytesToDumpTable (const OpalTable& t, DumpContext_& dc);
inline int BytesToDumpList (const OpalTable& t, DumpContext_& dc);
inline int BytesToDumpVal (const OpalValue& v, DumpContext_& dc);

char* P2TopLevelLoadOpal (OpalValue& ov, char* mem)
{ return toplevelload_(ov, mem); }

 
char* P2TopLevelDumpOpal (const OpalValue& ov, char* mem, 
			  ArrayDisposition_e dis,
			  PicklingIssues_e issues)
{ return topleveldump_(ov, mem, dis, issues); }

int P2TopLevelBytesToDumpOpal (const OpalValue& v, 
			       ArrayDisposition_e dis,
			       PicklingIssues_e issues)
{ return TopLevelBytesToDump (v, dis, issues); }


// Too many objects (mostly strings) get constructed while just trying
// to count bytes: rather than do that, let's get straight at the
// string Again: this assumes the implementation stays the same ...
// In other words: we want a reference to the string inside the
// OpalValue->OpalValueA->OpalValueT!!
template <class T>
struct HackToGetToT : public OpalValueA {
  HackToGetToT () : OpalValueA(OpalValueA_USERDEFINED),
		    to_something(0) { }
  virtual ~HackToGetToT () { }
  inline virtual string stringValue () const { return ""; }
  inline virtual Number number () const { return Number(0); }
  inline virtual Vector vector () const { return Vector(); }
  inline virtual void prettyPrint (ostream& out, Size indent = 0) const { }
  inline virtual void formattedRead (istream& in) { }
  inline virtual void binarySerialize (OMemStream& out) const { }
  inline virtual void binaryDeserialize (IMemStream& in) { }
  inline virtual OpalValueA* clone (char* data) const { return 0; }
  inline virtual bool compare (const OpalValueA& rhs) const { return true; }
  T inside;
  string* to_something;
}; // Hack

template <class T>
static inline T& GetTHack (const OpalValue& ov) 
{
  // Assume is a string: OpalValueA_type == string
  OpalValueA* ovap = ov.data();
  HackToGetToT<T>* hp = (HackToGetToT<T>*)ovap;
  return hp->inside;
}


#define P2_PLAINARRAYDUMP(T, FUN) { T*d=(T*)vec.readData();int len=vec.elements();for (int ii=0;ii<len;ii++) { FUN(d[ii], dc); } } 
#define P2_PLAINARRAYDUMPWITHHELP(T, FUN, HELP) { T*d=(T*)vec.readData();int len=vec.elements();for (int ii=0;ii<len;ii++) { FUN(HELP(d[ii]), dc); } } 

// The user doesn't have Numeric installed and Array is an older version
// of python that doesn't work (!), so we have to be able to give them
// back something:  an Array of Generic Values.
inline void dumpOpalArrayAsList_ (const OpalValue& ov, DumpContext_& dc)
{
  Vector vec = UnOpalize(ov, Vector);
  int length = vec.elements();

  // Other arrays ...
  *(dc.mem)++ = PY_EMPTY_LIST;
  if (length==0) return;
  if (length!=1) *(dc.mem)++ = '(';
  
  switch (vec.type()) {
  case M2_BYTE:     P2_PLAINARRAYDUMP(int_1,     dump4ByteInteger);  break;
  case M2_UBYTE:    P2_PLAINARRAYDUMP(int_u1,    dump4ByteInteger);  break;
  case M2_INTEGER:  P2_PLAINARRAYDUMP(int_2,     dump4ByteInteger);  break;
  case M2_UINTEGER: P2_PLAINARRAYDUMP(int_u2,    dump4ByteInteger);  break;
  case M2_LONG    : P2_PLAINARRAYDUMP(int_4,     dump4ByteInteger);  break;
  case M2_ULONG:    P2_PLAINARRAYDUMP(int_u4,    dump4ByteInteger);  break;
  case M2_XLONG:    P2_PLAINARRAYDUMP(int_8,     dump8ByteInteger);  break;
  case M2_UXLONG:   P2_PLAINARRAYDUMP(int_u8,    dump8ByteUnsignedInteger);  break;
  case M2_FLOAT:    P2_PLAINARRAYDUMP(real_4,    dump8ByteFloat);    break;
  case M2_DOUBLE:   P2_PLAINARRAYDUMP(real_8,    dump8ByteFloat);    break;
  case M2_CX_FLOAT: P2_PLAINARRAYDUMPWITHHELP(complex_8, dump16ByteComplex, complex_16); break;
  case M2_CX_DOUBLE:P2_PLAINARRAYDUMP(complex_16,dump16ByteComplex); break;
  case M2_TIME:     P2_PLAINARRAYDUMPWITHHELP(M2Time,    dumpString, Stringize); break;
  case M2_DURATION: P2_PLAINARRAYDUMPWITHHELP(M2Duration,dumpString, Stringize); break;
  default: p2error_("unknown Vector type in dumpOpalArrayAsList_");
  }
  *(dc.mem)++ = (length==1) ? PY_APPEND : PY_APPENDS;
}  

inline int BytesToDumpOpalArrayAsList_ (const OpalValue& ov, DumpContext_& dc)
{
  Vector vec = UnOpalize(ov, Vector);
  int bytes = 3;
  const int len = vec.elements();
  int element_size = 0;
  switch (vec.type()) {
  case M2_BYTE:     element_size = BytesToDump4ByteInteger();  break;
  case M2_UBYTE:    element_size = BytesToDump4ByteInteger();  break;
  case M2_INTEGER:  element_size = BytesToDump4ByteInteger();  break;
  case M2_UINTEGER: element_size = BytesToDump4ByteInteger();  break;
  case M2_LONG    : element_size = BytesToDump4ByteInteger();  break;
  case M2_ULONG:    element_size = BytesToDump4ByteInteger();  break;
  case M2_XLONG:    element_size = BytesToDump8ByteInteger(dc);  break;
  case M2_UXLONG:   element_size = BytesToDump8ByteUnsignedInteger(dc);  break;
  case M2_FLOAT:    element_size = BytesToDump8ByteFloat();    break;
  case M2_DOUBLE:   element_size = BytesToDump8ByteFloat();    break;
  case M2_CX_FLOAT: element_size = BytesToDump16ByteComplex(dc); break;
  case M2_CX_DOUBLE:element_size = BytesToDump16ByteComplex(dc); break;
  case M2_TIME:     element_size = 50; break; // Overestime
  case M2_DURATION: element_size = 20; break; // Overestime
  default: p2error_("unknown Vector type in dumpOpalArrayAsList_");
  }
  bytes += element_size*len;
  return bytes;
}  




// This is a newer feature, allowing us to dump arrays as Python
// package array, but it is broken in some version of Pythons,
// so we have to make sure it is a newer version of Python
inline void dumpOpalArray_ (const OpalValue& ov, DumpContext_& dc)
{
  Vector vec = UnOpalize(ov, Vector);

  // Assume v is an Array
  int sz = -1;
  const char* c;
  switch (vec.type()) { //array.array typecodes
  case M2_BYTE:      sz=sizeof(int_1);  c="c"; break;
  case M2_UBYTE:     sz=sizeof(int_u1); c="B"; break;
  case M2_INTEGER:   sz=sizeof(int_2);  c="h"; break;
  case M2_UINTEGER:  sz=sizeof(int_u2); c="H"; break;
  case M2_LONG:      sz=sizeof(int_4);  c="i"; break;
  case M2_ULONG:     sz=sizeof(int_u4); c="I"; break;
  case M2_XLONG:     sz=sizeof(int_8);  c="l"; break;
  case M2_UXLONG:    sz=sizeof(int_u8); c="L"; break;
  case M2_FLOAT:     sz=sizeof(real_4); c="f"; break;
  case M2_DOUBLE:    sz=sizeof(real_8); c="d"; break;
  case M2_CX_FLOAT:  sz=sizeof(complex_8);  c="F"; break;
  case M2_CX_DOUBLE: sz=sizeof(complex_16); c="D"; break;
  case M2_TIME:      sz=sizeof(int_u8); c="L"; break; 
  case M2_DURATION:  sz=sizeof(M2Duration); c="d"; break; 
  default: p2error_("Can't dumpOpalArray");
  }
  
  // THIS IS A POD Array!  can bit blit
  
  // Dump the memoize to make it faster
  if (dc.array_preamble_dumped) {
    P2DumpCodeAndInt_(dc.array_handle, PY_BINGET, PY_LONG_BINGET, dc);
  } else {
    // Not dumped, figure out what handle should be
    dc.array_handle = dc.current_handle++;
    dc.array_preamble_dumped = true;
    
    // Then dump it (with memo to annotate it)
    memcpy(dc.mem, ArrayPreamble, sizeof(ArrayPreamble)-1);
    dc.mem += sizeof(ArrayPreamble)-1;
    P2DumpCodeAndInt_(dc.array_handle, PY_BINPUT, PY_LONG_BINPUT, dc);
  }
  
  // Same layout, regardless of type.
  // TODO:  Will we have to reendiaze this?
  char* dat = (char*)vec.readData();
  
  // Dump the format before the data
  dumpCString(c, 1, dc);
  dumpCString(dat, vec.elements()*sz, dc);
  *(dc.mem)++ = PY_TUPLE2;
  *(dc.mem)++ = PY_REDUCE;
} 
    

inline int BytesToDumpOpalArray_ (const OpalValue& ov, DumpContext_& dc)
{
  Vector v = UnOpalize(ov, Vector);
  int bytes = ByteLength(v.type())*v.elements() + 26;
  if (dc.array_preamble_dumped) {
    bytes += 2;
  } else {
    dc.array_handle = dc.current_handle++;
    dc.array_preamble_dumped = true;
    bytes += sizeof(ArrayPreamble)+2;
  }
  return bytes;
  
}


// Helper function so we can debug dumps to memory
template <class PT, class T>
void dumpNumericHelp_(PT*, T*, const char*c, DumpContext_& dc, const Vector& v)
{
  dumpCString(c, 1, dc); // Type string

  // Get necessary data 
  T* od = (T*)v.readData();
  int elements = v.elements();

  // Dump string header
  int len = elements*sizeof(PT);
  P2DumpCodeAndInt_(len, PY_SHORT_BINSTRING, PY_BINSTRING, dc);

  // Dump actual data for string
  PT* sm=(PT*)dc.mem; 
  for (int ii=0; ii<elements; ii++) { 
    sm[ii] = od[ii];
  }
  dc.mem += len;
}

// If someone is using XMPY, they may want their Numeric Arrays
void dumpOpalNumericArray_ (const OpalValue& ov, DumpContext_& dc)
{
  Vector v = UnOpalize(ov, Vector);
  
  // THIS IS A POD Array!  can bit blit
  const char* c;
  
  // If already memoized, just dump the memo for faster serialization
  if (dc.numeric_preamble_dumped) {
    P2DumpCodeAndInt_(dc.numeric_handle, PY_BINGET, PY_LONG_BINGET, dc);
  } else {
    // Get new handle
    dc.numeric_handle = dc.current_handle++;
    dc.numeric_preamble_dumped = true;

    // Dump the original data
    memcpy(dc.mem, NumericPreamble, sizeof(NumericPreamble)-1);
    dc.mem += sizeof(NumericPreamble)-1;
    P2DumpCodeAndInt_(dc.numeric_handle, PY_BINPUT, PY_LONG_BINPUT, dc);
  }
  
  // Same layout, regardless of type.
  // TODO:  Will we have to reendiaze this?

  // Dump (, then length of array, then tuple
  *(dc.mem)++ = '(';
  if (dc.pickling_issues==AS_PYTHON_2_2) {
    *(dc.mem)++ = '(';
  }
  int elements = v.elements();
  dump4ByteInteger(elements, dc); 
  *(dc.mem)++ = (dc.pickling_issues==AS_PYTHON_2_2) ? 't' : PY_TUPLE1;
    
  // Dump string with typecode, then dump as data
  switch (v.type()) {
  case M2_BYTE:    c="1"; dumpNumericHelp_((int_1*)0, (int_1*)0, c,dc,v);break;
  case M2_UBYTE:   c="b"; dumpNumericHelp_((int_u1*)0,(int_u1*)0,c,dc,v);break;
  case M2_INTEGER: c="s"; dumpNumericHelp_((int_2*)0, (int_2*)0, c,dc,v);break;
  case M2_UINTEGER:c="w"; dumpNumericHelp_((int_u2*)0,(int_u2*)0,c,dc,v);break;
  case M2_LONG:    c="i"; dumpNumericHelp_((int_4*)0, (int_4*)0, c,dc,v);break;
  case M2_ULONG:   c="u"; dumpNumericHelp_((int_u4*)0,(int_u4*)0, c,dc,v);break;

  // These two may or may not be supported???  If the version of Python
  // supports long as 8 bytes, then this is the best we can do.  It's
  // unfortunate that Numeric is "restricted" in its int types.  As long as
  // we are talking to implementations that support the 'l' as int_u8 
  // typecode, we don't lose any information.

  //case M2_XLONG: c="l";dumpNumericHelp_((int_8*)0,(int_8*)0,c,dc,v);break;
  //case M2_UXLONG:c="x";dumpNumericHelp_((int_u8*)0,(int_u8*)0,c,dc,v);break;
  case M2_XLONG:  c="l"; dumpNumericHelp_((long*)0, (int_8*)0,c,dc,v);break;
  case M2_UXLONG: c="l"; dumpNumericHelp_((long*)0, (int_u8*)0,c,dc,v);break;

  case M2_FLOAT: c="f"; dumpNumericHelp_((real_4*)0, (real_4*)0, c,dc,v);break;
  case M2_DOUBLE:c="d"; dumpNumericHelp_((real_8*)0, (real_8*)0, c,dc,v);break;
  case M2_CX_FLOAT:c="F"; dumpNumericHelp_((complex_8*)0, (complex_8*)0, c,dc,v);break;  
  case M2_CX_DOUBLE: c="D"; dumpNumericHelp_((complex_16*)0, (complex_16*)0, c, dc, v); break;
    // The M2Time has the same limitations as M2_XLONG and M2_UXLONG above
  case M2_TIME: c="l"; dumpNumericHelp_((long*)0, (int_u8*)0, c,dc,v);break;
  case M2_DURATION:c="d";dumpNumericHelp_((real_8*)0, (real_8*)0,c,dc,v);break;
  default: p2error_("Can't use type in dumpOpalNumericArray_");
  }

  if (dc.pickling_issues==AS_PYTHON_2_2) {
    dump4ByteInteger(1, dc);
  } else { 
    *(dc.mem)++ = PY_NEWTRUE;
  }
  *(dc.mem)++ = 't';
  *(dc.mem)++ = PY_REDUCE;
}
    
inline int BytesToDumpOpalNumericArray_ (const OpalValue& ov, DumpContext_& dc)
{
  Vector v = UnOpalize(ov, Vector);
  int bytes = ByteLength(v.type())*v.elements()+14+2*BytesToDump4ByteInteger();
  if (dc.numeric_preamble_dumped) {
    bytes += 2;
  } else {
    dc.numeric_handle = dc.current_handle++;
    dc.numeric_preamble_dumped = true;
    bytes += sizeof(NumericPreamble)+2;
  }
  return bytes;
}


// NumPy dumps

// Helper: convert Val type codes into NumPy type strings
string M2kToNumPyCode (Numeric_e type)
{
  switch (type) {
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
    string ty=Stringize(type); throw MidasException("Cannot handle arrays of "+ty);
  }
  }
}

bool IsLittleEndian();

// Used for dumping tuples ... M2k has no notion of tuples, but 
// cetain things have to be dumped as tuples for pickling to work
void P2DumpTuple (const OpalValue& ov, DumpContext_& dc)
{
  OpalTable ot = UnOpalize(ov, OpalTable);
  if (ot.implementation().type() != OpalTableImpl_GRAPHARRAY) {
    throw MidasException("Not a tuple");
  }

  // longer tuples require a mark
  int len = ot.length();
  if (len>3) {  
    *(dc.mem)++ = PY_MARK;
  }

  // Dump each element
  for (int ii=0; ii<len; ii++) {
    OpalValue t = ot.get(ii);
    P2DumpValue(t, dc);
  }
  
  // Small tuples have special opcode
  const int opcodes[] = {PY_EMPTY_TUPLE, PY_TUPLE1, PY_TUPLE2, PY_TUPLE3 };
  if (len>3) {
    *(dc.mem)++ = PY_TUPLE;
  } else {
    *(dc.mem)++ = opcodes[len];
  }
			
}

// If someone is using XMPY, they may want their Numeric Arrays
void dumpOpalNumPyArray_ (const OpalValue& ov, DumpContext_& dc)
{
  Vector v = UnOpalize(ov, Vector);
  Numeric_e subtype=v.type();

  // THIS IS A POD Array!  can bit blit
  char* c;
  int shape = v.elements();
  
  // PY_GLOBAL reconstruct
  PreambleDumperNumPyReconstruct(dc);
  
  // Initial arguments to a "prototype" NDArray
  PreambleDumperNumPyNDArray(dc);   // 1: GLOBAL
  OpalTable unused_shape; unused_shape.append(OpalValue(Number(int_4(0)))); // Tup(Val(0))
  P2DumpTuple(unused_shape, dc);    // 2: initial unused shape
  dumpCString("b",1,dc);            // 3: initial unused type
  *(dc.mem)++ = PY_TUPLE3;          // (1,2,3)
  *(dc.mem)++ = PY_REDUCE;

  // Starting argumements for "prototype" multiarray BUILD
  {
    *(dc.mem)++ = PY_MARK;
    
    P2DumpValue(OpalValue(Number(int_4(1))), dc);
    // Shape in a tuple
    OpalTable shape_tuple; shape_tuple.append(OpalValue(Number(int_4(shape))));
    P2DumpTuple(shape_tuple, dc);
    
    // Starting DTYPE
    {
      // Initial args to a "prototype" dtype
      PreambleDumperNumPyDtype(dc);
      string numpy_code = M2kToNumPyCode(subtype);

      OpalTable dtype_initial;       // Tup dtype_initial(numpy_code, 0, 1);
      dtype_initial.append(numpy_code);
      dtype_initial.append(OpalValue(Number(int_4(0))));
      dtype_initial.append(OpalValue(Number(int_4(1))));
      P2DumpTuple(dtype_initial, dc);
      *(dc.mem)++ = PY_REDUCE;

      // Tuple of arguments that get applied to "prototype" dtype BUILD
      string endian = ByteLength(subtype)==1 ? "|" : IsLittleEndian() ? "<" : ">";
      //Tup dtype_args(3, endian, None, None, None, -1, -1, 0);
      OpalValue none("None", OpalValueA_TEXT); 
      OpalTable dtype_args;
      dtype_args.append(OpalValue(Number(int_4(3))));
      dtype_args.append(endian);
      dtype_args.append(none);
      dtype_args.append(none);
      dtype_args.append(none);
      dtype_args.append(OpalValue(Number(int_4(-1))));
      dtype_args.append(OpalValue(Number(int_4(-1))));
      dtype_args.append(OpalValue(Number(int_4(0))));

      P2DumpTuple(dtype_args, dc);
      *(dc.mem)++ = PY_BUILD;
    }
    // Assertion: Dtype top thing on values stack
    
    *(dc.mem)++ = PY_NEWFALSE;

    // Dump the actual data
    const char* raw_data = (const char*)v.readData(); 
    dumpCString(raw_data, ByteLength(subtype)*shape, dc);
    
    *(dc.mem)++ = PY_TUPLE;
  }
  *(dc.mem)++ = PY_BUILD;
  //if (memoize_self) { MemoizeSelf_(memoize_self, dc); }
}


inline int BytesToDumpOpalNumPyArray_ (const OpalValue& ov, 
				       DumpContext_& dc)
{

  // Otherwise, POD data
  Vector v = ov.vector();
  Numeric_e subtype = v.type();
  int bytes = ByteLength(subtype)*v.elements()+50+2*BytesToDump4ByteInteger();
  bytes += BytesPreambleNumPyReconstruct(dc);
  bytes += BytesPreambleNumPyNDArray(dc);
  bytes += BytesPreambleNumPyDtype(dc);
  return bytes;
}


// End NumPy

void dumpOpalNumber_ (const OpalValue& ov, DumpContext_& dc)
{
  // Number n = UnOpalize(ov, Number);
  Number& n = GetTHack<Number>(ov);
    switch (n.type()) {
    case M2_BYTE     : // int_1
    case M2_UBYTE    : // int_u1
    case M2_INTEGER  : // int_2
    case M2_UINTEGER : // int_u2
    case M2_LONG     : // int_4
      {
	int_4 i4 = int_4(n);
	dump4ByteInteger(i4,dc);   
	break;
      }
    case M2_XLONG  : // int_8
      {
	int_8 i8 = int_8(n);
	dump8ByteInteger(i8, dc); 
	break;
      }
    case M2_ULONG  : // int_u4
    case M2_UXLONG : // int_u8
      {
	int_u8 iu8 = int_u8(n);
	dump8ByteUnsignedInteger(iu8, dc); 
	break;
      }
    case M2_FLOAT  : // real_4
    case M2_DOUBLE : // real_8
      {
	real_8 d8 = real_8(n);
	dump8ByteFloat(d8, dc);    break;
	break;
      }
    case M2_CX_FLOAT : // complex_8
    case M2_CX_DOUBLE : // complex_16
      {
	complex_16 c16 = n.operator complex_16();
	dump16ByteComplex(c16, dc); break;
	break;
      }
    case M2_TIME:  // Turn em into strings
    case M2_DURATION: {
      string arg1 = ov.stringValue();
      dumpString(arg1, dc);
      break;
    }
    default: {
      cerr << string("Don't know how to serialize numeric type:"+
		     Stringize(n.type())+". Converting to string.\n");
      string arg1 = ov.stringValue();
      dumpString(arg1, dc);
      break;
    }
    }
}

inline int BytesToDumpOpalNumber_ (const OpalValue& ov, DumpContext_& dc)
{
  // Number n = UnOpalize(ov, Number);
  Number& n = GetTHack<Number>(ov);
    switch (n.type()) {
    case M2_BYTE     : // int_1
    case M2_UBYTE    : // int_u1
    case M2_INTEGER  : // int_2
    case M2_UINTEGER : // int_u2
    case M2_LONG     : // int_4
      return BytesToDump4ByteInteger();  
    case M2_XLONG  : // int_8
      return BytesToDump8ByteInteger(dc); 
    case M2_ULONG  : // int_u4
    case M2_UXLONG : // int_u8
      return BytesToDump8ByteUnsignedInteger(dc); 
    case M2_FLOAT  : // real_4
    case M2_DOUBLE : // real_8
      return BytesToDump8ByteFloat();    break;
    case M2_CX_FLOAT : // complex_8
    case M2_CX_DOUBLE : // complex_16
      return BytesToDump16ByteComplex(dc); break;
    case M2_TIME:  // Turn em into strings
    case M2_DURATION: {
      string arg1 = ov.stringValue();
      return BytesToDumpString(arg1);
    }
    default: {
      cerr << string("WARNING: Don't know how to serialize numeric type:"+
		     Stringize(n.type())+". Converting to string.\n");
      string arg1 = ov.stringValue();
      return BytesToDumpString(arg1);
    }
    }
}

#include "m2opalheader.h"
#include "m2opallink.h"
#include "m2opalconv.h" 

// Generic dump
inline void P2DumpValue (const OpalValue& ov, DumpContext_& dc)
{
  switch(ov.type()) {
  
  case OpalValueA_NUMBER: dumpOpalNumber_(ov, dc); break;
  case OpalValueA_BOOL:  { bool b=UnOpalize(ov,bool); dumpBool(b, dc); break; }

  case OpalValueA_TABLE: { // Either a table or list
    OpalTable* tp = dynamic_cast(OpalTable*, ov.data());
    OpalTableImpl_Type_e typer = tp->implementation().type();
    switch (typer) { 
    case OpalTableImpl_GRAPHHASH:
    case OpalTableImpl_NONE: 
      P2DumpTable(*tp, dc);    break;
    case OpalTableImpl_GRAPHARRAY:
      P2DumpList(*tp, dc);    break;
    default: p2error_("Unknown how to serialize:"+Stringize(typer));
    }
    break;
  }

  case OpalValueA_HEADER: P2DumpValue(OpalHeaderToTable(ov),dc); break;

  case OpalValueA_LINK: dumpString(OpalLinkToString(ov), dc); break;

  case OpalValueA_STRING : {
    string& arg = GetTHack<string>(ov);
    // string arg = ov.stringValue(); 
    dumpString(arg, dc); break;
  }

  case OpalValueA_VECTOR: {
    switch (dc.disposition) {
    case AS_LIST: dumpOpalArrayAsList_(ov,dc); break;
    case AS_PYTHON_ARRAY: dumpOpalArray_(ov,dc); break;
    case AS_NUMERIC: dumpOpalNumericArray_(ov,dc); break;
    case AS_NUMPY: dumpOpalNumPyArray_(ov,dc); break;
    default: p2error_("unknown disposition for dumping arrays");
    }
    break; 
  }

  case OpalValueA_MULTIVECTOR: P2DumpList(MultiVectorToList(ov), dc); break;

  case OpalValueA_TEXT : *(dc.mem)++ = 'N'; break;

  case OpalValueA_EVENTDATA: P2DumpTable(EventDataToTable(ov),dc); break;

  case OpalValueA_TIMEPACKET: P2DumpTable(TimePacketToTable(ov),dc); break;

  case OpalValueA_USERDEFINED: dumpString(ov.stringValue(), dc); break;

  default:  p2error_("Unknown type in dump:"+Stringize(ov.type()));  
  }
}

inline int BytesToDumpVal (const OpalValue& ov, DumpContext_& dc)
{
  switch(ov.type()) {
  
  case OpalValueA_NUMBER: return BytesToDumpOpalNumber_(ov, dc); 
  case OpalValueA_BOOL:   return BytesToDumpBool(); 

  case OpalValueA_TABLE: { // Either a table or list
    OpalTable* tp = dynamic_cast(OpalTable*, ov.data());
    OpalTableImpl_Type_e typer = tp->implementation().type();
    switch (typer) { 
    case OpalTableImpl_GRAPHHASH:
    case OpalTableImpl_NONE: 
      return BytesToDumpTable(*tp, dc);   
    case OpalTableImpl_GRAPHARRAY:
      return BytesToDumpList(*tp, dc);    
    default: p2error_("Unknown how to serialize:"+Stringize(typer));
    }
    break;
  }

  case OpalValueA_HEADER: { // Should just serialize like a Table
    OpalHeader opalhdr(UnOpalize(ov, OpalHeader));
    return BytesToDumpVal(opalhdr.table(), dc); 
    break;
  };

  case OpalValueA_LINK: return BytesToDumpString(OpalLinkToString(ov)); break;

  case OpalValueA_STRING : {
    string& arg = GetTHack<string>(ov); 
    // string arg = ov.stringValue(); // Makes copies of strings ...
    return BytesToDumpString(arg); break;
  }

  case OpalValueA_VECTOR: {
    switch (dc.disposition) {
    case AS_LIST: return BytesToDumpOpalArrayAsList_(ov,dc); break;
    case AS_PYTHON_ARRAY: return BytesToDumpOpalArray_(ov,dc); break;
    case AS_NUMERIC: return BytesToDumpOpalNumericArray_(ov,dc); break;
    case AS_NUMPY: return BytesToDumpOpalNumPyArray_(ov,dc); break;
    default: p2error_("unknown disposition for dumping arrays");
    }
    break; 
  }

  case OpalValueA_MULTIVECTOR: 
    return BytesToDumpList(MultiVectorToList(ov), dc); break;
  
  case OpalValueA_TEXT : return 1; break;

  case OpalValueA_EVENTDATA: 
    return BytesToDumpTable(EventDataToTable(ov),dc);  break; 

  case OpalValueA_TIMEPACKET: 
    return BytesToDumpTable(TimePacketToTable(ov),dc); break;

  case OpalValueA_USERDEFINED: 
    return BytesToDumpString(ov.stringValue()); break;
  
  default:  p2error_("BytesToDumpVal: Unknown type in dump:"+Stringize(ov.type()));  
  }
  return 0;
}


// Dump an OpalTable as a Python Dictionary (pickling protocol 2)

inline void P2DumpTable (const OpalTable& t, DumpContext_& dc)
{
  const int len = t.entries();
  *(dc.mem)++ = PY_EMPTY_DICT;
  
  if (len!=0)  { // Empty Table just single }
    
    if (len!=1) *(dc.mem)++ = '(';

    OpalTableIterator ii(t);
    while (ii()) {
      const string& key = ii.key();
      const OpalValue value = ii.value();
      dumpString(key, dc);
      P2DumpValue(value, dc);
    }
    
    *(dc.mem)++ = (len==1) ? PY_SETITEM : PY_SETITEMS;
  }
}

inline int BytesToDumpTable (const OpalTable& t, DumpContext_& dc)
{
  int bytes = 1 + 1 + 1; // PY_EMPTY_DICT, ( PY_SETITEMS
  OpalTableIterator ii(t);
  while (ii()) {
    const string& key = ii.key();
    const OpalValue& value = ii.value();
    bytes += BytesToDumpString(key);
    bytes += BytesToDumpVal(value, dc);
  }
  return bytes;
}


// Dump a Arr as a Python List (pickling protocol 2)
inline void P2DumpList (const OpalTable& a, DumpContext_& dc)
{
  const int len = a.surfaceEntries();
  *(dc.mem)++ = PY_EMPTY_LIST;
  
  if (len!=0) { // Empty List just single ]
    
    if (len!=1) *(dc.mem)++ = '(';

    // Get implementation as GraphArray
    const OpalGraphArray& oga = dynamic_cast(const OpalGraphArray&, 
					     a.implementation());

    for (int ii=0; ii<len; ii++) {
      OpalEdge edge = oga.at(ii);
      OpalValueA* ovp = edge->data();
      OpalValue value(*ovp);
      P2DumpValue(value, dc);
    }
    
    *(dc.mem)++ = (len==1) ? PY_APPEND : PY_APPENDS;
  }
}

inline int BytesToDumpList (const OpalTable& a, DumpContext_& dc)
{
  int bytes = 1 + 1 + 1; // PY_EMPTY_LIST, ( PY_APPENDS
  const int len = a.surfaceEntries();
  const OpalGraphArray& oga = dynamic_cast(const OpalGraphArray&, 
					   a.implementation());
  for (int ii=0; ii<len; ii++) {
    OpalEdge edge = oga.at(ii);
    OpalValueA* ovp = edge->data();
    OpalValue value(*ovp);
    bytes += BytesToDumpVal(value, dc);
  }
  return bytes;
}





//**************************** LOAD SECTION **********************

// Needed inside of handleAGet_
inline void HandleGetAssignment (OpalValue& lhs, OpalValue& rhs)
{ lhs = rhs; }

#define P2_LOADARRAY(T, SUB) { int_u4 len=byte_len/sizeof(T); Vector vec(SUB, len); memcpy(vec.writeData(), lc.mem, byte_len); ov=OpalValue(vec);}

template <class OT>
void loadArray_ (OT& ov, LoadContext_& lc, bool saw_memo=false) 
{
  // If the preamble was memoized, then we saw the preamble already as a get
  if (!saw_memo) {
    lc.mem    = P2EXPECT_(ArrayPreamble, lc.mem);
    lc.handle = P2_ARRAY_HANDLE;
    handleAPut_(0, lc);
  }

  // Get a string describes the type of the array: it is 
  // a simple string with a typecode
  string string_desc;  
  P2LoadString(string_desc, lc);  
  char array_tag = string_desc[0];   // TODO:  More complex usage?

  // Start getting the length of the data array .. in bytes:
  // by getting the length first, we can preallocate the 
  // Array<T>
  int_u4 byte_len = loadStringLength_(lc);

  switch (array_tag) {
  case 'c': P2_LOADARRAY(int_1, M2_BYTE); break;
  case 'B': P2_LOADARRAY(int_u1,M2_UBYTE); break;
  case 'h': P2_LOADARRAY(int_2, M2_INTEGER); break;
  case 'H': P2_LOADARRAY(int_u2,M2_UINTEGER); break; 
  case 'i': P2_LOADARRAY(int_4, M2_LONG); break; 
  case 'I': P2_LOADARRAY(int_u4,M2_ULONG); break; 
  case 'l': P2_LOADARRAY(int_8, M2_XLONG); break; 
  case 'L': P2_LOADARRAY(int_u8,M2_UXLONG); break; 
  case 'f': P2_LOADARRAY(real_4,M2_FLOAT); break; 
  case 'd': P2_LOADARRAY(real_8,M2_DOUBLE); break; 
  case 'F': P2_LOADARRAY(complex_8, M2_CX_FLOAT); break; 
  case 'D': P2_LOADARRAY(complex_16,M2_CX_DOUBLE); break; 
  default: p2error_("Can't have anything other than POD data in an array");
  }

  // .. with length known, normally we'd finish "loading the string",
  //  but don't need to  ... we already memcpy the data from
  // the buffer!
  ///  -> NO NEEED! finishLoadingString_(v, dc);
  lc.mem += byte_len;  // Advance to end of string
  handleAPut_(&ov, lc); // PY_BINPUT, the handle  

  lc.mem = P2EXPECT_("\x86R", lc.mem);// PY_TUPLE, PY_REDUCE
  handleAPut_(&ov, lc); // PY_BINPUT, the handle
}


void loadOpalNumeric_ (OpalValue& ov, LoadContext_& lc, bool saw_memo=false) 
{
  // If the preamble was memoized, then we saw the preamble already as a get
  if (!saw_memo) {
    lc.mem    = P2EXPECT_(NumericPreamble, lc.mem);  
    lc.handle = P2_NUMERIC_HANDLE;
    handleAPut_(0, lc);
  }
  if (*(lc.mem)++!='(') p2error_("expected ( in Numeric Array");
  // .. and there might be another if this was Python 2.2!
  if (*lc.mem=='(') {
    lc.mem++;
  }

  // It's possible to send a shapeless array: Numeric.zeros((),'i') which
  // causes there to be a closing ')' instead of a number.  This causes
  // us to 'skip' the number of elements in the array and go straight to the
  // type code string.
  if (*lc.mem==')') {
    lc.mem++;
  } else {
    // get an integer:  this is the number of elements in the array
    OpalValue length_of_array;
    P2LoadValue(length_of_array, lc);
    int array_len = UnOpalize(length_of_array, int);

    char tp = *(lc.mem)++; // 't' is in Python 2.2, PY_TUPLE1 in > Python 2.2
    if (tp!='t'&&tp!=PY_TUPLE1) p2error_("expected start tuple in Numeric Array");
  }

  // Get a string describes the type of the array: it is 
  // a simple string with a typecode
  string string_desc;
  P2LoadString(string_desc, lc);
  char array_tag = string_desc[0];

  // Start getting the length of the data array .. in bytes:
  // by getting the length first, we can preallocate the 
  // Array<T>
  int_u4 byte_len = loadStringLength_(lc);

  switch (array_tag) {
  case '1': P2_LOADARRAY(int_1, M2_BYTE); break;
  case 'b': P2_LOADARRAY(int_u1,M2_UBYTE); break;
  case 's': P2_LOADARRAY(int_2, M2_INTEGER); break;
  case 'w': P2_LOADARRAY(int_u2,M2_UINTEGER); break; 
  case 'i': P2_LOADARRAY(int_4, M2_LONG); break; 
  case 'u': P2_LOADARRAY(int_u4,M2_ULONG); break; 

  // Careful with these: longs are necessarily int_8 or int_u8
  //case 'l': P2_LOADARRAY(int_8, M2_XLONG); break; 
  //case 'x': P2_LOADARRAY(int_u8,M2_UXLONG); break; 
  case 'l': {
    int len=byte_len/sizeof(long); 
    Vector vec(M2_XLONG, len); 
    int_8* out_data = (int_8*)vec.writeData();
    long*   in_data  = (long*)lc.mem;
    for (int ii=0; ii<len; ii++) {
      out_data[ii] = in_data[ii];
    }
    ov=OpalValue(vec);
    break;
  }

  case 'f': P2_LOADARRAY(real_4,M2_FLOAT); break; 
  case 'd': P2_LOADARRAY(real_8,M2_DOUBLE); break; 
  case 'F': P2_LOADARRAY(complex_8, M2_CX_FLOAT); break; 
  case 'D': P2_LOADARRAY(complex_16,M2_CX_DOUBLE); break; 
  default: p2error_("Can't have anything other than POD data in an array");
  }

  // .. with length known, normally we'd finish "loading the string",
  //  but don't need to  ... we already memcpy the data from
  // the buffer!
  ///  -> NO NEEED! finishLoadingString_(v, dc);
  lc.mem += byte_len;  // Advance to end of string
  handleAPut_(&ov, lc); // PY_BINPUT, the handle  

  char bool_of = *(lc.mem)++;
  if (bool_of!=PY_NEWTRUE && bool_of!=PY_NEWFALSE) {
    if (bool_of=='K') { // small int for small int for boolean
      bool_of = *(lc.mem)++;
    } else {
      p2error_("expected bool at end of Numeric array");
    }
  }

  lc.mem = P2EXPECT_("tR", lc.mem);// PY_TUPLE, PY_REDUCE
  handleAPut_(&ov, lc); // PY_BINPUT, the handle
}



// Get the offset of the tbl_: This implementation has to look
// JUST LIKE the OpalGraphHash so that we can correctly calculate offsets
// and look at the "private parts"
class HackToGetTbl : public OpalTableImpl {
public:
  virtual ~HackToGetTbl () { }
  HackToGetTbl() : OpalTableImpl(OpalTableImpl_NONE) { }
  virtual OpalTableIteratorImpl* makeIterator (SortOrder_e order) const { return 0; }
  virtual Size surfaceEntries () const { return 0; }
  virtual void clear () { }
  virtual bool valueExistsAtIndex (Index idx) const { return false; }
  virtual bool findEdge (const string& key, OpalEdge& edge) const { return false; }
  virtual bool contains (const string& key) const { return false; }
  virtual void put (const string& key, const OpalEdge& edge) { } 
  virtual void append (const OpalEdge& edge) { }
  virtual bool remove (const string& key) { return false; }
  virtual OpalTableImpl* clone () const { return 0; }

  HashTable<OpalEdge> tbl_;
}; // HackToGetTbl;
static HackToGetTbl StaticHackTable; // static so only have to create one once

// Once we have the table, we wan tto be able to get the location of the 
// string
struct HT : public HashTable<OpalEdge> {
  HashEntryT_<string, OpalEdge>* containerLookup (const string& s)
  {
    return this->containsHelper_(s);
  }
}; // HT



inline void loadSingleKeyValue_ (OpalGraphHash& ogh, LoadContext_& lc)
{
  // Load key: Note that memo for the key will be WRONG, since it
  // currently thinks the memo belongs on the stack!!
  OpalValue opal_key;
  lc.handle = -999;
  bool saw_bin_get = P2LoadValue(opal_key, lc);
  string key = opal_key.stringValue();
  int key_handle = lc.handle;
  
  // Put the key in the table, and get references to the 
  // Underlying Opalvalue in the key and value
  OpalEdge new_value(new OpalValue());
  ogh.put(key, new_value);
  OpalValue& value = *new_value;
  
  // Once the *key* is IN the table, it won't move.  We have to
  // readjust the memoization of the key since it exists now and is
  // in a stable place, whereas before it was on the stack
  if (!saw_bin_get && key_handle >= 0) {
    // HACK to get pointer to key!!!!
    //char* ptr = (char*)&value;
    //ptr -= sizeof(OpalEdge);// key always before value in struct, I Hope!!!!
    char* start = (char*) &StaticHackTable;
    char* end   = (char*) &StaticHackTable.tbl_;
    ptrdiff_t len = end-start;

    char* ogh_start = (char*)&ogh;
    char* table_start = ogh_start + len;

    HashTable<OpalEdge>* table_ptr = (HashTable<OpalEdge>*)table_start;
    HT* htp = (HT*) table_ptr;
    HashEntryT_<string, OpalEdge>* entry = htp->containerLookup(key);
    
    lc.memoize_table[key_handle] = &(entry->key);
    lc.is_string[key_handle]     = true;  // Key is ALWAYS a string!
  }
  
  // Load value
  lc.handle = -444;
  P2LoadValue(value, lc);
}


// When there MIGHT be a single value in an OpalTable list or Hash, we
// have to try to load it.  It that load fails, then we throw this
// exception to restart.  This is a bit of a hack.
struct BadRetry : public MidasException {
  BadRetry () { }
}; // BadRetry

inline void P2LoadTable (OpalValue& ov, LoadContext_& lc)
{
  // Initialize into so always start with empy table.  Shouldn't have
  // to worry about resizing: the table is implemented as a hash table
  // where the nodes don't move or resize.
  ov = OpalValue(OpalTable(OpalTableImpl_GRAPHHASH));

  // Painful to get the implementation out, though.  We want to make
  // sure we insert quickly into the GraphArray
  OpalTable* t = dynamic_cast(OpalTable*, ov.data());
  OpalGraphHash& ogh = dynamic_cast(OpalGraphHash&, t->implementation());


  if (*(lc.mem)++ != PY_EMPTY_DICT) p2error_("not the start of a dict");
  handleAPut_(&ov, lc); // in case PUT
    
  // Empty dictionary or single item
  if (*(lc.mem) != '(') {
    char* saved_mem = lc.mem; 
    bool bad_retry = false;
    try {
      // THIS IS A HACK to "retry":  we really probably want a stack
      loadSingleKeyValue_(ogh, lc);    
    } catch (const BadRetry& br) {
      bad_retry = true;
    }
    if (bad_retry || *lc.mem!=PY_SETITEM) {
      lc.mem = saved_mem;
      ov = OpalValue(OpalTable(OpalTableImpl_GRAPHHASH));
    } else {
      lc.mem++;
    }
    return;
  }

  // Once we've seen the '(', only a PY_SETITEM or PY_SETITEMS ends it
  lc.mem++;
  while (1) {
    char tag = *(lc.mem);
    if (tag==PY_SETITEM || tag==PY_SETITEMS) break;
    loadSingleKeyValue_(ogh, lc);
  }

  *(lc.mem)++;  // Reinstall memory into struct past PY_SETITEM(s)
}


inline void loadSingleArr_ (OpalGraphArray& oga, LoadContext_& lc)
{
  // Append an item, and get its reference out so we can set it
  OpalEdge new_item(new OpalValue());
  oga.append(new_item);
  OpalValue& item = *new_item;
  
  lc.handle = -999;
  P2LoadValue(item, lc);
}


inline void P2LoadList (OpalValue& ov, LoadContext_& lc)
{  
  // Shouldn't have to resize with Arr:  thety are Stingy Arrays,
  // so the elements, once in place, never move
  ov = OpalValue(OpalTable(OpalTableImpl_GRAPHARRAY)); 
  
  // Painful to get the implementation out, though.  We want to make
  // sure we insert quickly into the GraphArray
  OpalTable* t = dynamic_cast(OpalTable*, ov.data());
  OpalGraphArray& oga = dynamic_cast(OpalGraphArray&, t->implementation());
  

  if (*(lc.mem)++ != PY_EMPTY_LIST) p2error_("not the start of a list");
  handleAPut_(&ov, lc); 

  // Empty List if no ( to start a tuple!
  if (*(lc.mem) != '(') { 
    char* saved_mem = lc.mem;
    bool bad_retry = false;
    try {
      // THIS IS A HACK to "retry":  we really probably want a stack
      loadSingleArr_(oga, lc);
    } catch (const BadRetry& br) {
      bad_retry = true;
    }
    if (bad_retry || *lc.mem!=PY_APPEND) {
      lc.mem = saved_mem;
      ov = OpalValue(OpalTable(OpalTableImpl_GRAPHARRAY)); 
    } else {
      lc.mem++;
    }
    return;
  }  

  // Once we've seen the '(', only a PY_APPEND or PY_APPENDS end it
  lc.mem++;
  while (1) {
    char tag = *(lc.mem);
    if (tag==PY_APPEND || tag==PY_APPENDS) break;
    loadSingleArr_(oga, lc);
  }

  lc.mem++;// Reinstall memory into struct past PY_APPEND(s)
}

inline void loadComplex_ (OpalValue& ov, LoadContext_& lc,
			  bool saw_mem=false)
{
  complex_16 c16;
  loadc16_(c16, lc, saw_mem);
  ov=Opalize(Number(c16));
  handleAPut_(&ov, lc);
}


inline bool P2LoadValue (OpalValue& ov, LoadContext_& lc)
{
  char tag = *(lc.mem)++;
  switch(tag) {

  case PY_NEWTRUE:  ov = OpalValue(bool(1));  break; 
  case PY_NEWFALSE: ov = OpalValue(bool(0));  break; 
  case PY_NONE :    ov = OpalValue("None", OpalValueA_TEXT);  break; 

  case PY_BINGET : // Lookup via small integer
  case PY_LONG_BINGET : { // Lookup via integer
    lc.mem--;
    void* vp = handleAGet_(ov, lc, (string*)0);
    if     (vp==&ComplexPreambleAsString){ loadComplex_(ov,lc,true);break;}
    else if(vp==&ArrayPreambleAsString)  { loadArray_(ov,lc,true);  break;}
    else if(vp==&NumericPreambleAsString){ loadOpalNumeric_(ov,lc,true);break;}
    return true;  // Indicate we set ov from a cached copy
  }

    // Make sure the compressed ints come out as int_4s for compatibility
  case PY_BININT1:{int_u1 i1;LOAD_FROM_LITTLE_ENDIAN1(lc.mem,i1);int_4 i4=i1; ov=OpalValue(Number(i4));lc.mem+=1;break;}
  case PY_BININT2:{int_u2 i2;LOAD_FROM_LITTLE_ENDIAN2(lc.mem,i2);int_4 i4=i2; ov=OpalValue(Number(i4));lc.mem+=2;break;}
  case PY_BININT: {int_4 i4; LOAD_FROM_LITTLE_ENDIAN4(lc.mem,i4);ov=OpalValue(Number(i4));lc.mem+=4;break;}
    // ...but the floats always BIG ENDIAN ... ????
  case PY_BINFLOAT: {real_8 r8; LOAD_FROM_BIG_ENDIAN8(lc.mem, r8); ov=OpalValue(Number(r8)); lc.mem+=8;break;}

  case PY_LONG1 : {
    int_8 i8;
    int_u1 len=*(lc.mem)++; 
    if (len>9) p2error_("Can't support longs over 8/9 bytes");
    char buff[9] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    memcpy(buff, lc.mem, len);
    LOAD_FROM_LITTLE_ENDIAN8(buff, i8); 
    lc.mem+=len;
    if (len==9) { 
      ov = Opalize(Number(int_u8(i8)));
    } else { 
      ov = Opalize(Number(i8));
    }
    break;
  }


  case PY_SHORT_BINSTRING : 
  case PY_BINSTRING : { 
    lc.mem--; 
    int_u4 len = loadStringLength_(lc);
    string s(lc.mem, len); // Specialize string load for speed 
    lc.mem += len;
    ov=s; 
    handleAPut_(&ov, lc);
    break; 
  }

  case PY_EMPTY_DICT: { lc.mem--; P2LoadTable(ov, lc); break; }
  case PY_EMPTY_LIST: { lc.mem--; P2LoadList(ov, lc); break; }
 
  case PY_GLOBAL: { // This can be lots of things, almost certainlly just complex
    char start = *(lc.mem)--;
    if (start=='_')        { loadComplex_(ov, lc); break;}
    else if (start=='a')   { loadArray_(ov, lc);   break;}
    else if (start=='N')   { loadOpalNumeric_(ov, lc); break;}
    else p2error_("Unknown global");
  }

    // These two cases are if an older version of Python (2.2) 
    // is writing out integers and Longs as strings
  case 'L': { 
    if (*lc.mem=='-') {
      ov = OpalValue(Number(loadLONG_(lc))); break;
    } else {
      ov = OpalValue(Number(loadULONG_(lc))); break;
    }
  }
  case 'I':  ov = OpalValue(Number(loadINT_(lc))); break;
    
    // Would use LONG and INT but those symbols already defined!

    // When we are "trying" to see if we have a single
    // entry table or array
  case PY_APPEND: case PY_APPENDS: case PY_SETITEM: case PY_SETITEMS:
  case PY_STOP: throw BadRetry(); break;

  default:  p2error_("Unknown tag in python pickling protocol 2");
  }

  return false;
}




