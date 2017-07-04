#ifndef M2OPALPROTOCOL2_H_
#define M2OPALPROTOCOL2_H_

#include "m2opalvalue.h"

// When serializing/deserializing M2k Vectors, they may dump as one of 
// following four entities:
// (1) A Python List (heterogenoues list where the elements are not contiguous in memory)
// (2) A Python Array (import array)
// (3) A Numeric Array (import Numeric)
// (4) A NumPy Array (import numpy)
// NOTE: load is smart enough to handle whatever comes across,
// so this option in NOT necessary in the loading, only dumping.
//  
// NumPy is a more recent de-facto standard, and is probably your best bet.
// If you are using older XMPY for any entities, AS_NUMERIC is
// probably your best best.  Array (2) has recently changed the way they
// unpickle (between 2.6 and 2.7), so we would suggest avoiding it.

enum ArrayDisposition_e { AS_NUMERIC, AS_LIST, AS_PYTHON_ARRAY, AS_NUMPY };


// Python 2.2.x does serialization a little differently using cPickle.
//  Longs serialize differently with cPickle (they serialize as strings)
//  Numeric arrays serialize slightly differently
//  Dumps versions don't have the P2 Preamble
//
// IF YOU HAVE NO ENTITIES USING Python 2.2, IGNORE THIS AND USE THE DEFAULT.
//
// NOTE: load is smart enough to handle whatever comes across,
// so this option in NOT necessary in the loading, only dumping.
enum PicklingIssues_e { AS_PYTHON_2_2=0, ABOVE_PYTHON_2_2=1,
                        CONVERT_OTAB_TUP_ARR__TO__TAB_ARR_STR=2};

// Dump an OpalTable to memory, serializing (aka Pickling) the
// OpalValue using Python Pickling Protocol 2 (the binary/fast protocol 
// for pickling).
// This returns where in memory where the serialization started.
char* P2TopLevelDumpOpal (const OpalValue& ov, char* mem, 
			  ArrayDisposition_e dis=AS_LIST,
			  PicklingIssues_e issues=ABOVE_PYTHON_2_2);

// Allocate a piece of memory this big for dumping ...
int P2TopLevelBytesToDumpOpal (const OpalValue& ov, 
			       ArrayDisposition_e dis=AS_LIST,
			       PicklingIssues_e issues=ABOVE_PYTHON_2_2);

// Load an OpalTable from memory that heas been serialized using
// Python Pickling Protocol 2 (the binary/fast protocol for pickling).
char* P2TopLevelLoadOpal (OpalValue& ov, char* mem);


#endif // M2OPALPROTOCOL2_H_
