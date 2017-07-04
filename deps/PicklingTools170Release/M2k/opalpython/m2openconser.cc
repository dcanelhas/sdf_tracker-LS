
#if defined(OC_FACTOR_INTO_H_AND_CC)
# include "m2openconser.h"
  // Explicit instantiations if needed
#endif

#include "m2opallink.h"
#include "m2opaltable.h"
#include "m2opalheader.h"
#include "m2opalgrapharray.h"
#include "m2opalgraphhash.h"
#include "m2eventdata.h"
#include "m2multivector.h"
#include "m2table.h"
#include "m2tableize.h"

#define VALCOPY(T,N) { memcpy(mem,&N,sizeof(T));mem+=sizeof(T); }

// This is an implementation class:  It allows us to track proxies so
// we don't serialize them twice.
struct OVDumpContext_ {
  OVDumpContext_ (char* start_mem ) : mem(start_mem) { }

  char* mem;  // Where we currently are in the buffer we are dumping into

  // Lookup table for looking up the markers:  When a proxy comes
  // in, we need to know if we have seen it before, and if so,
  // what proxy it refers to.  Note that we hold each proxy by
  // pointer because we are ASSUMING that nothing is changing the
  // table in question (and avoids extra locking)
  HashTableT<void*, int_4, 8> lookup_;
}; // OVDumpContext_

// Forwards
inline size_t BytesToSerialize (const OpalValue& ov, OVDumpContext_&);
inline void Serialize (const OpalValue& ov, OVDumpContext_&);

size_t BytesToSerialize (const string& s)
{ 
  size_t len = s.length();
  size_t bytes = 1 +((len<0xFFFFFFFF) ? sizeof(int_u4) : sizeof(int_u8)) + len;
  return bytes;
  // return 1 + sizeof(int_u4) + s.length(); 
} // 'a' tag, then length, then data

inline size_t BytesToSerializeTable (const OpalTable& t, OVDumpContext_& dc)
{
  // A 't'/'T' marker (actually single byte, not the full Val) starts, plus len
  // size_t bytes = 1 + sizeof(int_u4)/sizeof(int_u8);
  size_t len = t.surfaceEntries();
  size_t bytes = 1 +((len<0xFFFFFFFF) ? sizeof(int_u4) : sizeof(int_u8));

  // ... then key/value pairs.  When we look for a key and see a None
  // marker, then we know we are at the end of the table.
  OpalTableIterator ii(t);
  while (ii()) {
    const string& key = ii.key();
    bytes += BytesToSerialize(key);
    const OpalValue value = ii.value();
    bytes += BytesToSerialize(value,dc);
  }
  return bytes;
}


inline size_t BytesToSerializeList (const OpalTable& t, OVDumpContext_& dc)
{
  // An 'n'/'N' marker (actually single byte, not the full Val) starts,
  // the the subtype (a Z for Vals)
  // then the length ....
  size_t len = t.surfaceEntries();
  size_t bytes = 1 + 1 + ((len<0xFFFFFFFF) ? sizeof(int_u4) : sizeof(int_u8));
  // size_t bytes = 1 + 1 + sizeof(int_u4);

  // ... then n vals.
  const size_t surface_entries = t.surfaceEntries();

  // Get implementation as GraphArray
  const OpalGraphArray& oga = 
    dynamic_cast(const OpalGraphArray&, t.implementation());

  for (size_t ii=0; ii<surface_entries; ii++) {
    OpalEdge edge = oga.at(ii);
    OpalValueA* ovp = edge->data();
    OpalValue value(*ovp);
    bytes += BytesToSerialize(value, dc);
  }
  return bytes;
}

inline size_t BytesToSerialize (const OpalTable& t, OVDumpContext_& dc)
{
  OpalTableImpl_Type_e typer = t.implementation().type();
  switch (typer) {
  case OpalTableImpl_GRAPHHASH:
  case OpalTableImpl_NONE:
    return BytesToSerializeTable(t, dc);
  case OpalTableImpl_GRAPHARRAY:
      return BytesToSerializeList(t, dc);
  default: throw MidasException("Unknown how to serialize:"+Stringize(typer));
  }
  throw MidasException("Unknown Table type");
}

