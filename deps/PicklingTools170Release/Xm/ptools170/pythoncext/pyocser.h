

#include <Python.h> // Include first to avoid redef problems

#define OC_NEW_STYLE_INCLUDES
#include "ocval.h"
#include "arraydisposition.h"


////////////////////////////////////////////////////////////////////
// OCPyDumpContext: Needed to get the proxies and markers right
//
// Info needed to get the shape of the table right
struct PyObjectInfo {
  PyObject* object; // The actual object, in case we need some info out
  size_t times;     // The number of times this object appears
  int_u4 marker;    // The mark associated with this object
  bool   already_serialized; // Have we already serialized it yet?
  PyObjectInfo () : object(0), times(0), marker(0), already_serialized(false) { }
};
typedef AVLHashT<PyObject*, PyObjectInfo, 64> PyObjectLookup;

struct OCPyDumpContext_ {
  OCPyDumpContext_ (char* start_mem, bool compat) :
    mem(start_mem), compat_(compat) { }

  char* mem;  // Where we currently are in the buffer we are dumping into

  // Lookup table for looking up the markers:  When a proxy comes
  // in, we need to know if we have seen it before, and if so,
  // what proxy it refers to.  Note that we hold each proxy by
  // pointer because we are ASSUMING that nothing is changing the
  // table in question (and avoids extra locking)
  PyObjectLookup objectLookup;

  // Compatibility mode: Starting with PicklingTools 1.2.0, we support
  // OTab and Tup: if previous sytems don't support these, we need to
  // be able to turn OTabs->Tab and Tup->Arr.
  bool compat_;  // true means convert OTab->Tab, Tup->Arr

}; // OCPyDumpContext_

// Helper class to keep track of all the Proxy's we've seen so we don't have
// to unserialize again
struct OCPyLoadContext_ {
  OCPyLoadContext_ (char* start_mem, bool compat, 
		    ArrayDisposition_e arrdisp=AS_NUMERIC) : 
    mem(start_mem),compat_(compat),arrdisp_(arrdisp),last_marker(int_u4(-1)) { }

  char* mem; // Where we are in the buffer

  // Lookup by object so you can get its marker
  PyObjectLookup objectLookup;

  // Lookup by marker to be able to get its object
  Array<PyObject*> markerLookup;

  // See OCDumpContext for discussion of compat_
  bool compat_;

  // This may or may not make sense, depending on the type of serialization
  ArrayDisposition_e arrdisp_;

  // When we go to deserialize a proxy, we remember it's marker:
  // the next item to update it will use this as it's marker.
  // This is so we can deserialize things that point to themselves
  // (dict containing a dict that refers to itself)
  int_u4 last_marker;


}; // OCPyLoadContext_

//////////////////////////////////////////////////////////////////////////


// Main entry routine: returns the number of bytes it will take
// to serialize the Object.  If compatibility mode is set, things
// that might be in a particular Python will become related objects
// (ODict->dict, etc.).  Note we build the OCPyDumpContext_ so
// we can fill it in and pass it to OCPySerialize
size_t OCPyBytesToSerialize (PyObject *po, OCPyDumpContext_& dc);

// Serialize into a memory buffer: this assumes mem is big enough to
// hold the data (it can be computed with BytesToSerialize). 
// This returns one past where the next serialize would begin 
// (so that the total bytes serialized
// can be computed from mem-Serialize(...))
void OCPySerialize (PyObject *po, OCPyDumpContext_& dc);


// Deserialize from the given memory and return a Python Object.
// It assumes there is enough  memory (computed with BytesToSerialize) to 
// deserialize from. 
// It returns one byte beyond where it serialized (so mem-return value is
// how many bytes it serialized).  The "into" value has to be an "empty"
// or it throws a logic_error exception.
// The array disposition is for how to load POD Arrays: as Lists?  Numeric?
//  Numpy?  By default, we try to do numpy
PyObject* OCPyDeserialize (char* mem, bool compatibility=false,
			   ArrayDisposition_e arrdisp=AS_NUMPY);
