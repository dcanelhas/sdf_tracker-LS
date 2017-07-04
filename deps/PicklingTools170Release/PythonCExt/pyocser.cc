
#include "pyocser.h"


#include "ocloaddumpcontext.h"
#include "psuedonumeric.h"

#if defined(OC_FORCE_NAMESPACE)
using namespace OC;
#endif

// Used so module compiles on more recent and older Pythons
// in 32 vs. 64
#if PY_VERSION_HEX < 0x02050000 && !defined(PY_SSIZE_T_MIN)
typedef int Py_ssize_t;
#define PY_SSIZE_T_MAX INT_MAX
#define PY_SSIZE_T_MIN INT_MIN
#define PYOC_FORMAT 'n'
#else
#define PYOC_FORMAT 'l' 
#endif


// Forwards
size_t OCPyBytesToSerialize (PyObject* po, OCPyDumpContext_& dc); 
size_t OCPyBytesToSerializeSeq (PyObject *po, OCPyDumpContext_& dc);
size_t OCPyBytesToSerializeTuple (PyObject *po,OCPyDumpContext_& dc);
size_t OCPyBytesToSerializeDict (PyObject* po,  OCPyDumpContext_& dc);
size_t OCPyBytesToSerializeODict (PyObject *po, OCPyDumpContext_& dc);
size_t OCPyBytesToSerializeFloat (PyObject *po, OCPyDumpContext_& dc);
size_t OCPyBytesToSerializeInt (PyObject *po, OCPyDumpContext_& dc);
size_t OCPyBytesToSerializeLong (PyObject *po, OCPyDumpContext_& dc);
size_t OCPyBytesToSerializeComplex (PyObject *po, OCPyDumpContext_& dc);
size_t OCPyBytesToSerializeBool (PyObject *po, OCPyDumpContext_& dc);
size_t OCPyBytesToSerializeNone (PyObject *po, OCPyDumpContext_& dc);
size_t OCPyBytesToSerializeString (PyObject *po, OCPyDumpContext_& dc);

size_t OCPyBytesToSerializeNumPyArray (PyObject *po, OCPyDumpContext_& dc);
size_t OCPyBytesToSerializeNumericArray (PyObject *po, OCPyDumpContext_& dc);

// bytes helper: returns true if proxy already, false otherwise.
// Fills in bytes with the amount for the 'P' and marker
bool OCPyBytesToSerializeProxy (PyObject *po, OCPyDumpContext_& dc, 
				size_t* bytes);

// Main routine to dispatch to all OCPyBytesToSerialize with OCDumpContext
size_t OCPyBytesToSerialize (PyObject *po, bool compat)
{
  OCPyDumpContext_ dc(0, compat);
  return OCPyBytesToSerialize(po, dc);
}

// Serialize
void OCPySerializeSeq (PyObject *po, OCPyDumpContext_& dc,
		       char small_tag='n', char big_tag='N');
void OCPySerializeTuple (PyObject *po, OCPyDumpContext_& dc);
void OCPySerializeDict (PyObject *po, OCPyDumpContext_& dc,
			char small_tag, char big_tag);
void OCPySerializeDict (PyObject *po, OCPyDumpContext_& dc);
void OCPySerializeODict (PyObject *po, OCPyDumpContext_& dc);
void OCPySerializeInt (PyObject *po, OCPyDumpContext_& dc);
void OCPySerializeFloat (PyObject *po, OCPyDumpContext_& dc);
void OCPySerializeLong (PyObject *po, OCPyDumpContext_& dc);
void OCPySerializeComplex (PyObject *po, OCPyDumpContext_& dc);
void OCPySerializeBool (PyObject *po, OCPyDumpContext_& dc);
void OCPySerializeNone (PyObject *po, OCPyDumpContext_& dc);
void OCPySerializeString (PyObject *po, OCPyDumpContext_& dc);

void OCPySerializeNumPyArray (PyObject *po, OCPyDumpContext_& dc);
void OCPySerializeNumericArray (PyObject *po, OCPyDumpContext_& dc);

// Serialize helper for noting Proxies: returns true if this was a
// proxy and all was done.  Otherwise, returns false.
// Note that this puts the 'P' and mark down regardless.
bool OCPySerializeProxy (PyObject *po, OCPyDumpContext_& lc);

// Deserialize
PyObject* OCPyDeserialize (OCPyLoadContext_& lc);


/*
// Main routine to dispatch to all OCPySerializeXXX with OCDumpContext
void OCPySerialize (PyObject *po, char *mem, bool compat)
{
  OCPyDumpContext_ dc(mem, compat);
  OCPySerialize(po, dc);
  return dc.mem;
}
*/

/////////////////////////////////////////////////////////////////////

// Generic sequence: this works for Lists and other Python sequences
// such as tuples, but we have to mark what kind of sequence it is:
// by default it becomes a List (but it can be a tuple with 'u''U')
size_t OCPyBytesToSerializeSeq (PyObject *po, OCPyDumpContext_& dc)
{
  size_t bytes = 0;
  if (OCPyBytesToSerializeProxy(po, dc, &bytes)) return bytes;

  // A 'n'/'N' marker, 'Z' starts, plus len
  const size_t len = PySequence_Size(po);
  bytes += 1+1+((len<=0xFFFFFFFF) ? sizeof(int_u4) : sizeof(int_u8));

  for (size_t ii=0; ii<len; ii++) {
    PyObject *pyitem = PySequence_GetItem(po, ii);
    bytes += OCPyBytesToSerialize(pyitem, dc);
  }

  return bytes;
}

void OCPySerializeSeq (PyObject *po, OCPyDumpContext_& dc,
		       char small_tag, char big_tag) // n, N by default
{
  if (OCPySerializeProxy(po, dc)) return;

  char*& mem = dc.mem;
  
  // Arrays: 'n'/'N', subtype, int_u4/int_u8 length, (length) vals
  size_t len = PySequence_Size(po);
  if (len > 0xFFFFFFFF) {
    *mem++ = big_tag;  // Always need tag
    *mem++ = 'Z'; // subtype
    int_u8 l = len;
    VALCOPY(int_u8, l);
  } else {
    *mem++ = small_tag; // Always need tag
    *mem++ = 'Z'; // subtype
    int_u4 l = len;
    VALCOPY(int_u4, l);
  } 

  for (size_t ii=0; ii<len; ii++) {
    PyObject *pyitem = PySequence_GetItem(po, ii);
    OCPySerialize(pyitem, dc);
  }
}


