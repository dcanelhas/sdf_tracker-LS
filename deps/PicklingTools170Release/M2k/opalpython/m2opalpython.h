#ifndef M2OPALPYTHON_H_

// These (mostly) inline functions bind OpalValues to Python Serialization.

#if defined(M2KLITE)
#  include "geoiostream.h"
#endif 

#include "m2opaltable.h"
#include "m2opalvaluet.h"
#include "m2opalgrapharray.h"
#include "m2table.h"

// This first part is for Python Depickling

inline OpalValue MakeNone () { return OpalValue(OpalValueA_TEXT); } 
inline OpalValue MakeBool (int b) { return OpalValue(bool(b)); }

// Make Opalvalue ints/floats from strings
inline OpalValue MakeInt4 (const char* ascii_int) 
{ return OpalValue(Number(UnStringize(ascii_int, int_4))); }
inline OpalValue MakeInt8 (const char* long_int) 
{ return OpalValue(Number(UnStringize(long_int, int_8))); }
inline OpalValue MakeIntu8 (const char* long_int) 
{ return OpalValue(Number(UnStringize(long_int, int_u8))); }

inline OpalValue MakeDouble (const char* ascii_float) 
{ return OpalValue(Number(UnStringize(ascii_float, real_8))); }
inline OpalValue MakeString (const char* ascii_string)
{ return OpalValue(string(ascii_string)); }
inline OpalValue MakeString (const char* ascii_string, int len)
{ return OpalValue(string(ascii_string, len)); }

inline int_4 GetInt4 (const OpalValue& val) { return UnOpalize(val, int_4); }
inline OpalValue PutInt4 (int_4 i) { return OpalValue(Number(i)); }

inline OpalValue MakeComplex (const OpalValue& real, const OpalValue& imag)
{ 
  real_8 re = UnOpalize(real, real_8); 
  real_8 im = UnOpalize(imag, real_8); 
  return OpalValue(Number(complex_16(re, im)));
}

// List operations
inline OpalValue MakeList ()
{ return OpalValue(new OpalTable(OpalTableImpl_GRAPHARRAY)); }
inline void ListAppend (OpalValue& l, const OpalValue& value)
{
  OpalTable* list_thing = dynamic_cast(OpalTable*, l.data());
  list_thing->append(value); 
}

// Table operations
inline OpalValue MakeTable ()
{ return OpalValue(new OpalTable(OpalTableImpl_GRAPHHASH)); }
inline void TableSet (OpalValue& table, 
		      const OpalValue& key, const OpalValue& value) 
{   
  OpalTable* dictionary = dynamic_cast(OpalTable*, table.data());
  dictionary->put(key.stringValue(), value);
}

// Tuple operations: The GraphHashM2 is not the best mapping to a
// Tuple, but nonetheless provides a distinction between lists and
// tuples.  Since tuples aren't used often, probably not a big deal.
inline OpalValue MakeTuple (bool compat)
{ 
  if (compat) return MakeList();
  return OpalValue(new OpalTable(OpalTableImpl_GRAPHM2NVPAIRS)); 
}
inline void TupleAppend (OpalValue& tuple, const OpalValue& value)
{   
  OpalTable* t = dynamic_cast(OpalTable*, tuple.data());
  t->append(value);
}
inline OpalValue TupleGet (const OpalValue& tuple, int key)
{
  OpalTable* t = dynamic_cast(OpalTable*, tuple.data());
  return t->get(key);
}
inline int TupleLength (const OpalValue& tuple) 
{
  OpalTable* t = dynamic_cast(OpalTable*, tuple.data());
  return t->length();
}

// OrderedDict operations
inline OpalValue MakeOrderedDict (bool compat)
{ return MakeTable(); } // TODO: Error message for no ordereddict?

inline void OrderedDictSet (OpalValue& table, 
			    const OpalValue& key, const OpalValue& value)
{ return TableSet(table, key, value); }

inline void* GetProxyHandle (const OpalValue& ov) 
{ return ov.data(); } // TODO: Is this ever even called?  IsProxy always false

// String
inline string GetString (const OpalValue& ov)
{ return ov.stringValue(); }

// Exceptions
inline void MakeException (const string& err_messg) 
{ throw MidasException(err_messg); }

// Warnings
inline void MakeWarning (const string& err_msg) 
{ 
  GlobalLog.warning(err_msg); 
  cerr << err_msg << endl; 
}

// Vector
OpalValue MakeVector (const string& typecode, size_t elements,
		      const char* mem, size_t bytes);

// These functions are for pickling


inline bool IsNone (const OpalValue& ov) 
{ return ov.type()==OpalValueA_TEXT; }

inline bool IsBool (const OpalValue& ov) 
{ return ov.type()==OpalValueA_BOOL; }

inline bool IsNumber (const OpalValue& ov) 
{ return ov.type()==OpalValueA_NUMBER;}

