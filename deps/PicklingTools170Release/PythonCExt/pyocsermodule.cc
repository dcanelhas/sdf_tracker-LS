#include "pyocser.h"


// C-linkage for Python hook
extern "C" {

PyObject *
CImpl_OCDumps (PyObject *self, PyObject *args)
{  
  // Parse argument: get single object to serialize
  PyObject *object_to_serialize;
  int compat = 0;
  if (!PyArg_ParseTuple(args, (char*)"O|i",
			&object_to_serialize, &compat)) {
    return NULL;
  }

  try {
    // Do the work:  this may throw an exception .. we'll catch and turn
    // into a Python exception
    OCPyDumpContext_ dc(NULL, compat);
    size_t bytes = OCPyBytesToSerialize(object_to_serialize, dc);
    
    // Get a string from Python's memory system that has enough memory
    // for us to serializre
    PyObject *string_mem = PyString_FromStringAndSize(NULL, bytes);
    if (string_mem==NULL) {
      PyErr_SetString(PyExc_RuntimeError, "Can't create big enough string to serialize into");
      return NULL;
    }
    
    // Assertion: have enough memory
    char *mem = PyString_AsString(string_mem);
    dc.mem = mem;
    OCPySerialize(object_to_serialize, dc);
    char *return_mem = dc.mem;
    if (return_mem != mem + bytes) {
      cerr << "return mem=" << int_u8(return_mem) << " mem+bytes=" << int_u8(mem+bytes) << endl;
      PyErr_SetString(PyExc_RuntimeError, "Different number of bytes returned than needed? ocdumps failed.");
      Py_DECREF(string_mem);
      return NULL;
    }

    // Return a string with this in it
    return string_mem;
  } catch (const exception& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return NULL;
  }
}


PyObject *
CImpl_OCLoads (PyObject *self, PyObject *args)
{
  PyObject *ret = NULL;
  
  // Parse argument: get memory to deserialize from
  PyObject *string_mem;
  int compat = 0;
  int arrdisp = int(AS_NUMPY);
  if (!PyArg_ParseTuple(args, (char*)"O|ii",
			&string_mem, &compat, &arrdisp)) {
    return NULL;
  }

  // Make sure this is a string
  if (!PyString_Check(string_mem)) {
    PyErr_SetString(PyExc_TypeError, "expected Python string object");
    return NULL;
  }

  try {
    // Do the work
    // size_t len = PyString_GET_SIZE(string_mem);
    char*  mem = PyString_AS_STRING(string_mem);
    ret = OCPyDeserialize(mem, compat, ArrayDisposition_e(arrdisp));
    return ret;
  } catch (const exception& e) {
    PyErr_SetString(PyExc_RuntimeError, e.what());
    return NULL;
  }
}


static PyMethodDef PyOCMethods[] = {

  {(char*)"ocdumps", CImpl_OCDumps,
   METH_VARARGS, (char*)"Dump an object to a string using OC serialization" },

  {(char*)"ocloads", CImpl_OCLoads,
   METH_VARARGS, (char*)"Load an object from a string using OC serialization" },


  {NULL, NULL, 0, NULL}   /* Sentinel */
};


PyMODINIT_FUNC
initpyocser(void)
{
  (void) Py_InitModule((char*)"pyocser", PyOCMethods); 
}


}; // End C-Linkage
