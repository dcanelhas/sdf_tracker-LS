#ifndef M2PRETTYPYTHON_H_

#include "m2opaltable.h"
#include "m2ocstringtools.h"
#include "m2opalheader.h"
#include "m2globalevaluator.h"
#include "m2tableize.h"
#include "m2stringizefloats.h"

// //// Forwards

// User-hook for printing OpalValues as Python values (OpalTables
// as Python dictionaries in particular)
inline void PrettyPrintPython (const OpalValue& ov, ostream& os, 
			       int starting_indent=0, int indent_additive=4);
inline void PrettyPrintPython (const OpalTable& ot, ostream& os, 
			       int starting_indent=0, int indent_additive=4);

// Print out an OpalTable as a Python dictionary: this is a helper function
// which mannages the recursion  
inline ostream& PrettyPrintPythonTable_ (const OpalTable& ot, ostream& os, 
					int indent, bool pretty, 
					int indent_additive)
{
  // Figure out if array or hash
  const OpalTableImpl& oti = ot.implementation();
  OpalTableImpl_Type_e impl = oti.type();

  // Base case, empty table
  if (ot.surfaceEntries()==0) {
    const char* out;
    if (impl==OpalTableImpl_GRAPHARRAY) {
      if (pretty) {
	out = "[ ]"; 
      } else {
	out = "[]"; 
      }
    } else {
      if (pretty) {
	out = "{ }";
      } else {
	out = "{}";
      }
    }
    return os << out;
  }

  // Recursive case
  os << ((impl==OpalTableImpl_GRAPHARRAY) ? "[" : "{");
  if (pretty) os << endl;

  // Iterate through, printing out each element
  OpalTableIterator sii(ot, (impl==OpalTableImpl_GRAPHARRAY) ? 
			FASTEST_ORDER : ALPHABETICAL_ORDER);
  for (int ii=0; sii(); ii++) {
    string key = sii.key();
    OpalValue value = sii.value();
   
    if (pretty) indentOut_(os, indent+indent_additive);
    if (impl==OpalTableImpl_GRAPHARRAY) {
      ;  // nothing
    } else {
      os << PyImage(key) << ":";
    }
   
    // For most values, use default output method
    if (value.type() == OpalValueA_TABLE) {
      OpalTable*t=dynamic_cast(OpalTable*, value.data());
      PrettyPrintPythonTable_(*t, os, pretty ? indent+indent_additive : 0,
			      pretty, indent_additive);
    } else {
      PrettyPrintPython(value, os);
    }

    if (ot.surfaceEntries()>1 && ii!=int(ot.surfaceEntries())-1) {
      os << ","; // commas on all but last
    }
    if (pretty) os << endl;
  }

  if (pretty) indentOut_(os, indent);
  os << ((impl==OpalTableImpl_GRAPHARRAY) ? "]" : "}");
  return os;
}

/*
inline void PrettyPrintPythonNumber (const Number& n, ostream& os)
{
  Numeric_e type = n.type();
  if (isComplex(type)) {
    if (type!=M2_CX_FLOAT) {  // Everything but complex_8 uses complex16
      complex_16 c = n;
      const char* sign = (c.im < 0) ? "" : "+";
      os << setprecision(M2StringizePrecision::instance().doublePrecision());
      os << "(" << c.re << sign << c.im << "j)";
    } else if (type==M2_CX_FLOAT) {
      complex_8 c = n;
      const char* sign = (c.im < 0) ? "" : "+";
      os << setprecision(M2StringizePrecision::instance().floatPrecision());
      os << "(" << c.re << sign << c.im << "j)";
    } else {
      throw MidasException("internal error with complex types???");
    }
  } else if (type == M2_TIME) {
    M2Time t = n;
    os << "'" << t << "'";
  } else if (type == M2_DURATION) {
    M2Duration d = n;
    real_8 seconds = d.seconds();
    os << setprecision(M2StringizePrecision::instance().doublePrecision());
    os << seconds;
  } else {
    os << n;
  }
}
*/

// Helper: factor out cx16 output
inline void outputCX16_(const Number& n, ostream& os)
{
  complex_16 c = n;
  const char* sign = (c.im < 0) ? "" : "+";
  os << "(" << StringizeReal8(c.re) << sign << StringizeReal8(c.im) << "j)";
}

// Print out a number
inline void PrettyPrintPythonNumber (const Number& n, ostream& os)
{
  switch (n.type()) {
  case M2_CX_FLOAT: {
    complex_8 c = n;
    const char* sign = (c.im < 0) ? "" : "+";
    os << "(" << StringizeReal4(c.re) << sign << StringizeReal4(c.im) << "j)";
    break;
  } 
  case M2_CX_DOUBLE: {
    outputCX16_(n, os);
    break;
  } 
  case M2_TIME: {
    M2Time t = n;
    os << "'" << t << "'";
    break;
  }
  case M2_DURATION : {
    M2Duration d = n;
    real_8 seconds = d.seconds();
    os << StringizeReal8(seconds);
    break;
  }
  case M2_FLOAT: {
    real_4 r4 = n;
    os << StringizeReal4(r4);
    break;
  }
  case M2_DOUBLE: {
    real_8 r8 = n;
    os << StringizeReal8(r8);
    break;
  }
  default: {
    if (n.isComplex()) { 
      outputCX16_(n, os);
    } else {
      os << n;
    }
    break; 
  }
  }
}