// Tuple: very similar to a List
size_t OCPyBytesToSerializeTuple (PyObject *po, OCPyDumpContext_& dc)
{ return OCPyBytesToSerializeSeq(po, dc); }

void OCPySerializeTuple (PyObject *po, OCPyDumpContext_& dc)
{ 
  if (dc.compat_) {
    return OCPySerializeSeq(po, dc);
  } else {
    return OCPySerializeSeq(po, dc, 'u', 'U');
  }
}


// Dictionary
size_t OCPyBytesToSerializeDict (PyObject *po, OCPyDumpContext_& dc)
{
  size_t bytes = 0;
  if (OCPyBytesToSerializeProxy(po, dc, &bytes)) return bytes;

  // A 't'/'T' marker (actually single byte, not the full Val) starts, plus len
  const size_t len = PyObject_Length(po);
  bytes += 1 + ((len<=0xFFFFFFFF) ? sizeof(int_u4) : sizeof(int_u8));

  // Python table iteration
  PyObject *pykey, *pyvalue;
  Py_ssize_t pos = 0;
  while (PyDict_Next(po, &pos, &pykey, &pyvalue)) {
    bytes += OCPyBytesToSerialize(pykey,dc) + OCPyBytesToSerialize(pyvalue,dc);
  }
  return bytes;
}

void OCPySerializeDict (PyObject *po, OCPyDumpContext_& dc)
{ OCPySerializeDict(po, dc, 't', 'T');  }

void OCPySerializeDict (PyObject *po, OCPyDumpContext_& dc,
			char small_tag, char big_tag) // 't' 'T' defaults
{
  if (OCPySerializeProxy(po, dc)) return;

  char*& mem = dc.mem;

  // Tables: 't'/'T', int_u4/int_u8 length, (length) Key/Value pairs
  const size_t len = PyObject_Length(po);
  if (len > 0xFFFFFFFF) {
    *mem++ = big_tag;  // Always need tag
    int_u8 l = len;
    VALCOPY(int_u8, l);
  } else {
    *mem++ = small_tag; // Always need tag
    int_u4 l = len;
    VALCOPY(int_u4, l);
  }

  // Python table iteration
  PyObject *pykey, *pyvalue;
  Py_ssize_t pos = 0;
  while (PyDict_Next(po, &pos, &pykey, &pyvalue)) {
    OCPySerialize(pykey, dc);
    OCPySerialize(pyvalue, dc);
  }
}

// Ordered Dictionary
size_t OCPyBytesToSerializeODict (PyObject *po, OCPyDumpContext_& dc)
{ return OCPyBytesToSerializeDict(po, dc); }

void OCPySerializeODict (PyObject *po, OCPyDumpContext_& dc)
{ OCPySerializeDict(po, dc, 'o', 'O'); }

// int
size_t OCPyBytesToSerializeInt (PyObject *po, OCPyDumpContext_& dc)
{ 
  Py_ssize_t pyvalue = PyInt_AsLong(po);
  size_t bytes = 1;  // for tag
  if (pyvalue < 0) {
    int_8 value = pyvalue;
    if (value >= -128) bytes += 1;
    else if (value >= -32768) bytes += 2;
    else if (value >= -2147483648LL) bytes += 4;
    else bytes += 8;
  } else {
    int_u8 value = pyvalue;
    if (value <= 255) bytes += 1;
    else if (value <= 65535) bytes += 2;
    else if (value <= 4294967295ULL) bytes += 4;
    else bytes += 8;
  }
  return bytes;
}

void OCPySerializeInt (PyObject *po, OCPyDumpContext_& dc)
{
  char*& mem = dc.mem;

  Py_ssize_t pyvalue = PyInt_AsLong(po);
  if (pyvalue < 0) {
    int_8 value = pyvalue;
    if (value >= -128) 
      { *mem++ = 's'; int_1 tmp=value; VALCOPY(int_1, tmp); }
    else if (value >= -32768) 
      { *mem++ = 'i'; int_2 tmp=value; VALCOPY(int_2, tmp); }
    else if (value >= -2147483648LL) 
      { *mem++ = 'l'; int_4 tmp=value; VALCOPY(int_4, tmp); }
    else 
      { *mem++ = 'x'; int_8 tmp=value; VALCOPY(int_8, tmp); }
  } else {
    int_u8 value = pyvalue;
    if (value <= 255)
      { *mem++ = 'S'; int_u1 tmp=value; VALCOPY(int_u1, tmp); }
    else if (value <= 65535)
      { *mem++ = 'I'; int_u2 tmp=value; VALCOPY(int_u2, tmp); }
    else if (value <= 4294967295ULL)
      { *mem++ = 'L'; int_u4 tmp=value; VALCOPY(int_u4, tmp); }
    else 
      { *mem++ = 'X'; int_u8 tmp=value; VALCOPY(int_u8, tmp); }
  }
}

// float
size_t OCPyBytesToSerializeFloat (PyObject *po, OCPyDumpContext_& dc)
{ return 1 + sizeof(double); } 

void OCPySerializeFloat (PyObject *po, OCPyDumpContext_& dc)
{
  char*& mem = dc.mem;
  *mem++ = 'd';
  double d = PyFloat_AsDouble(po);
  VALCOPY(double, d);
}

// Long: real big ints.  TODO: reconsider?  This method makes
// us turn them into strings first ...
size_t OCPyBytesToSerializeLong (PyObject *po, OCPyDumpContext_& dc)
{
  // Convert PyLong to string, THEN we reparse it
  PyObject *str = PyObject_Str(po); /* new ref */
  char* data = PyString_AS_STRING(str);
  int_n bigint = StringToBigInt(data);
  Py_DECREF(str);
  string repr = MakeBinaryFromBigInt(bigint);
  
  const size_t len = repr.length();
  size_t bytes = 1 + ((len<=0xFFFFFFFF) ? sizeof(int_u4) : sizeof(int_u8));

  return bytes + len;
}