inline bool IsString (const OpalValue& ov)
{ return ov.type()==OpalValueA_STRING; }

inline bool IsVector (const OpalValue& ov)
{ return ov.type()==OpalValueA_VECTOR; }

inline bool IsTable (const OpalValue& ov)
{ 
  if (ov.type()!=OpalValueA_TABLE) return false;
  OpalTable* t = dynamic_cast(OpalTable*, ov.data());
  return (t->implementation().type()==OpalTableImpl_GRAPHHASH ||
	  t->implementation().type()==OpalTableImpl_NONE);
}

inline bool IsOrderedDict (const OpalValue& ov)
{ return false; }

inline bool IsTuple (const OpalValue& ov)
{   
  if (ov.type()!=OpalValueA_TABLE) return false;
  OpalTable* t = dynamic_cast(OpalTable*, ov.data());
  return (t->implementation().type()==OpalTableImpl_GRAPHM2NVPAIRS);
}

inline bool IsList (const OpalValue& ov)
{ 
  if (ov.type()!=OpalValueA_TABLE) return false;
  OpalTable* t = dynamic_cast(OpalTable*, ov.data());
  return (t->implementation().type()==OpalTableImpl_GRAPHARRAY);
}

inline bool IsProxy (const OpalValue& ov) { return false; } // TODO: OpalLink?
inline bool IsProxyTable (const OpalValue& ov) { return IsTable(ov); }
inline bool IsProxyOrderedDict (const OpalValue& ov) { return false; }
inline bool IsProxyTuple (const OpalValue& ov) { return IsTuple(ov); }
inline bool IsProxyList (const OpalValue& ov) { return IsList(ov); }
inline bool IsProxyVector (const OpalValue& ov) { return IsVector(ov); }

inline void Tablify (const OpalValue& src, OpalValue& dest)
{ throw MidasException("No OrderedDicts, so no need to Tablify them in M2k"); }


inline string TypeOf (const OpalValue& ov) { return Stringize(ov.type()); }

inline bool IsTrue (const OpalValue& ov) 
{ bool b = UnOpalize(ov, bool); return b; }


inline int VectorElements (const OpalValue& ov)
{ Vector v = ov.vector(); return v.elements(); }
inline OpalValue VectorGet (const OpalValue& ov, int ii)
{  Vector v = ov.vector(); return OpalValue(v[ii]); }

inline int ListElements (const OpalValue& ov)
{ 
  OpalTable* t = dynamic_cast(OpalTable*, ov.data());
  return t->surfaceEntries();
} 
inline OpalValue ListGet (const OpalValue& ov, int ii)
{  
  OpalTable* t = dynamic_cast(OpalTable*, ov.data());
  OpalGraphArray& oga = dynamic_cast(OpalGraphArray&, t->implementation());
  OpalEdge edge = oga.at(ii);
  return *edge->data();
}

class TableIterator {
 public:
  TableIterator (const OpalValue& ot) :
    it_(UnOpalize(ot, OpalTable))
  { }
  bool operator() () { return it_(); } 
  OpalValue key () const { return it_.key(); }
  OpalValue value () const { return it_.value(); }
 protected:
    OpalTableIterator it_;
}; // TableIterator
 
// Return the python typecode "l" or "d", "F" or "D" for this vector
inline string BestFitForVector (const OpalValue& ov)
{
  Vector v = ov.vector();
  switch(v.type()) {
  case M2_BYTE     : // int_1
  case M2_UBYTE    : // int_u1
  case M2_INTEGER  : // int_2
  case M2_UINTEGER : // int_u2
  case M2_LONG     : // int_4
  case M2_TIME     : // really wish we could be guaranteed an int_u8
  case M2_XLONG:
  case M2_UXLONG:
    return "l"; break;
  case M2_FLOAT    : // real_4
  case M2_DOUBLE   : // real_8
  case M2_DURATION : // a duration is implemented as a real_8
    return "d"; break;
  case M2_CX_FLOAT : // complex_8
    return "F";
  case M2_CX_DOUBLE: // complex_16
    return "D";
  }
  MakeException("Don't know how serialize Vector of type:"+
		Stringize(v.type()));
  return "";
}

inline void Proxyize (const OpalValue& ov) { }  // Does nothing ...

char ChooseNumber (const OpalValue& ov, string& arg1, string& arg2);
string BuildVector (const OpalValue& ov, const string& python_typecode);



#include "m2pythonpickler.h"

// Convert those unknown types to known Python types
void UnknownType (PythonPicklerA<OpalValue>& pickler, const OpalValue& ov);
string OBJToNumPyCode (const OpalValue& obj);

string BuildNumPyVector (const OpalValue& ov, const string& endian);

#define M2OPALPYTHON_H_
#endif // M2OPALPYTHON_H_
