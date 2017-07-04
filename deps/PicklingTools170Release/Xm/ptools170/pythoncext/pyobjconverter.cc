
#include "pyobjconverter.h"
#include "ocavlhasht.h"
#include "occonforms.h" 
#include "xmldumper.h"

#include "psuedonumeric.h"

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

// Deep-Copy table: to make sure we preserve the structure of the PyObjects
typedef AVLHashT<PyObject*, Val*, 8> DCT;
void PyObjectToVal (PyObject *o, DCT& dct, Val& where_to) ;

// Used so module compiles on more recent and older Pythons
// in 32 vs. 64
#if PY_VERSION_HEX < 0x02050000 && !defined(PY_SSIZE_T_MIN)
typedef int Py_ssize_t;
#define PY_SSIZE_T_MAX INT_MAX
#define PY_SSIZE_T_MIN INT_MIN
#endif

// Return the equiv. Tab for a PyDict
template <class TAB>
inline void PyDictToVal (PyObject *d, DCT& deep_copy_table,
			 Val& where_to)
{
  // Already there?
  if (deep_copy_table.contains(d)) {
    Val *vp = deep_copy_table[d];
    where_to = *vp;
    return;
  }

  // Not there, need to create
  where_to = new TAB();
  TAB& t = where_to;

  // Python table iteration
  PyObject *pykey, *pyvalue;
  Py_ssize_t pos = 0;
  while (PyDict_Next(d, &pos, &pykey, &pyvalue)) {

    // keys don't have to worry about deep-copy, unless we support tuples ... 
    // TODO: Support tuples as keys into dicts for Vals, but for
    Val key;
    PyObjectToVal(pykey, deep_copy_table, key);

    // Values: definitely could already be in deep_copy_table
    if (deep_copy_table.contains(pyvalue)) {
      // In table already ...
      Val value_copy = *deep_copy_table[pyvalue];
      t.swapInto(key, value_copy);
    } else {
      // Not in there yet, have to do conversion
      Val& value = t[key];
      PyObjectToVal(pyvalue, deep_copy_table, value);
      
      if (!IsCompositeType(value)) {
	deep_copy_table[pyvalue] = &value;
      }
    }
  }

  // Remember
  deep_copy_table[d] = &where_to;
}

inline void PySeqToVal (PyObject *l, DCT& deep_copy_table,
			Val& where_to)
{
  // Is this List already in there?
  if (deep_copy_table.contains(l)) {
    where_to = *deep_copy_table[l]; // Hopefully a proxy copy
    return;
  }

  // Create a new list
  int l_len = PySequence_Size(l);
  where_to = new Arr(l_len); // IMPORTANT! arr no resizes, so DCT pointers valid
  Arr& a = where_to;
  for (int ii=0; ii<l_len; ii++) {
    PyObject *pyitem = PySequence_GetItem(l, ii);

    if (!deep_copy_table.contains(pyitem)) {
      // We have to put it in, then we can refer to it by pointer
      a.append(None);
      Val& last = a[a.length()-1];
      PyObjectToVal(pyitem, deep_copy_table, last);
      
      // Only save composite data structures: Tabs, Arrs, Tups, OTabs
      if (IsCompositeType(last)) {
	deep_copy_table[pyitem] = &last;
      }
    } else {
      // Already in deep copy table, so just copy
      Val* vp = deep_copy_table[pyitem];
      a.append(*vp); // proxy copy
    }
  }

  // Remember this List
  deep_copy_table[l] = &where_to; 
}


inline void PyTupleToVal (PyObject *l, DCT& deep_copy_table,
			  Val& where_to)
{
  PySeqToVal(l, deep_copy_table, where_to);

  // Swap the implementations of Tup and Arr
    Arr& a = where_to;
    Array<Val>& az = a;

    Val temp = Tup();
    Tup& t = temp;
    Array<Val>& tz = t.impl();

    tz.swap(az);

  // Swap in the new Tup
  where_to.swap(temp);
}

inline void PyIntToVal (PyObject *o, DCT& deep_copy_table, Val& where_to)
{ where_to = long(PyInt_AsLong(o)); }

inline void PyBoolToVal (PyObject *o, DCT& deep_copy_table, Val& where_to)
{ where_to = bool(PyObject_IsTrue(o)); }

