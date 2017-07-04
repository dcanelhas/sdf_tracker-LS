
// We may or may not have Numeric/NumPy installed, but we still
// have to be able to compile.  This is included just for that.
// TODO: worry, what if this changes?  Numeric almost certainaly
// won't and NumPy probably won't, but 

// PyArrayObject from Numeric ... 
typedef struct {
  PyObject_HEAD
  char *data;
  int nd;
  int *dimensions, *strides;
  PyObject *base;
  PyObject *something; // Invalid PyArray_Desc otherwise  //PyArray_Descr *descr;
  int flags;
  PyObject *weakreflist;
} NUMERIC_PyArrayObject;
// NumPy is similar enough ... all we care about is "data" 
