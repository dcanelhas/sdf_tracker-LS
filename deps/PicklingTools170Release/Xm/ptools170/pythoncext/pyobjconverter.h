

#include <Python.h> // Include first to avoid redef problems

#define OC_NEW_STYLE_INCLUDES
#include "ocval.h"
#include "arraydisposition.h"

#if defined(OC_FORCE_NAMESPACE)
using namespace OC;
#endif

// Convert from PyObject to a Val
Val ConvertPyObjectToVal (PyObject* pyo);

// Convert from Val to PyObject: creates adoptable object */
PyObject *ConvertValToPyObject (const Val& v, ArrayDisposition_e arr_disp); 