inline void PyLongToVal (PyObject *o, DCT& deep_copy_table, Val& where_to)
{
  // Convert PyLong to string, THEN we reparse it
  PyObject *str = PyObject_Str(o); /* new ref */
  char* data = PyString_AS_STRING(str);
  where_to = StringToBigInt(data);
  Py_DECREF(str);
}

inline void PyFloatToVal (PyObject *f, DCT& deep_copy_table, Val& where_to)
{ where_to = PyFloat_AsDouble(f); }

inline void PyComplexToVal (PyObject *c, DCT& deep_copy_table, Val& where_to)
{
  Py_complex cx = PyComplex_AsCComplex(c);
  where_to = complex_16(cx.real,cx.imag);
}

inline void PyStringToVal (PyObject *o, DCT& deep_copy_table, Val& where_to)
{
  char *s = PyString_AS_STRING(o);
  int len = PyString_GET_SIZE(o);
  where_to = string(s, len);
}



// Raw data blitter: we have to be able to convert from Numeric, array, numpy
template <class T>
inline void SomeArrayToVal (T *adata, int len, Val& where_to)
{
  where_to = new Array<T>(len);
  Array<T>& a = where_to;
  a.expandTo(len);
  memcpy(&a[0], adata, sizeof(T)*len);
}

#define OC_ARRAY_CAST(T) SomeArrayToVal((T*)data, len, where_to) 
inline void PyOCTypeArrayToVal (void* data, int len, char oc_type,
				DCT& deep_copy_table, 
				Val& where_to)
{
  switch (oc_type) {
  case 's': OC_ARRAY_CAST(int_1); break;
  case 'S': OC_ARRAY_CAST(int_u1); break;
  case 'i': OC_ARRAY_CAST(int_2); break;
  case 'I': OC_ARRAY_CAST(int_u2); break;
  case 'l': OC_ARRAY_CAST(int_4); break;
  case 'L': OC_ARRAY_CAST(int_u4); break;
  case 'x': OC_ARRAY_CAST(int_8); break;
  case 'X': OC_ARRAY_CAST(int_u8); break;
  case 'b': OC_ARRAY_CAST(bool); break;
  case 'f': OC_ARRAY_CAST(real_4); break;
  case 'd': OC_ARRAY_CAST(real_8); break;
  case 'F': OC_ARRAY_CAST(complex_8); break;
  case 'D': OC_ARRAY_CAST(complex_16); break;
  default: throw runtime_error("Unknown typecode: don't know how to handle");
  }

}


// If Numeric is supported, we have to be able to convert from
inline void PyNumericArrayToVal (PyObject *o, DCT& deep_copy_table, 
				 Val& where_to)
{
  // Is this List already in there?
  if (deep_copy_table.contains(o)) {
    where_to = *deep_copy_table[o]; // Hopefully a proxy copy
    return;
  }
  
  // Otherwise, we gotta put it in there
  int len = PyObject_Length(o);  // get length in neutral way
  NUMERIC_PyArrayObject *hope_this_works = (NUMERIC_PyArrayObject*)o;
  char* data = hope_this_works->data;
  
  // Get the typecode in a string-based wway
  PyObject *res = PyObject_CallMethod(o, (char*)"typecode", (char*)"()"); /* new ref */
  if (res==NULL) {
    throw runtime_error("Not a Numeric object? Expected typecode to return str");
  }
  if (!PyString_Check(res)) {
    Py_DECREF(res);
    throw runtime_error("Not a Numeric object? Expected typecode to return str");
  }
  char *typecode_data = PyString_AS_STRING(res);
  int typecode_len = PyString_GET_SIZE(res);
  if (typecode_len!=1) {
    Py_DECREF(res);
    throw runtime_error("typecode isn't single letter?");
  }
  char oc_typecode = NumericTagToOC(typecode_data[0]);
  PyOCTypeArrayToVal(data, len, oc_typecode, deep_copy_table, where_to);

  deep_copy_table[o] = &where_to;

  // Clean
  Py_DECREF(res);
}