void OCPySerializeLong (PyObject *po, OCPyDumpContext_& dc)
{
  PyObject *str = PyObject_Str(po);
  if (dc.compat_) { 
    OCPySerialize(str, dc);
    Py_DECREF(str);
    return;
  }
  
  char*& mem = dc.mem;
  
  // TODO: this is slow: moving from PyObj to Val to binary dump
  char *data = PyString_AS_STRING(str);
  int_n t = StringToBigInt(data);
  Py_DECREF(str);

  // int_n 'q'/'y' int_u4/int_u8 length, (length) chars
  string repr = MakeBinaryFromBigInt(t);
  size_t len = repr.length();
  if (len > 0xFFFFFFFF) {
    *mem++ = 'y';  // Always need tag
    int_u8 l = len;
    VALCOPY(int_u8, l);
  } else {
    *mem++ = 'q'; // Always need tag
    int_u4 l = len;
    VALCOPY(int_u4, l);
  }
  memcpy(mem, repr.data(), len);
  mem += len;
}

// Complex
size_t OCPyBytesToSerializeComplex (PyObject *po, OCPyDumpContext_& dc)
{
  return 1 + sizeof(Py_complex); // Tag 'D' and then 8 real, 8 im
}

void OCPySerializeComplex (PyObject *po, OCPyDumpContext_& dc)
{
  char*& mem = dc.mem;
  *mem++ = 'D';
  Py_complex cx = PyComplex_AsCComplex(po);
  memcpy(mem, &cx, sizeof(Py_complex));
  mem += sizeof(Py_complex);
}

// bool
size_t OCPyBytesToSerializeBool (PyObject *po, OCPyDumpContext_& dc)
{
  return 1 + sizeof(bool); // Tag 'D' and then 8 real, 8 im
}

void OCPySerializeBool (PyObject *po, OCPyDumpContext_& dc)
{
  char*& mem = dc.mem;
  *mem++ = 'b';
  bool res = PyObject_IsTrue(po);
  *mem++ = res;
}

// bool
size_t OCPyBytesToSerializeNone (PyObject *po, OCPyDumpContext_& dc)
{
  return 1; // 'Z'
}

void OCPySerializeNone (PyObject *po, OCPyDumpContext_& dc)
{
  char*& mem = dc.mem;
  *mem++ = 'Z';
}

// String
size_t OCPyBytesToSerializeString (PyObject *po, OCPyDumpContext_& dc)
{
  // 'a' or 'A' plus length then string
  const size_t len = PyString_GET_SIZE(po);
  size_t bytes = 1 + ((len<=0xFFFFFFFF) ? sizeof(int_u4) : sizeof(int_u8));
  return bytes + len;
}

void OCPySerializeString (PyObject *po, OCPyDumpContext_& dc)
{
  char*& mem = dc.mem;

  // string: 'a'/'A', int_u4/int_u8 length, (length) chars
  const size_t len = PyString_GET_SIZE(po);
  if (len > 0xFFFFFFFF) {
    *mem++ = 'A';  // Always need tag
    int_u8 l = len;
    VALCOPY(int_u8, l);
  } else {
    *mem++ = 'a'; // Always need tag
    int_u4 l = len;
    VALCOPY(int_u4, l);
  }
  
  // Copy the bytes
  char *str = PyString_AS_STRING(po);
  memcpy(mem, str, len);
  mem += len;
}

// Raw data blitter: we have to be able to convert from Numeric, array, numpy
inline void OCPyTypeArrayToSerialize (void* data, size_t len, char oc_type,
				      OCPyDumpContext_& dc)
{
  char *&mem = dc.mem;

  size_t byte_len = ByteLength(oc_type);
  if (len <= 0xFFFFFFFF) {
    *mem++ = 'n';
    *mem++ = oc_type;
    int_u4 l = len;
    VALCOPY(int_u4, l);
  } else {
    *mem++ = 'N'; 
    *mem++ = oc_type;
    int_u8 l = len;
    VALCOPY(int_u8, l);
  }
  memcpy(mem, data, byte_len*len);
  mem += len*byte_len;
}

// helper routine for serialization of Numeric arrays:  Returns the
// OC typecode, len, and void* data of the array
void* OCPyGetNumPyArrayInfo (PyObject *po,
			     char& oc_typecode, size_t& len)
{
  len = PyObject_Length(po); // get length in a neutral way
  NUMERIC_PyArrayObject *hope_this_works = (NUMERIC_PyArrayObject*)po;
  char *data = hope_this_works->data;

  // Get the typecode in a string-based way
  PyObject *res = PyObject_GetAttrString(po, (char*)"dtype"); /* new ref */
  if (res==NULL) {
    throw runtime_error("Not a numpy object? Expected dtype");
  }
  PyObject *type_str = PyObject_Str(res); /* new ref */
  char *dtype_data = PyString_AS_STRING(type_str);

  // cerr << "dtypde_data:" << dtype_data << ":" << endl;

  // Convert typecode to OC and let below handle creating array
  oc_typecode = '?';
  try {
    oc_typecode = NumPyStringToOC(dtype_data);
    // cerr << "AFTER TYPECODE" << oc_typecode << int(oc_typecode) << endl;
  } catch (...) { }
  Py_DECREF(type_str);

  return data;
}

size_t OCPyBytesToSerializeNumPyArray (PyObject *po, OCPyDumpContext_& dc)
{
  size_t bytes = 0;
  if (OCPyBytesToSerializeProxy(po, dc, &bytes)) return bytes;

  char oc_typecode; size_t len;
  (void)OCPyGetNumPyArrayInfo(po,
			      oc_typecode, len);

  // Compute bytes from info
  bytes += (len<0xFFFFFFFF) ? sizeof(int_u4) : sizeof(int_u8);
  return 1+1+bytes+len*ByteLength(oc_typecode); // 'n''s'len data
}


void OCPySerializeNumPyArray (PyObject *po, OCPyDumpContext_& dc)
{
  if (OCPySerializeProxy(po, dc)) return;

  char oc_typecode; size_t len;
  void* data = OCPyGetNumPyArrayInfo(po,
				     oc_typecode, len);

  // Have to dispatch and do all the work 
  OCPyTypeArrayToSerialize(data, len, oc_typecode, dc);
}