size_t BytesToSerialize (const Vector& v)
{
  // An 'n' marker (actually single byte, not the full Val) starts,
  // then the subtype of the array, then the length
  size_t len = v.elements();
  size_t bytes = 1 + 1 + ((len<0xFFFFFFFF) ? sizeof(int_u4) : sizeof(int_u8));
  // size_t bytes = 1 + 1 + sizeof(int_u4);

  /// then the bytes of the vector
  Numeric_e typer = v.type();
  bytes += ByteLength(typer)*v.elements();
  return bytes;
}

inline size_t BytesToSerialize (const Number& n)
{
  if (n.type()==M2_TIME) return 37;  // about how long a full stringized time
  else if (n.type()==M2_DURATION) return 24; // 16 bits of prec + e00 negs, etc
  else return 1 + ByteLength(n.type());  
}

inline size_t BytesToSerialize (const OpalValue& ov, OVDumpContext_& dc)
{
  switch(ov.type()) {

  case OpalValueA_NUMBER: return BytesToSerialize(ov.number());

  case OpalValueA_TABLE: { // Either a table or list
    OpalTable* tp = dynamic_cast(OpalTable*, ov.data());
    return BytesToSerialize(*tp, dc);
    break;
  }

  case OpalValueA_STRING : return BytesToSerialize(ov.stringValue()); break;
  case OpalValueA_VECTOR:  return BytesToSerialize(ov.vector()); break;

  case OpalValueA_TEXT : return 1; // The None value

  case OpalValueA_MULTIVECTOR: {
    // return BytesToSerializeList(MultiVectorToList(ov),dc); break;
    MultiVector mv = UnOpalize(ov, MultiVector);
    size_t len = mv.length();
    //size_t bytes = 1 + 1 + 4; // 'n' 'Z' len
    size_t bytes = 1 + 1 + ((len<0xFFFFFFFF) ? sizeof(int_u4) : sizeof(int_u8)); // 'n' 'Z' len
    for (size_t ii=0; ii<len; ii++) {
      bytes += BytesToSerialize(mv[ii]);
    }
    return bytes; 
    break;
  }

  case OpalValueA_HEADER: 
    return BytesToSerializeTable(OpalHeaderToTable(ov), dc); break;

  case OpalValueA_LINK: 
    return BytesToSerialize(OpalLinkToString(ov)); break;

  case OpalValueA_EVENTDATA: 
    return BytesToSerializeTable(EventDataToTable(ov), dc); break;

  case OpalValueA_TIMEPACKET: 
    return BytesToSerializeTable(TimePacketToTable(ov),dc); break;

  case OpalValueA_USERDEFINED:return BytesToSerialize(ov.stringValue()); break;
  
  default:// cerr << "Found unknown type in dump: turning into string" << endl;
    return BytesToSerialize(ov.stringValue()); break;
  }
  return 0;
}

char* Serialize (const string& s, char* mem)
{
  size_t len = s.length();
  if (len >= 0xFFFFFFFF) {
    *mem++ = 'A'; // Always need tag
    // Strings: 'a', int_u8 length, (length) bytes of data
    const int_u8 len=s.length();
    VALCOPY(int_u8, len);
  } else {
    *mem++ = 'a'; // Always need tag
    // Strings: 'a', int_u4 length, (length) bytes of data
    const int_u4 len=s.length();
    VALCOPY(int_u4, len);
  }
  memcpy(mem, s.c_str(), len);
  mem += len;
  return mem;
}