// If NumPy is supported, we have to be able to convert from
inline void NumPyArrayToVal (PyObject *o, DCT& deep_copy_table, Val& where_to)
{
  // Is this List already in there?
  if (deep_copy_table.contains(o)) {
    where_to = *deep_copy_table[o]; // Hopefully a proxy copy
    return;
  }
  
  // Otherwise, we gotta put it in there
  int len = PyObject_Length(o);  // get length in neutral way
  NUMERIC_PyArrayObject *hope_this_works = (NUMERIC_PyArrayObject*)o;
  char* data = hope_this_works->data;
  
  // Get the typecode in a string-based way
  PyObject *res = PyObject_GetAttrString(o, (char*)"dtype"); /* new ref */
  if (res==NULL) {
    throw runtime_error("Not a numpy object? Expected dtype");
  }
  PyObject *type_str = PyObject_Str(res); /* new ref */
  char *dtype_data = PyString_AS_STRING(type_str);

  // Convert typecode to OC and let below handle creating array
  char oc_typecode = '?';
  try {
    oc_typecode = NumPyStringToOC(dtype_data);
  } catch (...) { }

  // Have this do all the work
  PyOCTypeArrayToVal (data, len, oc_typecode, deep_copy_table, where_to);

  deep_copy_table[o] = &where_to;

  // Clean
  Py_DECREF(type_str);
  Py_DECREF(res);
}




// Convert a dictionary in the PyObject object model to the Val object model.
// NOTE: A lot of the code above assumes that once an object has entered
// a structure, IT WILL NOT MOVE (ie., its pointers will remain valid).

typedef void (*PYTOVAL)(PyObject *o, DCT& deep_copy_table, Val& where_to);
typedef AVLHashT<PyTypeObject*, PYTOVAL, 16> ConverterLookup;


// Dynamically import the module, see if we can find the type object
// within the object constructed, and insert into table
static void 
dynamic_check_con (ConverterLookup& l,
		   const char* module, const char* type,
		   PYTOVAL conversion_routine)
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




void PyObjectToVal (PyObject *o, DCT& dct, Val& where_to) 
{
  if (o == Py_None) return;  // no change

  static ConverterLookup* lookup = 0;
  if (lookup == 0) {
    ConverterLookup *lp= new ConverterLookup;
    ConverterLookup& l=  *lp;
    
    l[&PyDict_Type]    = &PyDictToVal<Tab>;
    l[&PyList_Type]    = &PySeqToVal;
    l[&PyTuple_Type]   = &PyTupleToVal;
    l[&PyInt_Type]     = &PyIntToVal;
    l[&PyLong_Type]    = &PyLongToVal;
    l[&PyFloat_Type]   = &PyFloatToVal;
    l[&PyComplex_Type] = &PyComplexToVal;
    l[&PyString_Type]  = &PyStringToVal;
    l[&PyBool_Type]    = &PyBoolToVal;

    dynamic_check_con(l, "numpy", "array", &NumPyArrayToVal);
    dynamic_check_con(l, "Numeric", "array", &PyNumericArrayToVal);
    dynamic_check_con(l, "collections", "OrderedDict", &PyDictToVal<OTab>);
    
    lookup = lp; // install
  }

  PyTypeObject *type_object = o->ob_type; // Py_TYPE(o);
  PYTOVAL routine;
  if (lookup->findValue(type_object, routine)) {
    routine(o, dct, where_to);
  } else if (PySequence_Check(o)) {
    return PySeqToVal(o, dct, where_to);
  } else {
    cerr << "Unknown??" << endl;
  }
  
  /*(
  if      (PyDict_Check(o)) return PyDictToVal(o, dct, where_to);
  else if (PyList_Check(o)) return PySeqToVal(o, dct, where_to);
  else if (PyTuple_Check(o)) return PyTupleToVal(o, dct, where_to);
  // else if (PySequence_Check(o)) return PyListToVal(o, dct, where_to);
  else if (PyInt_Check(o)) return PyIntToVal(o, dct, where_to);
  else if (PyLong_Check(o)) return PyLongToVal(o, dct, where_to);
  else if (PyFloat_Check(o)) return PyFloatToVal(o, dct, where_to);
  else if (PyComplex_Check(o)) return PyComplexToVal(o, dct, where_to);
  else if (PyString_Check(o)) return PyStringToVal(o, dct, where_to);
  else if (o == Py_None) return;  // no change
  else {
    cerr << "Unknown??" << endl;
  }
  */
}


