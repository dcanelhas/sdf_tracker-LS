#ifndef M2OPENCONSER_H_

#include "m2opalvalue.h"

// Routines to serialize/deserialize OpalValues as per OpenContainers
// serialization.

// The number of bytes it will take to serialize the given.  Note that
// there are special signatures for Tabs and strings to avoid extra
// copies (so it won't get converted to a Val first!).
size_t BytesToSerialize (const OpalValue& v);
size_t BytesToSerialize (const OpalTable& t);
size_t BytesToSerialize (const string& s);
size_t BytesToSerialize (const Vector& a);

// Serialize into a memory buffer: this assumes mem is big enough to
// hold the data (it can be computed with BytesToSerialize).  Note
// that we have extra signatures for OpalTable and strings to avoid extra
// construction that may not be necessary. This returns one past where
// the next serialize would begin (so that the total bytes serialized
// can be computed from mem-Serialize(...))
char* Serialize (const OpalValue& v, char* mem);
char* Serialize (const OpalTable& t, char* mem);
char* Serialize (const string& s, char* mem);
char* Serialize (const Vector& s, char* mem);

// Deserialize into the given memory.  It assumes there is enough
// memory (computed with BytesToSerialize) to deserialize from. This
// returns one byte beyond where it serialized (so mem-return value is
// how many bytes it serialized).  The into value has to be an "empty"
// OpalValue or it throws a logic_error exception.
char* Deserialize (OpalValue& into, char* mem);

#define M2OPENCONSER_H_
#endif // M2OPENCONSER_H_