// helper routine for serialization of Numeric arrays:  Returns the
// OC typecode, len, and void* data of the array
void* OCPyGetNumericArrayInfo (PyObject *po,
			       char& oc_typecode, size_t& len)
{
  len = PyObject_Length(po);  // get length in neutral way
  NUMERIC_PyArrayObject *hope_this_works = (NUMERIC_PyArrayObject*)po;
  char* data = hope_this_works->data;
  
  // Get the typecode in a string-based wway
  PyObject *res = PyObject_CallMethod(po, (char*)"typecode", (char*)"()"); /* new ref */
  if (res==NULL) {
    throw runtime_error("Not a Numeric object? Expected typecode to return str");
  }
  if (!PyString_Check(res)) {
    Py_DECREF(res);
    throw runtime_error("Not a Numeric object? Expected typecode to return str");
  }
  char *typecode_data = PyString_AS_STRING(res);
  size_t typecode_len = PyString_GET_SIZE(res);
  if (typecode_len!=1) {
    Py_DECREF(res);
    throw runtime_error("typecode isn't single letter?");
  }
  oc_typecode = NumericTagToOC(typecode_data[0]);
  
  // Clean up
  Py_DECREF(res);
  
  // Return the data
  return data;
}


size_t OCPyBytesToSerializeNumericArray (PyObject *po, OCPyDumpContext_& dc)
{
  size_t bytes = 0;
  if (OCPyBytesToSerializeProxy(po, dc, &bytes)) return bytes;

  // Get the info about the Numeric array
  char oc_typecode; size_t len;
  (void)OCPyGetNumericArrayInfo(po,
				oc_typecode, len);
  // Count bytes 
  bytes += (len<=0xFFFFFFFF) ? sizeof(int_u4) : sizeof(int_u8);
  return 1 + 1 + bytes + len*ByteLength(oc_typecode); // 'n''s' int4/8 data
}


void OCPySerializeNumericArray (PyObject *po, OCPyDumpContext_& dc)
{
  if (OCPySerializeProxy(po, dc)) return;

  // Get the info about the Numeric array
  char oc_typecode; size_t len;
  void *data = OCPyGetNumericArrayInfo(po,
				       oc_typecode, len);
  // Serialize as an OC Array
  OCPyTypeArrayToSerialize(data, len, oc_typecode, dc);
}

// Proxy

// Because we want the C++ side to deserialize to Proxys,
// we go ahead and mark EVERY once with a marker, even if it's
// not needed.  We *could* note which objects were only deserialized
// once and just emit a val object, but that subjugates the
// proxyness
bool OCPyBytesToSerializeProxy (PyObject *po, OCPyDumpContext_& dc,
				  size_t* bytes)
{
  *bytes = 1 + 4; // 'P' and int_u4 marker always there!

  // Check to see if already been serialized and get the marker,
  // otherwise we'll just be appending new marker
  PyObjectInfo& pyinfo = dc.objectLookup[po];
  pyinfo.object = po;
  if (pyinfo.times++ == 0) {  // 1 more user
    // First time! Fill in marker
    pyinfo.object = po;
    pyinfo.marker = dc.objectLookup.entries()-1;
    *bytes += 3;   // for adopt, lock, allocator
    return false; // Force check of all bytes
  } else {
    // Already seen, no more bytes needed other than 'P'+ marker
    return true;
  }
}


// Only called for Dict, ODict, NumPyArray, NumericArray, List, Seq
bool OCPySerializeProxy (PyObject *po, OCPyDumpContext_& dc)
{
  char *& mem = dc.mem;

  // By the time we are here, we know exactly how many times an object
  // has been serialized, as OCPyBytesToSerialize counted it for us.
  // Then we can emit the proper code for it.
  PyObjectInfo& pyinfo = dc.objectLookup[po];
  *mem++ = 'P';
  int_u4 m = pyinfo.marker;
  VALCOPY(int_u4, m);

  // If not serialized, put out some markers, then
  // let the real serialization begin
  if (!pyinfo.already_serialized) {
    pyinfo.already_serialized = true;
    *mem++ = 1; // adopt flag
    *mem++ = 1; // lock flag.  TODO:this turns on the transaction lock.Too much?
    *mem++ = ' ';
    return false;
  }

  // Proxy there, all done serializing
  return true;
}


// Dynamically import the module, see if we can create a prototype object
static PyObject* /* new ref */
dynamic_check (const char* module, const char* type)
{
  // Try to import  to get the type object therein
  PyObject *mood = PyImport_ImportModule((char*)module);
  if (mood==NULL) { // Not there!
    PyErr_Clear();
    return NULL;
  }
  // Okay, we can get the module, can we get the type object?
  PyObject *callable = PyObject_GetAttrString(mood, (char*)type); // new ref
  if (callable==NULL) {
    PyErr_Clear();
    return NULL;
  }
  int check = PyCallable_Check(callable);
  if (!check) {
    Py_DECREF(callable);
    return NULL;
  }
  PyObject *args     = NULL; 
  if (string(type)=="array") {
    args     = Py_BuildValue((char*)"([i],s)", 1,"i");          // new ref
  } else  {
    args     = Py_BuildValue((char*)"()");          // new ref
  }

  PyObject *res      = PyEval_CallObject(callable, args);        // new ref
  
  Py_DECREF(callable);
  Py_DECREF(args);
  
  return res;
}


// Dynamically import the module, see if we can find the type object
// within the object constructed, and insert into table
template<class TABLE, class ROUTINE> 
static void dynamic_lookup_check (TABLE& l,
				  const char* module, const char* type,
				  ROUTINE conversion_routine)
{
  // Res now has the "type" we want, but we have to get the type object
  // from it
  PyObject *res = dynamic_check(module, type);
  if (res) {
    PyTypeObject *type_object = res->ob_type; // Py_TYPE(res);
    l[type_object] = conversion_routine;
    Py_DECREF(res);
  }
}