// Top_level routine to be called by users.  Note that in the conversion
// it will remember structure like a deep_copy would preserve structure.
Val ConvertPyObjectToVal (PyObject* pyo)
{
  Val rvo;
  DCT deep_copy_table;
  PyObjectToVal(pyo, deep_copy_table, rvo);
  return rvo;
}

// ///////////////////////////////////////////////////  Val to PyObject

typedef AVLHashT<void*, PyObject*, 8> VTO;
struct CCC {
  VTO deep_copy_table;
  ArrayDisposition_e arr_disp;
};

PyObject* ConvertValToPyObj (const Val& v, CCC& context);

inline PyObject*
IsAlreadyThere (const Val& v, CCC& context)
{
  // Already there?  Only if a proxy ...
  if (IsProxy(v)) {
    Proxy* p = (Proxy*)&v.u.P;
    void* data = p->data_();
    if (context.deep_copy_table.contains(data)) {
      PyObject *result = context.deep_copy_table(data);
      Py_INCREF(result); // "make a copy"
      return result;
    }
  } 
  return 0;
}

inline void Remember (const Val& v, CCC& context, PyObject *object)
{
  // Put in deep copy table ... if it was a proxy
  if (IsProxy(v)) {
    Proxy* p = (Proxy*)&v.u.P;
    void* data = p->data_();
    context.deep_copy_table[data] = object;
  }
}

PyObject* /* new ref */
ConvertTabToPyObj (const Val& v, CCC& context)
{
  PyObject* table = IsAlreadyThere(v, context);
  if (!table) {

    // Not there, have to recreate this list
    Tab& t = v;
    table = PyDict_New();
    for (It ii(t); ii(); ) {
      const Val& key = ii.key();
      Val& value = ii.value();
      
      PyObject *pykey   = ConvertValToPyObj(key, context);
      if (pykey==NULL) return NULL;
      PyObject *pyvalue = ConvertValToPyObj(value, context);
      if (pyvalue==NULL) return NULL;

      PyDict_SetItem(table, pykey, pyvalue);
      
      Py_DECREF(pykey);
      Py_DECREF(pyvalue);
    }

    Remember(v, context, table);
  }
  
  return table;
}

// helper: if we have collections.OrderedDict, use that, otherwise,
// use dict
PyObject * /* new ref */
CreateODict ()
{
  PyObject *obj = dynamic_check("collections", "OrderedDict");
  if (obj==NULL) {
    return PyDict_New();
  } else {
    return obj;
  }
}

PyObject* /* new ref */
ConvertOTabToPyObj (const Val& v, CCC& context)
{
  PyObject* table = IsAlreadyThere(v, context);
  if (!table) {

    // Not there, have to recreate this list
    OTab& t = v;
    table = CreateODict(); 
    for (It ii(t); ii(); ) {
      const Val& key = ii.key();
      Val& value = ii.value();
      
      PyObject *pykey   = ConvertValToPyObj(key, context);
      if (pykey==NULL) return NULL;
      PyObject *pyvalue = ConvertValToPyObj(value, context);
      if (pyvalue==NULL) return NULL;

      PyObject *key_str = PyObject_Str(pykey);
      char * keyc = PyString_AS_STRING(key_str);
      PyMapping_SetItemString(table, keyc, pyvalue);

      Py_DECREF(key_str);      
      Py_DECREF(pykey);
      Py_DECREF(pyvalue);
    }

    Remember(v, context, table);
  }
  
  return table;
}


inline PyObject* /* new ref */
ConvertArrToPyObj (const Val& v, CCC& context)
{
  PyObject *list = IsAlreadyThere(v, context);
  if (!list) {

    // Not there.  Have to copy list below
    Arr& a = v;
    const int len = a.length(); 
    list = PyList_New(len);
    for (int ii=0; ii<len; ii++) {
      Val& value = a[ii];
      PyObject *pyvalue = ConvertValToPyObj(value, context);
      if (pyvalue==NULL) return NULL;
      PyList_SET_ITEM(list, ii, pyvalue);
    }
    Remember(v, context, list);
  }
  return list;
}