// Convert M2k Numeric_e type into Python Numeric type tag
inline char OpalNumberTypeToPythonNumericStringType (Numeric_e type)
{
  char c;
  switch (type) {
  case M2_BYTE:      c='1'; break;
  case M2_UBYTE:     c='b'; break;
  case M2_INTEGER:   c='s'; break;
  case M2_UINTEGER:  c='w'; break;
  case M2_LONG:      c='i'; break;     
  case M2_ULONG:     c='u'; break;
  case M2_XLONG:     c='l'; break;
  case M2_UXLONG:    c='l'; break;
  case M2_FLOAT:     c='f'; break;
  case M2_DOUBLE:    c='d'; break;
  case M2_CX_FLOAT:  c='F'; break;
  case M2_CX_DOUBLE: c='D'; break;
  case M2_TIME:      c='l'; break; // as M2Time); 
  case M2_DURATION:  c='d'; break; // as M2Duration); 
  default: throw MidasException("Can't process"+type);
    break;
  }
  return c;
}

// Helper function for printing Vectors
template <class T>
inline void ppPythonVector_ (const T* data, int length, Numeric_e type, ostream& os)
{
  os << "array([";
  for (int ii=0; ii<length; ii++) {
    Number n = data[ii];            // reuse Number print code to keep precision
    PrettyPrintPythonNumber(n, os);
    if (ii!=length-1) os << ",";
  }
  
  char numeric_type_string = OpalNumberTypeToPythonNumericStringType(type);
  os << "], '" << numeric_type_string << "')";
}

// PrettyPrint a Vector as a Numeric array (which numpy likes too)
#define PPPYTHONVECTOR_(T) { const T* d=(const T*)v.readData(); ppPythonVector_(d, v.elements(), v.type(), os); break; }
inline void PrettyPrintPythonVector (const Vector& v, ostream& os)
{
  switch (v.type()) {
  case M2_BYTE:      PPPYTHONVECTOR_(int_1); 
  case M2_UBYTE:     PPPYTHONVECTOR_(int_u1); 
  case M2_INTEGER:   PPPYTHONVECTOR_(int_2); 
  case M2_UINTEGER:  PPPYTHONVECTOR_(int_u2); 
  case M2_LONG:      PPPYTHONVECTOR_(int_4); 
  case M2_ULONG:     PPPYTHONVECTOR_(int_u4); 
  case M2_XLONG:     PPPYTHONVECTOR_(int_8); 
  case M2_UXLONG:    PPPYTHONVECTOR_(int_u8); 
  case M2_FLOAT:     PPPYTHONVECTOR_(real_4); 
  case M2_DOUBLE:    PPPYTHONVECTOR_(real_8); 
  case M2_CX_FLOAT:  PPPYTHONVECTOR_(complex_8); 
  case M2_CX_DOUBLE: PPPYTHONVECTOR_(complex_16); 
  case M2_TIME:      PPPYTHONVECTOR_(int_u8); // as M2Time); 
  case M2_DURATION:  PPPYTHONVECTOR_(real_8); // as M2Duration); 
  default: throw MidasException("Unknown how to Python print vector of"+
				Stringize(v.type()));
    break;
  }
}

// Pretty Print the OpalTable as a Python Dioctionary, with indentation
// showing structure
inline void PrettyPrintPython (const OpalTable& ot, ostream& os, 
			       int starting_indent, int indent_additive)
{
  indentOut_(os, starting_indent);
  PrettyPrintPythonTable_(ot, os, starting_indent, true,
			  indent_additive) << endl;
}

// Print an OpalTable as a Python dictionary, but compactly.
inline void PrintPython (const OpalTable& ot, ostream& os)
{
  PrettyPrintPythonTable_(ot, os, 0, false, 0) << endl;
}



// Full implementation
inline void PrettyPrintPython (const OpalValue& ov, ostream& os,
			       int starting_indent, int indent_additive)
{
  bool pretty = true;
  switch (ov.type()) {
    // All these as tables
  case OpalValueA_TABLE : {
    OpalTable* otp = dynamic_cast(OpalTable*, ov.data());
    PrettyPrintPython(*otp, os, starting_indent, indent_additive);
    break;
  }
  case OpalValueA_HEADER : {
    OpalTable ot = OpalHeaderToTable(ov);
    PrettyPrintPython(ot, os, starting_indent, indent_additive);
    break;
  }
  case OpalValueA_MULTIVECTOR : {
    OpalTable ot = MultiVectorToList(ov);
    PrettyPrintPython(ot, os, starting_indent, indent_additive);
    break;
  }
  case OpalValueA_EVENTDATA : {
    OpalTable ot = EventDataToTable(ov);
    PrettyPrintPython(ot, os, starting_indent, indent_additive);
    break;
  }
  case OpalValueA_TIMEPACKET : {
    OpalTable ot = TimePacketToTable(ov);
    PrettyPrintPython(ot, os, starting_indent, indent_additive);
    break;
  }
    // Mostly same as OpalValues
  case OpalValueA_NUMBER : {
    Number n = ov.number();
    PrettyPrintPythonNumber(n, os);
    break;
  } 

    // Vectors become arrays of 
  case OpalValueA_VECTOR: {
    Vector v = UnOpalize(ov, Vector);
    PrettyPrintPythonVector(v, os);
    break;
  }

    // Watch the ' '
  case OpalValueA_STRING: {
    string s = UnOpalize(ov, string);
    os << PyImage(s);
    break;
  }
    // Bool 
  case OpalValueA_BOOL: {
    bool b = UnOpalize(ov, bool);
    const char* out = b ? "True": "False";
    os << out;
    break;
  }

  default: {
    // Defer to op << 
    os << ov;
    break;
  }
  }
}


#define M2PRETTYPYTHON_H_
#endif // M2PRETTYPYTHON_H_