// Dispatch to proper type for OCPyBytesToSerialize
size_t OCPyBytesToSerialize (PyObject *po, OCPyDumpContext_& dc)
{
  // Use a table to have types and their proper type lookups for dispatch
  typedef size_t (*PYSERIALIZER_BYTES)(PyObject *po, OCPyDumpContext_& dc);
  typedef AVLHashT<PyTypeObject*, PYSERIALIZER_BYTES, 16> BytesTypeDispatchLookup;

  static BytesTypeDispatchLookup* lookup = 0;
  if (lookup == 0) {
    BytesTypeDispatchLookup *lp = new BytesTypeDispatchLookup;
    BytesTypeDispatchLookup& l=  *lp;

    l[&PyDict_Type]    = &OCPyBytesToSerializeDict;
    // l[&PyList_Type]    = &OCPyBytesToSerializeList; // Sequence
    l[&PyTuple_Type]   = &OCPyBytesToSerializeTuple;    
    l[&PyInt_Type]     = &OCPyBytesToSerializeInt;
    l[&PyLong_Type]    = &OCPyBytesToSerializeLong;
    l[&PyFloat_Type]   = &OCPyBytesToSerializeFloat;
    l[&PyComplex_Type] = &OCPyBytesToSerializeComplex;
    l[&PyString_Type]  = &OCPyBytesToSerializeString;
    l[&PyBool_Type]    = &OCPyBytesToSerializeBool;
    
    dynamic_lookup_check(l, "numpy",       "array", 
			 &OCPyBytesToSerializeNumPyArray);
    dynamic_lookup_check(l, "Numeric",     "array", 
			 &OCPyBytesToSerializeNumericArray);
    dynamic_lookup_check(l, "collections", "OrderedDict", 
			 &OCPyBytesToSerializeODict);
    
    lookup = lp; // install
  }

  PyTypeObject *type_object = po->ob_type; // Py_TYPE(o);
  PYSERIALIZER_BYTES routine;
  if (lookup->findValue(type_object, routine)) {
    return routine(po, dc);
  } else if (PySequence_Check(po)) {
    return OCPyBytesToSerializeSeq(po, dc);
  } else if (po == Py_None) {
    return OCPyBytesToSerializeNone(po, dc);
  } else {
    throw runtime_error("Can't OC serialize this object");
  }
  
}


// Dispatch to proper type
void OCPySerialize (PyObject *po, OCPyDumpContext_& dc)
{
  // Use a table to have types and their proper type lookups for dispatch
  typedef void (*PYSERIALIZER)(PyObject *po, OCPyDumpContext_& dc);
  typedef AVLHashT<PyTypeObject*, PYSERIALIZER, 16> TypeDispatchLookup;
  static TypeDispatchLookup* lookup = 0;
  if (lookup == 0) {
    TypeDispatchLookup *lp = new TypeDispatchLookup;
    TypeDispatchLookup& l=  *lp;
    
    l[&PyDict_Type]    = &OCPySerializeDict;
    // l[&PyList_Type]    = &OCPySerializeList;
    l[&PyTuple_Type]   = &OCPySerializeTuple;
    l[&PyInt_Type]     = &OCPySerializeInt;
    l[&PyLong_Type]    = &OCPySerializeLong;
    l[&PyFloat_Type]   = &OCPySerializeFloat;
    l[&PyComplex_Type] = &OCPySerializeComplex;
    l[&PyString_Type]  = &OCPySerializeString;
    l[&PyBool_Type]    = &OCPySerializeBool;    

    dynamic_lookup_check(l, "numpy",       "array", 
			 &OCPySerializeNumPyArray);
    dynamic_lookup_check(l, "Numeric",     "array", 
			 &OCPySerializeNumericArray);
    dynamic_lookup_check(l, "collections", "OrderedDict", 
			 &OCPySerializeODict);
    
    lookup = lp; // install
  }

  PyTypeObject *type_object = po->ob_type; // Py_TYPE(o);
  PYSERIALIZER routine;
  if (lookup->findValue(type_object, routine)) {
    routine(po, dc);
  } else if (PySequence_Check(po)) {
    OCPySerializeSeq(po, dc);
  } else if (po == Py_None) {
    OCPySerializeNone(po, dc);
  } else {
    throw runtime_error("Can't OC serialize this object");
  }  
}


/////////////////////////////////////////////////////////////////////////

// When a heavyweight object (like List, Tuple, Dict, ODict, Numeric/Numpy
// array) deserializes, it's almost certainally a proxy.  If that's
// the case, we have to remember it in case it comes up again.  But
// we only remember it if it had a mark:  If the lc.last_marker is -1,
// then this is NOT a candidate for remembering.
inline void OCPyMarkProxy (PyObject *po, OCPyLoadContext_& lc)
{
  // If the last marker isn't set, then this isn't for proxying
  int_u4 marker = lc.last_marker;
  if (marker!=int_u4(-1)) {
    PyObjectInfo pyinfo;
    pyinfo.object = po;
    pyinfo.times = 1;
    pyinfo.marker = marker;
    lc.objectLookup[po] = pyinfo;

    PyObject* there = lc.markerLookup[marker];
    if (there != NULL) {
      throw runtime_error("Broke invariant about markers up by 1");
    }
    lc.markerLookup[marker] = po;
    // make sure we don't mark things unwarrantedly
    lc.last_marker = int_u4(-1);
  }
}

// Make a PyInt if the int is small enough, or a PyLong is the int is
// too big

// Most instantiations
template <class T>
inline PyObject* OCPyMakeSomeNumber (T i) 
{ return PyInt_FromLong(i); } 

//inline PyObject* OCMakeSomeNumber (int_1 i) { return PyInt_FromLong(i); } 
//inline PyObject* OCMakeSomeNumber (int_u1 i) { return PyInt_FromLong(i); } 
//inline PyObject* OCMakeSomeNumber (int_2 i) { return PyInt_FromLong(i); } 
//inline PyObject* OCMakeSomeNumber (int_u2 i) { return PyInt_FromLong(i); } 
//inline PyObject* OCMakeSomeNumber (int_4 i) { return PyInt_FromLong(i); } 