inline PyObject* /* new ref */
ConvertTupToPyObj (const Val& v, CCC& context)
{
  PyObject *tuple = IsAlreadyThere(v, context);
  if (!tuple) {
    // Not there.  Have to copy tuple below
    Tup& a = v;
    const int len = a.length(); 
    tuple = PyTuple_New(len);
    for (int ii=0; ii<len; ii++) {
      Val& value = a[ii];
      PyObject *pyvalue = ConvertValToPyObj(value, context);
      if (pyvalue==NULL) return NULL;
      PyTuple_SET_ITEM(tuple, ii, pyvalue);
    }
    Remember(v, context, tuple);
  }
  return tuple;
}

inline PyObject* /* new ref */
ConvertStringToPyObj (const Val& v, CCC& /*context*/)
{
  OCString* ocp = (OCString*)&v.u.a;
  return PyString_FromStringAndSize(ocp->data(), ocp->length());
}

template <class CXT>
inline PyObject* /* new ref */
ConvertComplexToPyObj (const CXT& cx)
{
  Py_complex prc;
  prc.real = cx.re;
  prc.imag = cx.im;
  return PyComplex_FromCComplex(prc);
}

inline PyObject* /* new ref */
ConvertBigIntToPyObj(const Val& n)
{
  string repr = n; /* get string repr, then convert */
  char* data = &repr[0];
  return PyLong_FromString(data, NULL, 10);
}



template <class T>
PyObject* PODArrayToNumPy (Array<T>& a)
{
  PyObject *numpy_mod = PyImport_ImportModule((char*)"numpy"); // new ref
  if (!numpy_mod) {
    return NULL;  // first time got set to exception
  }
  // length, Type tag for NumPy Array
  int len = a.length();
  const char* numpy_tag = OCTagToNumPy(TagFor((T*)0));

  // Get the "zeros" function and call it
  PyObject *zeros = PyObject_GetAttrString(numpy_mod, (char*)"zeros");    // new ref
  PyObject *args  = Py_BuildValue((char*)"(is)", len, numpy_tag);      // new ref
  PyObject *numpy_array = PyEval_CallObject(zeros, args);      // new ref

  // Now have a numpy array of correct length ... have to bit blit
  // real data in
  NUMERIC_PyArrayObject* ao = (NUMERIC_PyArrayObject*)numpy_array;
  memcpy(ao->data, &a[0], sizeof(T)*len);

  // Clean up on way out
  Py_DECREF(zeros);
  Py_DECREF(args);
  Py_DECREF(numpy_mod);
  return numpy_array;
}


template <class T>
PyObject* PODArrayToNumeric (Array<T>& a)
{
  PyObject *numeric_mod = PyImport_ImportModule((char*)"Numeric");  // new ref
  if (!numeric_mod) {
    return NULL;  
  }
  // length, Type tag for Numeric Array
  int len = a.length();
  char numeric_tag[2] = { 0,0 };
  numeric_tag[0]  = OCTagToNumeric(TagFor((T*)0));

  // Get the "zeros" function and call it
  PyObject *zeros = PyObject_GetAttrString(numeric_mod, (char*)"zeros");    // new ref
  PyObject *args  = Py_BuildValue((char*)"(is)", len, numeric_tag);      // new ref
  PyObject *numeric_array = PyEval_CallObject(zeros, args);      // new ref

  // Now have a numeric array of correct length ... have to bit blit
  // real data in
  NUMERIC_PyArrayObject* ao = (NUMERIC_PyArrayObject*)numeric_array;
  memcpy(ao->data, &a[0], sizeof(T)*len);

  // Clean up on way out
  Py_DECREF(zeros);
  Py_DECREF(args);
  Py_DECREF(numeric_mod);
  return numeric_array;
}

template <class T>
PyObject* PODArrayToPythonArray (Array<T>& a)
{
  PyErr_SetString(PyExc_RuntimeError, "Python array module currently unsupported");
  return NULL;
}