#define M2OCNUMBER(N, T, TAG) { T temp = N.operator T(); *mem++=TAG; VALCOPY(T, temp); }
inline char* Serialize (const Number& n, char* mem)
{
  switch (n.type()) {
  case M2_BYTE      : M2OCNUMBER(n, int_1, 's'); break;
  case M2_UBYTE     : M2OCNUMBER(n, int_u1,'S'); break;
  case M2_INTEGER   : M2OCNUMBER(n, int_2, 'i'); break;
  case M2_UINTEGER  : M2OCNUMBER(n, int_u2,'I'); break;
  case M2_LONG      : M2OCNUMBER(n, int_4, 'l'); break;
  case M2_ULONG     : M2OCNUMBER(n, int_u4,'L'); break;
  case M2_XLONG     : M2OCNUMBER(n, int_8, 'x'); break;
  case M2_UXLONG    : M2OCNUMBER(n, int_u8,'X'); break;
  case M2_FLOAT     : M2OCNUMBER(n, real_4,'f'); break;
  case M2_DOUBLE    : M2OCNUMBER(n, real_8,'d'); break;
  case M2_CX_FLOAT  : M2OCNUMBER(n, complex_8,'F'); break;
  case M2_CX_DOUBLE : M2OCNUMBER(n, complex_16,'D'); break;

  case M2_TIME: { // as int_u8 or string?  By default Python Pickling 0 and 2
                  // do as M2Time and M2Duration as strings.
    M2Time t = n.operator M2Time();
    string t_as_s = t.stringValue();
    mem = Serialize(t_as_s, mem);
    break;
    //  M2OCNUMBER(n, int_u8, 'X'); break;  // If we want as int_u8
  }
  case M2_DURATION: { // as real_8 or string?
    M2Duration d = n.operator M2Duration();
    string d_as_s = d.stringValue();
    mem = Serialize(d_as_s, mem);
    break;
    // M2OCNUMBER(n, real_8, 'd'); break;   // If we want as real_8
  }

  default: {
    //cerr << string("Don't know how to serialize numeric type:"+
    //             Stringize(n.type())+". Converting to string.\n");
    throw MidasException("Don't know how to serialize numeric type:"+
			 Stringize(n.type())+".");

  }
  }
  return mem;
}

inline void SerializeTable (const OpalTable& t, OVDumpContext_& dc)
{
  char*& mem = dc.mem;

  //  Tables: 't', int_u4 length, (length) Key/Value pairs
  size_t len = t.surfaceEntries();
  if (len >= 0xFFFFFFFF) {
    *mem++ = 'T';
    int_u8 l8 = len;
    VALCOPY(int_u8, l8);
  } else {
    *mem++ = 't';
    int_u4 l4 = len;
    VALCOPY(int_u4, l4);
  }

  OpalTableIterator ii(t);
  while (ii()) {
    const string& key = ii.key();
    mem = Serialize(key, mem);

    const OpalValue value = ii.value();
    Serialize(value, dc);
  }
}


inline void SerializeList (const OpalTable& t, OVDumpContext_& dc)
{
  char*& mem = dc.mem;

  // Arrays: 'n', subtype, int_u4 length, (length) vals
  size_t len = t.surfaceEntries();
  if (len >= 0xFFFFFFFF) {
    *mem++ = 'N'; // Always need tag
    *mem++ = 'Z'; // subtype
    int_u8 l4 = len;
    VALCOPY(int_u8, l4);
  } else {
    *mem++ = 'n'; // Always need tag
    *mem++ = 'Z'; // subtype
    int_u4 l4 = len;
    VALCOPY(int_u4, l4);
  }

  // Get implementation as GraphArray
  const OpalGraphArray& oga = 
    dynamic_cast(const OpalGraphArray&, t.implementation());

  for (size_t ii=0; ii<len; ii++) {
    OpalEdge edge = oga.at(ii);
    OpalValueA* ovp = edge->data();
    OpalValue value(*ovp);
    Serialize(value, dc);
  }
}
/*
inline void SerializeList (const OpalTable& t, OVDumpContext_& dc)
{
  char*& mem = dc.mem;

  // Arrays: 'n', subtype, int_u4 length, (length) vals
  *mem++ = 'n'; // Always need tag
  *mem++ = 'Z'; // subtype

  int_u4 len = t.surfaceEntries();
  VALCOPY(int_u4, len);

  // Get implementation as GraphArray
  const OpalGraphArray& oga = 
    dynamic_cast(const OpalGraphArray&, t.implementation());

  for (int ii=0; ii<len; ii++) {
    OpalEdge edge = oga.at(ii);
    OpalValueA* ovp = edge->data();
    OpalValue value(*ovp);
    Serialize(value, dc);
  }
}
*/