template <> 
inline PyObject* OCPyMakeSomeNumber<int_u4>(int_u4 someint)
{
  if (int_u8(someint) <= int_u8(LONG_MAX)) {
    return PyInt_FromLong(long(someint));
  } else {
    string rep = StringizeUInt(someint);
    return PyLong_FromString((char*)rep.c_str(), NULL, 10);
  }
} 

template <> inline PyObject* OCPyMakeSomeNumber<int_8>(int_8 someint)
{
  if ((someint <= int_8(LONG_MAX)) && (someint >= int_8(LONG_MIN))) {
    return PyInt_FromLong(long(someint));
  } else {
    string rep = StringizeInt(someint);
    return PyLong_FromString((char*)rep.c_str(), NULL, 10);
  }
}

template <> inline PyObject* OCPyMakeSomeNumber<int_u8>(int_u8 someint)
{
  if (someint <= int_u8(LONG_MAX)) {
    return PyInt_FromLong(long(someint));
  } else {
    string rep = StringizeUInt(someint);
    return PyLong_FromString((char*)rep.c_str(), NULL, 10);
  }
} 

template <> inline PyObject* OCPyMakeSomeNumber<real_4>(real_4 flt) 
{ return PyFloat_FromDouble(flt); }

template <> inline PyObject* OCPyMakeSomeNumber<real_8>(real_8 flt)
{ return PyFloat_FromDouble(flt); }

#define OCCMPLEXMAKES(T) template <> inline PyObject* OCPyMakeSomeNumber<T> (T cx) \
{ Py_complex px; px.real=double(cx.re); px.imag=double(cx.im);return PyComplex_FromCComplex(px); }

/*
template <class T>
PyObject* OCPyMakeSomeNumber (cx_t<T> cx) 
{
  Py_complex px; 
  px.real = double(cx.re); 
  px.imag = double(cx.im);
  return PyComplex_FromCComplex(px); 
}
*/
OCCMPLEXMAKES(cx_t<int_1>)
OCCMPLEXMAKES(cx_t<int_u1>)
OCCMPLEXMAKES(cx_t<int_2>)
OCCMPLEXMAKES(cx_t<int_u2>)
OCCMPLEXMAKES(cx_t<int_4>)
OCCMPLEXMAKES(cx_t<int_u4>)
OCCMPLEXMAKES(cx_t<int_8>)
OCCMPLEXMAKES(cx_t<int_u8>)
OCCMPLEXMAKES(cx_t<real_4>)
OCCMPLEXMAKES(cx_t<real_8>)


template <class INT>
PyObject* OCPyDeserializeULong (INT len, OCPyLoadContext_& lc)
{
  char*& mem = lc.mem;

  VALDECOPY(INT, len);
  int_un impl;
  MakeBigUIntFromBinary(mem, len, impl);
  mem += len;

  string s = Stringize(impl);
  const char* cstr = s.c_str();

  if (!lc.compat_) {
    return PyLong_FromString((char*)cstr, NULL, 10);
  } else {
    return PyString_FromString(cstr);
  }
}

template <class INT>
PyObject* OCPyDeserializeLong (INT len, OCPyLoadContext_& lc)
{
  char*& mem = lc.mem;

  VALDECOPY(INT, len);
  int_n impl;
  MakeBigIntFromBinary(mem, len, impl);
  mem += len;

  string s = Stringize(impl);
  const char* cstr = s.c_str();

  if (!lc.compat_) {
    return PyLong_FromString((char*)cstr, NULL, 10);
  } else {
    return PyString_FromString(cstr);
  }
}

// For string, long or super long
template <class INT>
PyObject* OCPyDeserializeString (INT len, OCPyLoadContext_& lc)
{ 
  char*& mem = lc.mem;
  // Strs: 'a' then int_u4/int_u8 len, then len bytes
  VALDECOPY(INT, len);
  
  PyObject* res = PyString_FromStringAndSize(mem, len);
  mem += len;
  return res;
}

// /// For ordered dictionary, you have to create the dict dynamically
typedef PyObject* (*PYOBJCREATOR)();

PyObject* ordered_dict_create ()
{ 
  PyObject *result = dynamic_check("collections", "OrderedDict");
  if (result==NULL) result = PyDict_New(); // In case we don't have em
  return result;
}

template <class INT, class PYOBJCREATOR>
PyObject* OCPyDeserializeTable(INT len, PYOBJCREATOR creator, OCPyLoadContext_& lc)
{
  char*& mem = lc.mem;

  VALDECOPY(INT, len);

  PyObject* table = creator(); // PyDict_New() or OrderedDict
  OCPyMarkProxy(table, lc);
  for (size_t ii=0; ii<len; ii++) {
    PyObject *pykey = OCPyDeserialize(lc);    // Get the key
    PyObject *pyvalue = OCPyDeserialize(lc);    // Get the value

    PyDict_SetItem(table, pykey, pyvalue);  // should work for dict and ODict?

    Py_DECREF(pykey);
    Py_DECREF(pyvalue);
  }

  return table;
}

// Deserialize some kind of array!

PyObject* OCPyPODMemoryToNumPy (char tag, size_t len, OCPyLoadContext_& lc)
{
  PyObject *numpy_mod = PyImport_ImportModule((char*)"numpy"); // new ref
  if (!numpy_mod) {
    return NULL;  // first time got set to exception
  }
  // Get Type tag for NumPy Array
  const char* numpy_tag = OCTagToNumPy(tag);

  // Get the "zeros" function and call it
  PyObject *zeros = PyObject_GetAttrString(numpy_mod,(char*)"zeros"); // new ref
  char format[] = "(ls)";  format[1] = PYOC_FORMAT; // l for old,n for PySize_t 
  PyObject *args  = Py_BuildValue(format, len, numpy_tag);  // new ref
  PyObject *numpy_array = PyEval_CallObject(zeros, args);          // new ref
  OCPyMarkProxy(numpy_array, lc);

  // Now have a numpy array of correct length ... have to bit blit
  // real data in
  NUMERIC_PyArrayObject* ao = (NUMERIC_PyArrayObject*)numpy_array;
  size_t byte_len = ByteLength(tag);
  memcpy(ao->data, lc.mem, byte_len*len);
  lc.mem += byte_len*len;

  // Clean up on way out
  Py_DECREF(zeros);
  Py_DECREF(args);
  Py_DECREF(numpy_mod);
  return numpy_array;
}