template <class T>
inline PyObject* /* new ref */
PODArrayToList(const Array<T>& a, CCC& context)
{
  const int len = a.length(); 
  PyObject *list = PyList_New(len);
  for (int ii=0; ii<len; ii++) {
    Val value = a[ii];
    PyObject *pyvalue = ConvertValToPyObj(value, context);
    if (pyvalue==NULL) return NULL;
    PyList_SET_ITEM(list, ii, pyvalue);
  }
  return list;
}

template <class POD>
PyObject *
ConvertPODArrayToPyObj (const Val& v, CCC& context, POD)
{
  PyObject* arr = IsAlreadyThere(v, context);
  if (!arr) {
    Array<POD>& a = v;

    switch (context.arr_disp) {
    case AS_LIST:    arr = PODArrayToList(a, context);  break;
    case AS_NUMPY:   arr = PODArrayToNumPy(a);  break;
    case AS_NUMERIC: arr = PODArrayToNumeric(a);  break;
    case AS_PYTHON_ARRAY: arr = PODArrayToPythonArray(a); break;
    default: 
      PyErr_SetString(PyExc_RuntimeError, "Unknown Array Disposition");
      return NULL;
    }
    if (arr) Remember(v, context, arr);
  }

  return arr;
}

#define INTCONVERT(VAL, INTTYPE) return PyInt_FromLong(INTTYPE(VAL))
#define ARRAYCONVERT(VAL, POD) return ConvertPODArrayToPyObj(VAL, context, POD(0))
PyObject* ConvertValToPyObj (const Val& v, CCC& context)
{
  switch (v.tag) {
  case 's': INTCONVERT(v, int_1); 
  case 'S': INTCONVERT(v, int_u1); 
  case 'i': INTCONVERT(v, int_2); 
  case 'I': INTCONVERT(v, int_u2); 
  case 'l': INTCONVERT(v, int_4); 
  case 'L': INTCONVERT(v, int_u4); 
  case 'x': INTCONVERT(v, int_8); 
  case 'X': INTCONVERT(v, int_u8); 
    //case 'b': INTCONVERT(v, bool);  // TODO: should use Python bool?
  case 'b': return PyBool_FromLong(long(v));
  case 'f': return PyFloat_FromDouble(real_4(v)); 
  case 'd': return PyFloat_FromDouble(real_8(v)); 
  case 'F': return ConvertComplexToPyObj<complex_8>(v); 
  case 'D': return ConvertComplexToPyObj<complex_16>(v); 
  case 'q': return ConvertBigIntToPyObj(v); 
  case 'Q': return ConvertBigIntToPyObj(v); 
  case 'o': return ConvertOTabToPyObj(v, context); 
  case 't': return ConvertTabToPyObj(v, context); 
  case 'a': return ConvertStringToPyObj(v, context); 
  case 'u': return ConvertTupToPyObj(v,  context); 
  case 'Z': { Py_INCREF(Py_None); return Py_None; }
  case 'n': {
    switch (v.subtype) {
    case 's': ARRAYCONVERT(v, int_1); 
    case 'S': ARRAYCONVERT(v, int_u1); 
    case 'i': ARRAYCONVERT(v, int_2); 
    case 'I': ARRAYCONVERT(v, int_u2); 
    case 'l': ARRAYCONVERT(v, int_4); 
    case 'L': ARRAYCONVERT(v, int_u4); 
    case 'x': ARRAYCONVERT(v, int_8); 
    case 'X': ARRAYCONVERT(v, int_u8); 
    case 'b': ARRAYCONVERT(v, bool);  // TODO: should use Python bool?
    case 'f': ARRAYCONVERT(v, real_4); 
    case 'd': ARRAYCONVERT(v, real_8); 
    case 'F': ARRAYCONVERT(v, complex_8); 
    case 'D': ARRAYCONVERT(v, complex_16); 
    case 'Z': return ConvertArrToPyObj(v, context); 
    default: 
      PyErr_SetString(PyExc_RuntimeError, "Can't handle arrays of anything but POD?");
      return NULL;
    }
  }
  default: 
      PyErr_SetString(PyExc_RuntimeError, "Unknown type");
      return NULL;
  }
} 


PyObject *
ConvertValToPyObject(const Val& v, ArrayDisposition_e arr_disp)
{
  CCC context;
  context.arr_disp = arr_disp;
  return ConvertValToPyObj(v, context);
}