inline void Serialize (const OpalTable& t, OVDumpContext_& dc)
{
  OpalTableImpl_Type_e typer = t.implementation().type();
  switch (typer) {
  case OpalTableImpl_GRAPHHASH:
  case OpalTableImpl_NONE:
    SerializeTable(t, dc);
    return;
  case OpalTableImpl_GRAPHARRAY:
    SerializeList(t, dc);
    return;
  default: throw MidasException("Unknown how to serialize:"+Stringize(typer));
  }
}


#define M2OCVECTOR(T, TAG) *mem++ = TAG
char* Serialize (const Vector& v, char* mem)
{
  // Arrays: 'n', subtype, int_u4 length, (length) bytes
  size_t len = v.elements();
  if (len >= 0xFFFFFFFF) {
    *mem++ = 'N'; // Always need tag
  } else {
    *mem++ = 'n'; // Always need tag
  }
  Numeric_e typer = v.type();

  switch (typer) {
  case M2_BYTE      : M2OCVECTOR(int_1, 's'); break;
  case M2_UBYTE     : M2OCVECTOR(int_u1,'S'); break;
  case M2_INTEGER   : M2OCVECTOR(int_2, 'i'); break;
  case M2_UINTEGER  : M2OCVECTOR(int_u2,'I'); break;
  case M2_LONG      : M2OCVECTOR(int_4, 'l'); break;
  case M2_ULONG     : M2OCVECTOR(int_u4,'L'); break;
  case M2_XLONG     : M2OCVECTOR(int_8, 'x'); break;
  case M2_UXLONG    : M2OCVECTOR(int_u8,'X'); break;
  case M2_FLOAT     : M2OCVECTOR(real_4,'f'); break;
  case M2_DOUBLE    : M2OCVECTOR(real_8,'d'); break;
  case M2_CX_FLOAT  : M2OCVECTOR(complex_8,'F'); break;
  case M2_CX_DOUBLE : M2OCVECTOR(complex_16,'D'); break;
  case M2_TIME      : M2OCVECTOR(int_u8,'X'); break; // Turn into int_u8
  case M2_DURATION  : M2OCVECTOR(real_8,'d'); break; // Turn into real_8
  default: // cerr << "Warning: unknown type: " << endl;
    throw MidasException("Unknown number type"+Stringize(typer));
  }
  
  // Len, depending on how big
  if (len >= 0xFFFFFFFF) {
    int_u8 l8 = len;
    VALCOPY(int_u8, l8);  
  } else {
    int_u4 l4 = len;
    VALCOPY(int_u4, l4);  
  }

  const size_t byte_len = ByteLength(typer)*len;
  memcpy(mem, v.readData(), byte_len);
  mem += byte_len;

  return mem;
}

/*
#define M2OCVECTOR(T, TAG) *mem++ = TAG
char* Serialize (const Vector& v, char* mem)
{


  // Arrays: 'n', subtype, int_u4 length, (length) bytes
  *mem++ = 'n'; // Always need tag

  const int len   = v.elements();
  Numeric_e typer = v.type();

  switch (typer) {
  case M2_BYTE      : M2OCVECTOR(int_1, 's'); break;
  case M2_UBYTE     : M2OCVECTOR(int_u1,'S'); break;
  case M2_INTEGER   : M2OCVECTOR(int_2, 'i'); break;
  case M2_UINTEGER  : M2OCVECTOR(int_u2,'I'); break;
  case M2_LONG      : M2OCVECTOR(int_4, 'l'); break;
  case M2_ULONG     : M2OCVECTOR(int_u4,'L'); break;
  case M2_XLONG     : M2OCVECTOR(int_8, 'x'); break;
  case M2_UXLONG    : M2OCVECTOR(int_u8,'X'); break;
  case M2_FLOAT     : M2OCVECTOR(real_4,'f'); break;
  case M2_DOUBLE    : M2OCVECTOR(real_8,'d'); break;
  case M2_CX_FLOAT  : M2OCVECTOR(complex_8,'F'); break;
  case M2_CX_DOUBLE : M2OCVECTOR(complex_16,'D'); break;
  case M2_TIME      : M2OCVECTOR(int_u8,'X'); break; // Turn into int_u8
  case M2_DURATION  : M2OCVECTOR(real_8,'d'); break; // Turn into real_8
  default: // cerr << "Warning: unknown type: " << endl;
    throw MidasException("Unknown number type"+Stringize(typer));
  }
  VALCOPY(int_u4, len);  // subtype

  const int_u4 byte_len = ByteLength(typer)*len;
  memcpy(mem, v.readData(), byte_len);
  mem += byte_len;

  return mem;
}
*/