PyObject* OCPyPODMemoryToNumeric (char tag, size_t len, OCPyLoadContext_& lc)
{
  PyObject *numeric_mod = PyImport_ImportModule((char*)"Numeric");  // new ref
  if (!numeric_mod) {
    return NULL;  
  }
  // Type tag for Numeric Array
  char numeric_tag[2] = { 0,0 };
  numeric_tag[0]  = OCTagToNumeric(tag);

  // Get the "zeros" function and call it
  PyObject *zeros=PyObject_GetAttrString(numeric_mod,(char*)"zeros");// new ref
  // Unclear if numeric even works with large ...
  PyObject *args =Py_BuildValue((char*)"(ls)", len, numeric_tag);    // new ref
  PyObject *numeric_array = PyEval_CallObject(zeros, args);          // new ref
  OCPyMarkProxy(numeric_array, lc);

  // Now have a numeric array of correct length ... have to bit blit
  // real data in
  size_t byte_len = ByteLength(tag);
  NUMERIC_PyArrayObject* ao = (NUMERIC_PyArrayObject*)numeric_array;
  memcpy(ao->data, lc.mem, byte_len*len);
  lc.mem += byte_len*len;

  // Clean up on way out
  Py_DECREF(zeros);
  Py_DECREF(args);
  Py_DECREF(numeric_mod);
  return numeric_array;
}


template <class T>
PyObject* OCPyPODMemoryToList(T *data, size_t len, OCPyLoadContext_& lc)
{
  PyObject *list = PyList_New(len);
  OCPyMarkProxy(list, lc);
  for (size_t ii=0; ii<len; ii++) {
    PyObject *pyvalue = OCPyMakeSomeNumber(data[ii]);
    if (pyvalue==NULL) return NULL;
    PyList_SET_ITEM(list, ii, pyvalue);
  }
  lc.mem += sizeof(T)*len;
  return list;
}

PyObject* OCPyPODMemoryToPythonArray (char tag, size_t len, OCPyLoadContext_& lc)
{ 
  PyErr_SetString(PyExc_RuntimeError, "Python Array currently unsupported");
  return NULL;
}


template <class POD>
PyObject* OCPyConvertPODArrayToPyObj (OCPyLoadContext_& lc, size_t len, POD *data)
{
  PyObject* arr=NULL; 
  char tag = TagFor(data);
  if (!arr) {
    switch (lc.arrdisp_) {
    case AS_LIST:    arr = OCPyPODMemoryToList(data, len, lc);  break;
    case AS_NUMPY:   arr = OCPyPODMemoryToNumPy(tag, len, lc);  break;
    case AS_NUMERIC: arr = OCPyPODMemoryToNumeric(tag, len, lc);  break;
    case AS_PYTHON_ARRAY: arr = OCPyPODMemoryToPythonArray(tag, len, lc); break;
    default: 
      PyErr_SetString(PyExc_RuntimeError, "Unknown Array Disposition");
      return NULL;
    }
    //if (arr) Remember(v, context, arr);
  }

  return arr;
}


template <class INT>
PyObject* OCPyDeserializeList (INT len, OCPyLoadContext_& lc)
{
  PyObject *new_list = PyList_New(len);
  OCPyMarkProxy(new_list, lc);
  for (INT ii=0; ii<len; ii++) {
    PyObject *pyvalue = OCPyDeserialize(lc);
    if (pyvalue==NULL) return NULL;
    PyList_SET_ITEM(new_list, ii, pyvalue);
  }
  return new_list;
}

template <class INT>
PyObject* OCPyDeserializeTuple (INT len, OCPyLoadContext_& lc)
{
  PyObject *new_tuple = PyTuple_New(len);
  OCPyMarkProxy(new_tuple, lc);
  for (INT ii=0; ii<len; ii++) {
    PyObject *pyvalue = OCPyDeserialize(lc);
    if (pyvalue==NULL) return NULL;
    PyTuple_SET_ITEM(new_tuple, ii, pyvalue);
  }
  return new_tuple;
}


#define OCPYVALARR(T) { return OCPyConvertPODArrayToPyObj(lc, len,(T*)lc.mem); }
template <class INT>
PyObject* OCPyDeserializeArrOrTup (char tag, INT len, 
				   OCPyLoadContext_& lc)
{
  char*& mem = lc.mem;
  
  // Arrays: 'n'/'N', then subtype, then int_u4/int_u8 len, then number of
  // values following)
  char subtype = *mem++;
  VALDECOPY(INT, len);
  switch(subtype) {
  case 's': OCPYVALARR(int_1);  break;
  case 'S': OCPYVALARR(int_u1); break;
  case 'i': OCPYVALARR(int_2);  break;
  case 'I': OCPYVALARR(int_u2); break;
  case 'l': OCPYVALARR(int_4);  break;
  case 'L': OCPYVALARR(int_u4); break;
  case 'x': OCPYVALARR(int_8);  break;
  case 'X': OCPYVALARR(int_u8); break;
  case 'b': OCPYVALARR(bool); break;
  case 'f': OCPYVALARR(real_4); break;
  case 'd': OCPYVALARR(real_8); break;
  case 'c': OCPYVALARR(cx_t<int_1>); break;
  case 'C': OCPYVALARR(cx_t<int_u1>); break;
  case 'e': OCPYVALARR(cx_t<int_2>); break;
  case 'E': OCPYVALARR(cx_t<int_u2>); break;
  case 'g': OCPYVALARR(cx_t<int_4>); break;
  case 'G': OCPYVALARR(cx_t<int_u4>); break;
  case 'h': OCPYVALARR(cx_t<int_8>); break;
  case 'H': OCPYVALARR(cx_t<int_u8>); break;
  case 'F': OCPYVALARR(complex_8); break;
  case 'D': OCPYVALARR(complex_16); break;
  case 'a': 
  case 'A': 
  case 't': 
  case 'T': 
  case 'o': 
  case 'O':  
  case 'u': 
  case 'U': // throw logic_error("Can't have POD arrays of tuples");
  case 'n': 
  case 'N': // throw logic_error("Can't have POD arrays of POD arrays");
    // We lose the "array of stringness", and "array of tabness" 
    // but it's probably better it's a list anyways
    return OCPyDeserializeList(len, lc);

  case 'Z': 
    if (lc.compat_ || (tag=='n' || tag=='N')) {
      return OCPyDeserializeList(len, lc);
    } else {
      return OCPyDeserializeTuple(len, lc);
    }
    break;
    
  default: break; // unknownType_("Deserialize Array", subtype);
  }
  
  PyErr_SetString(PyExc_RuntimeError, "Can't OC deserialize this array");
  return NULL;  
}


