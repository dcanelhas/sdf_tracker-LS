#include "pyobjconverter.h"

#include "xmldumper.h"
#include "xmlloader.h"

// C-linkage for Python hook
extern "C" {

 

PyObject *
CImpl_ReadFromXMLStream(PyObject* self, PyObject *args)
{
  PyObject *ret = NULL;

  PyObject* stream;
  int options=XML_STRICT_HDR | XML_LOAD_DROP_TOP_LEVEL | XML_LOAD_EVAL_CONTENT;
  int arr_disp_int = AS_NUMERIC;
  int prepend_char = XML_PREPEND_CHAR;

  if (!PyArg_ParseTuple(args, (char*)"o|iii", 
			&stream, &options, &arr_disp_int, &prepend_char)) {
    return NULL;
  }

  // Turn Python File object into FILE*
  if (!PyFile_Check(stream)) {
    PyErr_SetString(PyExc_TypeError, "expected Python stream object");
    return NULL;
  }
  FILE* fp = PyFile_AsFile(stream);

  // Valid args, so convert!
  try {
    ArrayDisposition_e arr_disp = ArrayDisposition_e(arr_disp_int);
    Val v;
    ReadValFromXMLFILE(fp, v, options, arr_disp, prepend_char);
    ret = ConvertValToPyObject(v, arr_disp);
    return ret;
  } catch (const exception& e) {
    const char *mesg = e.what();
    PyErr_SetString(PyExc_RuntimeError, mesg);
  }
  return ret;
}

PyObject *
CImpl_ReadFromXMLFile(PyObject* self, PyObject *args)
{
  PyObject *ret = NULL;

  char* filename;
  int options=XML_STRICT_HDR | XML_LOAD_DROP_TOP_LEVEL | XML_LOAD_EVAL_CONTENT;
  int arr_disp_int = AS_NUMERIC;
  int prepend_char = XML_PREPEND_CHAR;

  if (!PyArg_ParseTuple(args, (char*)"s|iii", 
			&filename, &options, &arr_disp_int, &prepend_char)) {
    return NULL;
  }

  // Valid args, so convert!
  try {
    ArrayDisposition_e arr_disp = ArrayDisposition_e(arr_disp_int);
    Val v;
    ReadValFromXMLFile(filename, v, options, arr_disp, prepend_char);
    ret = ConvertValToPyObject(v, arr_disp);
    return ret;
  } catch (const exception& e) {
    const char *mesg = e.what();
    PyErr_SetString(PyExc_RuntimeError, mesg);
  }
  return ret;
}


PyObject *
CImpl_ReadFromXMLString(PyObject* self, PyObject *args)
{
  PyObject *ret = NULL;

  char* xml_string;
  int options=XML_STRICT_HDR | XML_LOAD_DROP_TOP_LEVEL | XML_LOAD_EVAL_CONTENT;
  int arr_disp_int = AS_NUMERIC;
  int prepend_char = XML_PREPEND_CHAR;

  if (!PyArg_ParseTuple(args, (char*)"s|iii", 
			&xml_string, &options, &arr_disp_int, &prepend_char)) {
    return NULL;
  }

  // Valid args, so convert!
  try {
    ArrayDisposition_e arr_disp = ArrayDisposition_e(arr_disp_int);
    Val v;
    ReadValFromXMLString(xml_string, v, options, arr_disp, prepend_char);
    ret = ConvertValToPyObject(v, arr_disp);
    return ret;
  } catch (const exception& e) {
    const char *mesg = e.what();
    PyErr_SetString(PyExc_RuntimeError, mesg);
  }
  return ret;
}


PyObject *
CImpl_WriteToXMLStream(PyObject* self, PyObject *args)
{
  PyObject *ret = NULL;

  PyObject *obj;
  PyObject* stream;
  PyObject* top_level_key_obj = NULL;
  int options=XML_DUMP_PRETTY | XML_STRICT_HDR | XML_DUMP_STRINGS_BEST_GUESS;
  int arr_disp_int = AS_NUMERIC;
  int prepend_char = XML_PREPEND_CHAR;

  if (!PyArg_ParseTuple(args, (char*)"OO|Oiii", 
			&obj, &stream, &top_level_key_obj, &options, &arr_disp_int, &prepend_char)) {
    return NULL;
  }

  // Turn Python File object into FILE*
  if (!PyFile_Check(stream)) {
    PyErr_SetString(PyExc_TypeError, "expected Python stream object");
    return NULL;
  }
  FILE* fp = PyFile_AsFile(stream);

  // top-level key has to be a string or none
  Val top_level_key;
  if (top_level_key_obj==NULL) {
    top_level_key = PyString_FromString("top");
  } else if (PyObject_Type(Py_None)==PyObject_Type(top_level_key_obj)) {
    ; // Keep top level key as none
  } else {
    // Must be string
    PyObject *top_level_as_string = PyObject_Str(top_level_key_obj); // new ref
    top_level_key = string(PyString_AS_STRING(top_level_as_string));
    Py_DECREF(top_level_as_string);
  }
  
  // Valid args, so convert!
  try {
    ArrayDisposition_e arr_disp = ArrayDisposition_e(arr_disp_int);
    Val v = ConvertPyObjectToVal(obj);
    WriteValToXMLFILEPointer(v, fp, top_level_key, options, arr_disp, prepend_char);
    ret = Py_None;
    Py_INCREF(ret);
  } catch (const exception& e) {
    const char *mesg = e.what();
    PyErr_SetString(PyExc_RuntimeError, mesg);
  }
  return ret;
}

PyObject *
CImpl_WriteToXMLFile(PyObject* self, PyObject *args)
{
  PyObject *ret = NULL;

  PyObject *obj;
  char *filename;
  PyObject* top_level_key_obj = NULL;
  int options=XML_DUMP_PRETTY | XML_STRICT_HDR | XML_DUMP_STRINGS_BEST_GUESS;
  int arr_disp_int = AS_NUMERIC;
  int prepend_char = XML_PREPEND_CHAR;

  if (!PyArg_ParseTuple(args, (char*)"Os|Oiii", 
			&obj, &filename, &top_level_key_obj, &options, &arr_disp_int, &prepend_char)) {
    return NULL;
  }

  Val top_level_key;
  if (top_level_key_obj==NULL) {
    top_level_key = PyString_FromString("top");
  } else if (PyObject_Type(Py_None)==PyObject_Type(top_level_key_obj)) {
    ; // Keep top level key as none
  } else {
    // Must be string
    PyObject *top_level_as_string = PyObject_Str(top_level_key_obj); // new ref
    top_level_key = string(PyString_AS_STRING(top_level_as_string));
    Py_DECREF(top_level_as_string);
  }
  
  // Valid args, so convert!
  try {
    ArrayDisposition_e arr_disp = ArrayDisposition_e(arr_disp_int);
    Val v = ConvertPyObjectToVal(obj);
    WriteValToXMLFile(v,filename,top_level_key,options,arr_disp,prepend_char);
    ret = Py_None;
    Py_INCREF(ret);

  } catch (const exception& e) {
    const char *mesg = e.what();
    PyErr_SetString(PyExc_RuntimeError, mesg);
  }
  return ret;
}

PyObject *
CImpl_WriteToXMLString(PyObject* self, PyObject *args)
{
  PyObject *ret = NULL;

  PyObject *obj;
  PyObject* top_level_key_obj = NULL;
  int options=XML_DUMP_PRETTY | XML_STRICT_HDR | XML_DUMP_STRINGS_BEST_GUESS;
  int arr_disp_int = AS_NUMERIC;
  int prepend_char = XML_PREPEND_CHAR;

  if (!PyArg_ParseTuple(args, (char*)"O|Oiii", 
			&obj, &top_level_key_obj, &options, &arr_disp_int, &prepend_char)) {
    return NULL;
  }

  Val top_level_key;
  if (top_level_key_obj==NULL) {
    top_level_key = PyString_FromString("top");
  } else if (PyObject_Type(Py_None)==PyObject_Type(top_level_key_obj)) {
    ; // Keep top level key as none
  } else {
    // Must be string
    PyObject *top_level_as_string = PyObject_Str(top_level_key_obj); // new ref
    top_level_key = string(PyString_AS_STRING(top_level_as_string));
    Py_DECREF(top_level_as_string);
  }
  
  // Valid args, so convert!
  try {
    ArrayDisposition_e arr_disp = ArrayDisposition_e(arr_disp_int);
    Val v = ConvertPyObjectToVal(obj);
    string res;
    WriteValToXMLString(v,res,top_level_key,options,arr_disp,prepend_char);
    ret = PyString_FromStringAndSize(res.data(), res.length());
  } catch (const exception& e) {
    const char *mesg = e.what();
    PyErr_SetString(PyExc_RuntimeError, mesg);
  }
  return ret;
}

PyObject *
CImpl_ConvertToVal(PyObject* self, PyObject *args)
{
  PyObject *obj = NULL;
  int prin = 0;
  if (!PyArg_ParseTuple(args, (char*)"O|i", &obj, &prin)) {
    return NULL;
  }
  try {
    Val v = ConvertPyObjectToVal(obj);
    if (prin) {
      v.prettyPrint(cout);
      cout << endl;
    }
  } catch (exception& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return NULL;
  }
  PyObject *non = Py_None;
  Py_INCREF(non);
  return non;
}

PyObject *
CImpl_deepcopy_via_val(PyObject* self, PyObject *args)
{
  PyObject *obj = NULL;
  int arr_disp_int = AS_LIST;
  if (!PyArg_ParseTuple(args, (char*)"O|i", &obj, &arr_disp_int)) {
    return NULL;
  }
  try {
    Val v = ConvertPyObjectToVal(obj);

    ArrayDisposition_e arr_disp = ArrayDisposition_e(arr_disp_int);
    PyObject *ret = ConvertValToPyObject(v, arr_disp);
    return ret;
  } catch (exception& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return NULL;
  }
  PyObject *non = Py_None;
  Py_INCREF(non);
  return non;
}

static PyMethodDef PyObjConvertMethods[] = {

  {(char*)"CReadFromXMLStream", CImpl_ReadFromXMLStream,
   METH_VARARGS, (char*)"convert XML stream to a PyObject" },

  {(char*)"CReadFromXMLFile", CImpl_ReadFromXMLFile,
   METH_VARARGS, (char*)"convert file containing XML to a PyObject" },

  {(char*)"CReadFromXMLString", CImpl_ReadFromXMLString,
   METH_VARARGS, (char*)"convert string containing XML to a PyObject" },

  {(char*)"CWriteToXMLStream", CImpl_WriteToXMLStream,
   METH_VARARGS, (char*)"convert PyObject to a stream containing XML" },

  {(char*)"CWriteToXMLFile", CImpl_WriteToXMLFile,
   METH_VARARGS, (char*)"convert PyObject to a file containing XML" },

  {(char*)"CWriteToXMLString", CImpl_WriteToXMLString,
   METH_VARARGS, (char*)"convert PyObject to a string containing XML" },

  {(char*)"ConvertToVal", CImpl_ConvertToVal,
   METH_VARARGS, (char*)"convert PyObject to a Val" },

  {(char*)"deepcopy", CImpl_deepcopy_via_val,
   METH_VARARGS, (char*)"perform a deep copy of an object (going to a Val first)" },


  {NULL, NULL, 0, NULL}   /* Sentinel */
};


PyMODINIT_FUNC
initpyobjconvert(void)
{
  (void) Py_InitModule((char*)"pyobjconvert", PyObjConvertMethods); 
}




};