inline void Serialize (const OpalValue& ov, OVDumpContext_& dc)
{
  char*& mem = dc.mem;

  switch(ov.type()) {
  case OpalValueA_BOOL:{*mem++='b';bool v=UnOpalize(ov,bool); *mem++=v;return;}
  case OpalValueA_NUMBER:{mem=Serialize(UnOpalize(ov,Number), mem); return;}

  case OpalValueA_TABLE: { // Either a table or list
    OpalTable* tp = dynamic_cast(OpalTable*, ov.data());
    Serialize(*tp, dc);
    return;
    break;
  }

  case OpalValueA_STRING : mem = Serialize(ov.stringValue(), mem); return;
  case OpalValueA_VECTOR:  mem = Serialize(ov.vector(), mem); return;

  case OpalValueA_TEXT : *mem++ = 'Z'; return;
  case OpalValueA_MULTIVECTOR: {
    // Serialize as List 
    MultiVector mv = UnOpalize(ov, MultiVector);
    size_t len = mv.length();
    size_t bytes = 1 + 1 + 4; // 'n' 'Z' len
    *mem++ = 'n'; 
    *mem++ = 'Z';
    if (len >= 0xFFFFFFFF) {
      int_u8 l8 = mv.length();
      VALCOPY(int_u8, l8);
    } else {
      int_u4 l4 = mv.length();
      VALCOPY(int_u4, l4);
    }
    for (size_t ii=0; ii<len; ii++) {
      mem = Serialize(mv[ii], mem);
    }
    return;
    break;
  }

  case OpalValueA_HEADER: SerializeTable(OpalHeaderToTable(ov), dc); return;

  case OpalValueA_LINK: mem=Serialize(OpalLinkToString(ov), mem); return;

  case OpalValueA_EVENTDATA: Serialize(EventDataToTable(ov), dc); return;

  case OpalValueA_TIMEPACKET: Serialize(TimePacketToTable(ov), dc); return;

  case OpalValueA_USERDEFINED: Serialize(ov.stringValue(), mem); return;

  default:// cerr << "Found unknown type in dump: turning into string" << endl;
    mem = Serialize(ov.stringValue(), mem); return;
  }
  throw MidasException("Unknown type in dump");
}

// These are really just for backwards compatibility: We really should
// be supporting the "Dump(..OCDumpContext)" iterface, but backwards
// compatibility is king.
size_t BytesToSerialize (const OpalValue& ov) 
{ OVDumpContext_ dc(0); return BytesToSerialize(ov, dc); }
size_t BytesToSerialize (const OpalTable& t) 
{ OVDumpContext_ dc(0); return BytesToSerialize(t, dc); }

char* Serialize (const OpalValue& ov, char* mem)
{ OVDumpContext_ dc(mem); Serialize(ov, dc); return dc.mem; }
char* Serialize (const OpalTable& t, char* mem)
{ OVDumpContext_ dc(mem); Serialize(t, dc); return dc.mem; }


/////////////////////////// Deserialize

// Helper class to keep track of all the Proxy's we've seen so we don't have
// to unserialize again
struct OVLoadContext_ {
  OVLoadContext_ (char* start_mem) : mem(start_mem) { }

  char* mem; // Where we are in the buffer
  // When we see a marker, see if we have already deserialized it.  If
  // we see it, there is some Proxys already out there
  HashTableT<int_4, OpalValue, 8> lookup_;
  
}; // OVLoadContext_

#define VALDECOPY(T,N) { memcpy(&N,mem,sizeof(T)); mem+=sizeof(T); }

// Forward
inline void DeserializeProxy (OpalValue& v, OVLoadContext_& lc);
inline void Deserialize (OpalValue& ov, OVLoadContext_& lc);