// Get the proxy
PyObject *OCPyDeserializeProxy (OCPyLoadContext_& lc)
{ 
  // Get the marker, always there!
  char *& mem = lc.mem;
  int_u4 marker;
  VALDECOPY(int_u4, marker);

  // Has this thing already been deserialized?
  if (marker >= lc.markerLookup.length()) {
    // Marker has not been seen.  Add this to the lookup

    // Check invariant
    if (marker != lc.markerLookup.length()) {
      throw runtime_error("Broke invariant: markers supposed to increase by 1");
    }
    lc.markerLookup.append(NULL);
    lc.last_marker = marker;
    
    // These markers aren't used in Python ....
    bool adopt, lock, shm;
    VALDECOPY(bool, adopt);
    VALDECOPY(bool, lock);
    VALDECOPY(bool, shm);  // TODO; do something with this?

    // Deserialize normally, but be on lookout in case we 
    // didn't unset last marker
    PyObject *object = OCPyDeserialize(lc);
    if (lc.last_marker!=int_u4(-1)) {
      lc.last_marker = int_u4(-1);
    }
    return object;
  } else {
    // Marker has already been seen, just get the object
    PyObject *previously_seen_object = lc.markerLookup[marker];
    Py_INCREF(previously_seen_object);
    return previously_seen_object;
  }
} 

#define OCPYDECOPY(T) { T temp; memcpy(&temp,mem,sizeof(T)); mem+=sizeof(T); return OCPyMakeSomeNumber(temp); }
PyObject* OCPyDeserialize (OCPyLoadContext_& lc)
{
  char* &mem = lc.mem;
  char tag = *mem++;
  switch (tag) {
    // ints
  case 's': OCPYDECOPY(int_1); break;
  case 'S': OCPYDECOPY(int_u1); break;
  case 'i': OCPYDECOPY(int_2); break;
  case 'I': OCPYDECOPY(int_u2); break;
  case 'l': OCPYDECOPY(int_4); break;
  case 'L': OCPYDECOPY(int_u4); break;
  case 'x': OCPYDECOPY(int_8); break;
  case 'X': OCPYDECOPY(int_u8); break;
  case 'b': { bool b=*mem++; if(b){ Py_RETURN_TRUE; } else { Py_RETURN_FALSE;} }
  case 'f': OCPYDECOPY(real_4); break;
  case 'd': OCPYDECOPY(real_8); break;
  case 'c': OCPYDECOPY(cx_t<int_1>); break;
  case 'C': OCPYDECOPY(cx_t<int_u1>); break;
  case 'e': OCPYDECOPY(cx_t<int_2>); break;
  case 'E': OCPYDECOPY(cx_t<int_u2>); break;
  case 'g': OCPYDECOPY(cx_t<int_4>); break;
  case 'G': OCPYDECOPY(cx_t<int_u4>); break;
  case 'h': OCPYDECOPY(cx_t<int_8>); break;
  case 'H': OCPYDECOPY(cx_t<int_u8>); break;
  case 'F': OCPYDECOPY(cx_t<real_4>); break;
  case 'D': OCPYDECOPY(cx_t<real_8>); break;

  case 'P': return OCPyDeserializeProxy(lc); break;

  case 'q': { int_u4 len=0; return OCPyDeserializeLong(len, lc); break; }
  case 'y': { int_u8 len=0; return OCPyDeserializeLong(len, lc); break; }
  case 'Q': { int_u4 len=0; return OCPyDeserializeULong(len, lc); break; }
  case 'Y': { int_u8 len=0; return OCPyDeserializeULong(len, lc); break; }

  case 'a': { int_u4 len=0; return OCPyDeserializeString(len, lc); break; }
  case 'A': { int_u8 len=0; return OCPyDeserializeString(len, lc); break; }

  case 'o': { if (!lc.compat_) { 
      int_u4 len=0; return OCPyDeserializeTable(len,ordered_dict_create,lc); break; 
              } // NOTE! Falls through if compatibility mode
            } 
  case 't' : { int_u4 len=0; return OCPyDeserializeTable(len,PyDict_New,lc); break; } 

  case 'O': { if (!lc.compat_) { 
      int_u8 len=0; return OCPyDeserializeTable(len,ordered_dict_create,lc); break; 
              } // NOTE! Falls through if compatibility mode
            } 
  case 'T' : { int_u8 len=0; return OCPyDeserializeTable(len,PyDict_New,lc); break; } 

  case 'u': { int_u4 len=0; return OCPyDeserializeArrOrTup(tag,len,lc); break; }
  case 'U': { int_u8 len=0; return OCPyDeserializeArrOrTup(tag,len,lc); break; }
  case 'n': { int_u4 len=0; return OCPyDeserializeArrOrTup(tag,len,lc); break; }
  case 'N': { int_u8 len=0; return OCPyDeserializeArrOrTup(tag,len,lc); break; }
  case 'Z': { Py_INCREF(Py_None); return Py_None; }
  default: break; //  unknownType_("Deserialize", v.tag);
  }
  
  // Reach here, an error
  PyErr_SetString(PyExc_RuntimeError, "Can't OC deserialize this value");
  return NULL;    
}


// Main routine to dispatch to all OCPySerializeXXX with OCDumpContext
PyObject* OCPyDeserialize (char *mem, bool compat, 
			   ArrayDisposition_e arrdisp)
{
  OCPyLoadContext_ dc(mem, compat, arrdisp);
  return OCPyDeserialize(dc);
}