template <class INT>
inline void DeserializeTable (INT len, OpalValue& ov, OVLoadContext_& lc)
{
  char*& mem=lc.mem;

  // Already saw 't' or 'T'  

  // Get the number of key-value entries
  VALDECOPY(INT, len);

  // Initialize into so always start with empy table.  Shouldn't have
  // to worry about resizing: the table is implemented as a hash table
  // where the nodes don't move or resize.
  ov = OpalValue(OpalTable(OpalTableImpl_GRAPHHASH));
  
  // Painful to get the implementation out, though.  We want to make
  // sure we insert quickly into the GraphArray
  OpalTable* tp = dynamic_cast(OpalTable*, ov.data());
  OpalGraphHash& ogh = dynamic_cast(OpalGraphHash&, tp->implementation());
  
  // The next n (key, value) pairs
  for (INT ii=0; ii<len; ii++) {
    // For OpenContainers, key is NOT always a string!  We deserialize
    // and turn the key into a string for Opal purpose.
    // Get the key, always a string: Do it inplace to avoid Opalvalue
    // conversion
    OpalValue okey;
    Deserialize(okey, lc);
    string key = okey.stringValue();

    // Get the value: First, put empty in table as inexpensively as
    // possible then get a ref to it.  
    OpalEdge new_value(new OpalValue());
    ogh.put(key, new_value);
    OpalValue& value=*new_value;

    Deserialize(value, lc);
  }
}

template <class INT>
inline void DeserializeList (OpalValue& ov, INT len, OVLoadContext_& lc)
{
  // Assertion: Already seen 'n' and 'Z' and len

  // Shouldn't have to resize with Arr:  they are Stingy Arrays,
  // so the elements, once in place, never move
  ov = OpalValue(OpalTable(OpalTableImpl_GRAPHARRAY));

  // Painful to get the implementation out, though.  We want to make
  // sure we insert quickly into the GraphArray
  OpalTable* tp = dynamic_cast(OpalTable*, ov.data());
  OpalGraphArray& oga = dynamic_cast(OpalGraphArray&, tp->implementation());

  for (INT ii=0; ii<len; ii++) {
    // Append an item, and get its reference out so we can set it
    OpalEdge new_item(new OpalValue());
    oga.append(new_item);
    OpalValue& item = *new_item;
    
    Deserialize(item, lc);
  }
}

inline void DeserializeArrays (OpalValue& ov, OVLoadContext_& lc, char tag)
{ 
  char*& mem=lc.mem;
  // Arrays: 'n' or 'N', then subtype, then int_u4 or int_u8 len, then number of
  // values following
  size_t len;
  char subtype = *mem++;
  if (tag=='n') {
    int_u4 l4; 
    VALDECOPY(int_u4, l4); 
    len = l4;
  } else {
    int_u8 l8; 
    VALDECOPY(int_u8, l8); 
    len = l8;
  }
  // cerr << "In DeserializeArray: len = " << len << endl;

  Numeric_e m2ktag;
  switch(subtype) {
  case 's': m2ktag=M2_BYTE; break; 
  case 'S': m2ktag=M2_UBYTE; break; 
  case 'i': m2ktag=M2_INTEGER; break; 
  case 'I': m2ktag=M2_UINTEGER; break; 
  case 'l': m2ktag=M2_LONG; break; 
  case 'L': m2ktag=M2_ULONG; break; 
  case 'x': m2ktag=M2_XLONG; break; 
  case 'X': m2ktag=M2_UXLONG; break; 
  case 'b': m2ktag=M2_UBYTE; break; ///TODO: ???? is there a vector of bool? 
  case 'f': m2ktag=M2_FLOAT; break; 
  case 'd': m2ktag=M2_DOUBLE; break; 
  case 'F': m2ktag=M2_CX_FLOAT; break; 
  case 'D': m2ktag=M2_CX_DOUBLE; break; 

  case 'a': 
  case 'A': throw MidasException("Can't have arrays of string");
  case 't': 
  case 'T': throw MidasException("Can't have arrays of Tabs");
  case 'n': 
  case 'N': throw MidasException("Can't have arrays of arrays");
  case 'Z': return DeserializeList(ov, len, lc); break;
  default: throw MidasException("DeserializeArray:"+Stringize(subtype));
  }
  // Assertion: Bit-blittable Array type if here
  size_t bytes = ByteLength(m2ktag)*len;
  Vector result(m2ktag, len);
  // cerr << "BYTES to deserializArrays" << bytes << endl; 
  memcpy(result.writeData(), mem, bytes);
  mem += bytes;

  // cerr << "Final Vector len" << result.elements() << endl;

  ov = Opalize(result); // Bind back!
}


template <class INT>
inline void DeserializeString (INT len, OpalValue& ov, OVLoadContext_& lc)
{ 
  char*& mem=lc.mem;

  // Strs: 'a' then int_u4 len, then len bytes
  VALDECOPY(INT, len);
  string s(mem, len);
  mem += len;
  ov = s;
} 


#define M2OCSMALLNUMB(T) { T tmp; memcpy(&tmp, mem, sizeof(T)); mem+=sizeof(T); ov=Opalize(Number(tmp)); }
inline void Deserialize (OpalValue& ov, OVLoadContext_& lc)
{
  char*& mem = lc.mem;

  char tag = *mem++; // Grab tag
  if (tag=='P') { 
    DeserializeProxy(ov, lc); 
    return;
  }
  switch(tag) {
  case 's': M2OCSMALLNUMB(int_1);  break;
  case 'S': M2OCSMALLNUMB(int_u1); break;
  case 'i': M2OCSMALLNUMB(int_2);  break;
  case 'I': M2OCSMALLNUMB(int_u2); break;
  case 'l': M2OCSMALLNUMB(int_4);  break;
  case 'L': M2OCSMALLNUMB(int_u4); break;
  case 'x': M2OCSMALLNUMB(int_8);  break;
  case 'X': M2OCSMALLNUMB(int_u8); break;
  case 'b': { bool b = *mem++; ov=Opalize(b); break; }
  case 'f': M2OCSMALLNUMB(real_4); break;
  case 'd': M2OCSMALLNUMB(real_8); break;
  case 'F': M2OCSMALLNUMB(complex_8); break;
  case 'D': M2OCSMALLNUMB(complex_16); break;

  case 'P': // already handled above 

  case 'a': { int_u4 len=0; DeserializeString(len, ov, lc); break; }
  case 'A': { int_u8 len=0; DeserializeString(len, ov, lc); break; }
  case 't': { int_u4 len=0; DeserializeTable(len, ov, lc); break; }
  case 'T': { int_u8 len=0; DeserializeTable(len, ov, lc); break; }
  case 'n': 
  case 'N': DeserializeArrays(ov, lc, tag); break;
  case 'Z': ov=OpalValue("None", OpalValueA_TEXT); break;  // Already got 'Z'
  default: throw MidasException("Deserialize:"+Stringize(tag));
  }
}


inline void DeserializeProxy (OpalValue& ov, OVLoadContext_& lc)
{
  char*& mem = lc.mem;  

  // Already seen the tag
  // if (*mem++!='P') throw MidasException("load:not starting Proxy");
  int_4 marker;
  VALDECOPY(int_4, marker);

  bool already_seen = lc.lookup_.contains(marker);
  if (already_seen) {
    ov = lc.lookup_[marker];     // There, just attach to new Proxy
    return;
  }

  // Assertion: NOT AVAILABLE! The first time we have deserialized
  // this proxy, so we have to completely pull it out.  First,
  // Deserialize the rest of the preamble
  bool adopt, lock, shm;
  VALDECOPY(bool, adopt);
  VALDECOPY(bool, lock);
  VALDECOPY(bool, shm);  // TODO; do something with this?

  // Now, Deserialize the full body as normal, then turn into proxy in
  // O(1) time
  Deserialize(ov, lc);   
  //v.Proxyize(adopt, lock);  // TODO: Maybe use OpalLink for proxy?

  // Add marker back in
  lc.lookup_[marker] = ov;
}



// Backwards compatibility
char* Deserialize (OpalValue& ov, char* mem)
{
  OVLoadContext_ lc(mem); 
  Deserialize(ov, lc);
  return lc.mem;
}
