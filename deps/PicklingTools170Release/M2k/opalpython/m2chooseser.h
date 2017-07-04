#ifndef M2CHOOSESER_H_

// Convenience routines for reading/writing OpalValues to/from Arrays and
// files using the different serializations supported by PTOOLS.
// Their usage is very simple. 
//
// ============For example, to write a pickled file that Python can read:
//
// // C++ side: Write a Value
//  OpalValue v = .. something ..
//  DumpOpalValueToFile(v, "state.p0", SERIALIZE_P0);
//
// # Python side: read the same value
//  import cPickle
//  result = cPickle.load( file('state.p0') ) # load figures out the protocol
//
//
// ==========Another example: have C++ read a file that Python created
// 
// # Python side: write a file
//  v = ... something ...
//  import cPickle
//  cPickle.dump( v, file('state.p2'), 2) # Use Pickling Protocol 2
// 
// // C++ side: read the same file
//  OpalValue result;
//  LoadOpalValueFromFile("state.p2", result, SERIALIZE_P2); 
//  /// .. and we have the same value from Python!
//
//
// ==========Another example: have C++ create an Array 
//  // C++: Creating an array of pickled data I can use
//  OpalValue v;
//  Array<char> buff;
//  LoadOpalValueFromArray(v, buff, SERIALIZE_OC);  // Faster of the protocols
//  cout << "serialized data:" << buff.data() << " length:" << buff.length();

#define MIDAS_COMPILER_TEMPLATES
#include "m2opalpython.h"

#include "m2opalprotocol2.h"
#include "m2pythonpickler.h"
#include "m2openconser.h"
#include "m2pickleloader.h"
#include "m2globalevaluator.h"
#include "m2opalreader.h"

// Different types of serialization: Notice that this is reasonably
// backwards compatible with previous releases, and 0 and 1 still
// correspond to "Pickling Protocol 0" and "No serialization".  Now,
// the value 2 becomes "Pickling Protocol 2".
enum Serialization_e { 
  SERIALIZE_SEND_STRINGS_AS_IS_WITHOUT_SERIALIZATION = 1, // No serialization at all: Dumps as a strings, reads back as a string
  SERIALIZE_PYTHON_PICKLING_PROTOCOL_0 = 0,   // Older, slower, more compatible
  SERIALIZE_PYTHON_PICKLING_PROTOCOL_2 = 2,   // Newer, faster serialization
  SERIALIZE_PYTHON_PICKLING_PROTOCOL_2_AS_PYTHON_2_2_WOULD = -2,
  SERIALIZE_MIDAS2K_BINARY = 4,
  SERIALIZE_OPENCONTAINERS = 5,
  SERIALIZE_PYTHONTEXT = 6,   // alias
  SERIALIZE_PYTHONPRETTY = 7, // ... alias to indicate printing Python dicts
  SERIALIZE_OPALPRETTY = 8,   // ... print an OpalTable pretty
  SERIALIZE_OPALTEXT = 9,     // ... print as Opal WITHOUT pretty indent

  SERIALIZE_TEXT = 6,         // Will stringize on DUMP, Eval on LOAD
  SERIALIZE_PRETTY = 7,       // Will prettyPrint on DUMP, Eval on LOAD

  SERIALIZE_P2_OLDIMPL = -222, // Older implementations of loader
  SERIALIZE_P0_OLDIMPL = -223, // Older implementation of loader

  // Aliases
  SERIALIZE_NONE   = SERIALIZE_SEND_STRINGS_AS_IS_WITHOUT_SERIALIZATION,
  SERIALIZE_P0     = SERIALIZE_PYTHON_PICKLING_PROTOCOL_0, 
  SERIALIZE_P2     = SERIALIZE_PYTHON_PICKLING_PROTOCOL_2, 
  SERIALIZE_M2K    = SERIALIZE_MIDAS2K_BINARY,
  SERIALIZE_OC     = SERIALIZE_OPENCONTAINERS, 

  // Older versions of Python 2.2.x specificially don't "quite" work with
  // serialization protocol 2: they do certain things wrong.  Before we
  // send messages to servers and clients, we need to tell them we are
  // using an older Python that does things slightly differently.
  SERIALIZE_P2_OLD = SERIALIZE_PYTHON_PICKLING_PROTOCOL_2_AS_PYTHON_2_2_WOULD
};


////////////////////////////////// ArrayDisposition_e ////////////////
// Different kinds of POD (Plain Old Data: int_1, int_2, real_4, etc.) arrays: 
// there are essentially 4 different types of POD arrays that might be moving 
// around: 
//
// (1) a = [1,2,3]  
//     Python styles lists (which are inefficient for storing 
//     homogeneous data)
//
// (2) import array; a = array.array('i',[1,2,3])
//     the arrays from the Python module array 
//
// (3) import Numeric: a = Numeric.array([1,2,3], 'i')
//     the Numeric arrays which are built in to XMPY,
//     but most standard Pythons do not have it installed.
//
// (4) import numpy; a = numpy.array([1,2,3], 'i')
//     numpy is the current defacto standard in Python land
//
// In C++, POD arrays are handled as Array<T>, thus (2) & (3) & (4)
// are handled with the same:  (1) is handled as the C++ Arr.  
// These distinctions are more important if you are in Python, or talking 
// to a Python system, as you have to specify how a C++ Array
// converts to a Python POD array.
//
// These 4 distinctions are made because Python doesn't deal
// well with POD (plain old data) arrays well:  This option allows
// you to choose what you want when dealing with POD when you
// convert between systems.  Consider:
// (1) Python style lists work, but are horribly inefficient for
//     large arrays of just plain numbers, both from a storage
//     perspective or accessing.  Also, you "lose" the fact 
//     that this is true POD array if you go back to C++.
// (2) Numeric is old, but handles all the different types well,
//     including complex (although Numeric doesn't deal with int_8s!).
//     It is also NOT a default-installed package: you may have to find
//     the proper RPM for this to work.
// (3) Python array from the array module are default but have issues:
//     (a) can't do complex data 
//     (b) may or may not support int_8
//     (c) pickling changes at 2.3.4 and 2.6, so if you are
//         pickling with protocol 2, you may have issues.
// (4) Python array from the numpy module: major drawback is that it's
//     not built-in
//     
// None of these solutions is perfect, but going to NumPy is
// probably your best choice for the future, although Numeric
// may be your choice if you are stuck on an older Python.
/////////////////////////////////////////////////////////////////////

// This is a convenience function for dumping an OpalValue as an of several
// serializations.  Dump the given OpalValue to the given array of chars: it
// serializes via the given serialization: ArrayDisposition only makes
// sense for SERIALIZE_P0/P2 (see above).
//
// Expert Parameter: perform_conversion_of_OTabTupBigInt_to_TabArrStr
// Some legacy systems may not support OTab, Tup or int_un/int_n, in
// which case we have the OPTION to convert those to something the
// legacy systems can understand: Tab, Arr and Str.  
inline void DumpOpalValueToArray (const OpalValue& given, Array<char>& dump,
				  Serialization_e ser=SERIALIZE_P0,
				  ArrayDisposition_e arrdisp=AS_LIST,
				  bool perform_conversion_of_OTabTupBigInt_to_TabArrStr = false,
				  MachineRep_e endian=MachineRep_EEEI)
{
  static int once = 0;
  if (arrdisp==AS_PYTHON_ARRAY && once==0) {
    once = 1;
    cerr << "As of PicklingTools 1.2.0, the array disposition\n" 
	 << " AS_PYTHON_ARRAY is deprecated.  You will see this warning\n"
	 << " only once...." << endl;
    cerr << "\n"
"  The whole purpose of adding the ArrayDisposition AS_PYTHON_ARRAY\n"
"  was because, in Python 2.6, it was a binary dump: dumping arrays\n"
"  of POD (Plain Old Data, like real_4, int_8, complex_16) was blindingly\n"
"  fast (as it was basically a memcpy): This was to help Python users who\n"
"  did not necessarily have the Numeric module installed.  As of Python 2.7,\n"
"  however, the Pickling of Arrays has changed: it turns each element into a\n"
"  Python number and INDIVIDUALLY pickles each element(much like the AS_LIST\n"
"  option).  The new Pickleloader DOES DETECT AND WORK with both 2.6\n"
"  and 2.7 pickle streams, but we currently only dump as 2.6: this change\n"
"  in Python 2.7 (and also Python 3.x) defeats the whole purpose of\n"
"  supporting array .. we wanted a binary protocol for dumping large amounts\n"
"  of binary data!  As of this release, we deprecate the AS_PYTHON_ARRAY\n"
"  serialization, but will keep it around for a number of releases. \n" 
	 << endl;
  }
  bool conv = perform_conversion_of_OTabTupBigInt_to_TabArrStr;
  switch (ser) {
  case SERIALIZE_P0: case SERIALIZE_P0_OLDIMPL: {
    if (arrdisp==AS_PYTHON_ARRAY) {
      throw runtime_error("SERIALIZE_P0 doesn't support AS_PYTHON_ARRAY");
    }
    PythonBufferPickler<OpalValue> pd(dump, arrdisp);
    pd.compatibility(conv);  // Makes it so we don't have to convert at top,
                             // only as we go.
    pd.dump(given);
    break;
  }

  case SERIALIZE_P2: case SERIALIZE_P2_OLDIMPL: case SERIALIZE_P2_OLD: {
    PicklingIssues_e issues = 
      ((ser==SERIALIZE_P2_OLD) ? AS_PYTHON_2_2 : ABOVE_PYTHON_2_2);
    if (conv) {
      issues = PicklingIssues_e(int(issues)|int(CONVERT_OTAB_TUP_ARR__TO__TAB_ARR_STR));
    }
    
    int bytes = P2TopLevelBytesToDumpOpal(given, arrdisp, issues);
    dump.expandTo(bytes); // overestimate
    char* mem = &dump[0];
    char* rest = P2TopLevelDumpOpal(given, mem, arrdisp, issues);
    int len = rest-mem;   // exact
    dump.expandTo(len);
    break;
  }

  case SERIALIZE_M2K: {
    // Unfortunately, there are about 3-4 ways to serialize data
    // in M2k: the way the OpalPythonDaemon and UDP daemon do it is by
    // forcing it to be a table 
    OMemStream mout;
    if (given.type() == OpalValueA_TABLE) {
      OpalTable tbl = UnOpalize(given, OpalTable);
      mout << tbl;
    } else {
      // Most sockets expect a table, and they can cause seg faults
      // if they don't get an opalTable on input, so force one
      OpalTable special;
      special.put("__SPECIAL__", given);
      mout << special;
    }
    dump.expandTo(mout.length());
    memcpy(&dump[0], mout.data(), sizeof(char)*mout.length()); // Sigh ... should be able to adopt, but M2 Array doesn't support (dagnabit)

    break;
  }

  case SERIALIZE_OC: {
    // The OC, if we request conversion, does it in place so its faster
    int bytes = BytesToSerialize(given);
    dump.expandTo(bytes); // overestimate
    char* mem = &dump[0];
    char* rest = Serialize(given, mem);
    int len = rest-mem;
    dump.expandTo(len);   // exact
    break;
  }

    //case SERIALIZE_TEXT: 
  case SERIALIZE_OPALTEXT: 
  case SERIALIZE_PYTHONTEXT: 
    {
    OpalValue* vp = const_cast<OpalValue*>(&given);
    try {
      if (conv) {
	throw MidasException("Unsupported converting from OTab/Tup/int_n");
	//vp = new OpalValue(given);
	//ConvertAllOTabTupBigIntToTabArrStr(*vp);
      }
      string s;
      ostringstream os;
      if (SERIALIZE_OPALTEXT==ser) {
	if (vp->type() == OpalValueA_TABLE) {
	  OpalTable *otp = dynamic_cast(OpalTable*, vp->data());
	  otp->compactPrint(os);
	} else {
	  s = Stringize(*vp);
	}
      } else {
	PrintPython(*vp, os);
      }
      s = OstrstreamToString(os);
      dump.expandTo(s.length());
      char* mem = &dump[0];
      memcpy(mem, s.data(), s.length());
    } catch (...) {
      //if (conv) delete vp;
      throw;
    }
    break;
  }

    //case SERIALIZE_PRETTY: 
  case SERIALIZE_PYTHONPRETTY: 
  case SERIALIZE_OPALPRETTY: 
    {
    OpalValue* vp = const_cast<OpalValue*>(&given);
    try {
      if (conv) {
	throw MidasException("Unsupported converting from OTab/Tup/int_n");
	//vp = new OpalValue(given);
	//ConvertAllOTabTupBigIntToTabArrStr(*vp);
      }
      ostringstream os;
      if (ser==SERIALIZE_OPALPRETTY) {
	vp->prettyPrint(os);
      } else {
	PrettyPrintPython(*vp, os);
      }
      string s = OstrstreamToString(os);
      dump.expandTo(s.length());
      char* mem = &dump[0];
      memcpy(mem, s.data(), s.length());
    } catch (...) {
      //if (conv) delete vp;
      throw;
    }
    break;
  }

  // This is *slightly* different from Stringize: on the other side,
  // this just becomes a string, where text and pretty do an Eval.
  // The convert flags has ABSOLUTELY NO EFFECT on this.
  case SERIALIZE_NONE: {
    string s = given.stringValue();  
    dump.expandTo(s.length());
    char* mem = &dump[0];
    memcpy(mem, s.data(), s.length());
    break;
  }
  default: throw runtime_error("Unknown serialization");
  }

}

// Load a OpalValue from an array containing serialized data. 
inline void LoadOpalValueFromArray (const Array<char>& dump, OpalValue& result,
			      Serialization_e ser=SERIALIZE_P0,
			      ArrayDisposition_e array_disposition=AS_LIST,
			      bool perform_conversion_of_OTabTupBigInt_to_TabArrStr = false,
			      MachineRep_e endian=MachineRep_EEEI)
{
  bool conv = perform_conversion_of_OTabTupBigInt_to_TabArrStr;
  char* mem = const_cast<char*>(dump.data());
  int   len = dump.length();

  switch (ser) {
    // The new loader supports both versions: P0, P2 (and P1 to a certain ex).
  case SERIALIZE_P0: case SERIALIZE_P2: {
    PickleLoaderImpl<OpalValue> pl(mem, len);

    //pl.env()["supportsNumeric"] = (array_disposition==AS_NUMERIC);
    OpalValue& ov = pl.env();
    OpalTable ot = UnOpalize(ov, OpalTable);
    ot.put("supportsNumber", (array_disposition==AS_NUMERIC));
    ov = ot;

    //cout << pl.env() << endl;
    pl.loads(result);
    if (conv) { 
      throw MidasException("Unsupported converting from OTab/Tup/int_n");
      //  ConvertAllOTabTupBigIntToTabArrStr(result);
    }
    break;
  }

  // The older P0 loader is available for backwards compat.
  case SERIALIZE_P0_OLDIMPL: {
    bool supports_numeric = (array_disposition==AS_NUMERIC);
    Array<char> c(dump); // This routine ACTUALLY CHANGES THE INPUT BUFFER
    PythonBufferDepickler<OpalValue> pbd(c.length(), &c[0], supports_numeric);
    result = pbd.load();

    if (conv) {
      throw MidasException("Unsupported converting from OTab/Tup/int_n");
      //ConvertAllOTabTupBigIntToTabArrStr(result);
    }
    break;
  }

  // The older P2 loader is available for backwards compat.
  case SERIALIZE_P2_OLDIMPL: case SERIALIZE_P2_OLD: {
    P2TopLevelLoadOpal(result, mem);
    if (conv) {
      throw MidasException("Unsupported converting from OTab/Tup/int_n");
      //ConvertAllOTabTupBigIntToTabArrStr(result);
    }
    break;
  }

  // There are many ways to serialize in M2k, we do what
  // the OpalPythonDaemon does: we expect a table
  case SERIALIZE_M2K: {
    OpalTable tbl;
    IMemStream imem(mem, len);
    //imem.setStreamDataEncoding(sde);
    imem >> tbl;

    if (tbl.entries()==1 && tbl.contains("__SPECIAL__")) {
      result = tbl.get("__SPECIAL__");
    } else {
      result = tbl;
    }

    if (conv) {
      throw MidasException("Unsupported converting from OTab/Tup/int_n");
      // ConvertAllOTabTupBigIntToTabArrStr(result);
    }
    break;
  }

  case SERIALIZE_OC: {
    Deserialize(result, mem);
    break;
  }

    //case SERIALIZE_TEXT: 
    // case SERIALIZE_PRETTY: 
  case SERIALIZE_OPALTEXT: 
  case SERIALIZE_OPALPRETTY: {
    // result = Eval(mem, len);
    OpalTable env;
    result = GlobalEvaluator::instance().evaluate(env, string(mem, len));
    if (conv) {
      throw MidasException("Unsupported converting from OTab/Tup/int_n");
      //ConvertAllOTabTupBigIntToTabArrStr(result);
    }
    break;
  }

  case SERIALIZE_PYTHONTEXT: 
  case SERIALIZE_PYTHONPRETTY: {
    result = Eval(mem, len);
    if (conv) {
      throw MidasException("Unsupported converting from OTab/Tup/int_n");
      //ConvertAllOTabTupBigIntToTabArrStr(result);
    }
    break;
  }

  case SERIALIZE_NONE: {
    result = string(mem, len);
    break;
  }

  default: throw runtime_error("Unknown serialization");
  }

}

// A convenience function for dumping a OpalValue to a file: if you want
// finer control over a dump, use the particular serialization by
// itself.  Dump an OpalValue to a file, using one of the serializations
// given: ArrayDisposition only matters for SERIALIZE_P0, SERIALIZE_P2
// (See top of file for full discussion).  Note that SERIALIZE_P0 and
// SERIALIZE_P2 should be readable with Python's cPickle.load(file)
inline void DumpOpalValueToFile (const OpalValue& v, const string& filename, 
				 Serialization_e ser=SERIALIZE_P0,
				 ArrayDisposition_e arrdisp=AS_LIST,
				 bool perform_conversion_of_OTabTupBigInt_to_TabArrStr = false,
				 MachineRep_e endian=MachineRep_EEEI)
{
  Array<char> buff;
  DumpOpalValueToArray(v, buff, ser, arrdisp,
		 perform_conversion_of_OTabTupBigInt_to_TabArrStr, endian);
  ofstream ofs(filename.c_str(), ios::out|ios::binary|ios::trunc);
  if (ofs.good()) {
    char* mem = &buff[0];
    int len = buff.length();
    ofs.write(mem, len);
  } else {
    throw MidasException("Trouble writing the file:"+filename);
  }
}

// Load a OpalValue from a file.
inline void LoadOpalValueFromFile (const string& filename, OpalValue& result, 
				   Serialization_e ser=SERIALIZE_P0,
				   ArrayDisposition_e arrdisp=AS_LIST,
				   bool perform_conversion_of_OTabTupBigInt_to_TabArrStr = false,
				   MachineRep_e endian=MachineRep_EEEI)
{
  Array<char> buff;
  ifstream ifs(filename.c_str(),ios::in|ios::binary|ios::ate);
  if (ifs.good()) {
    ifstream::pos_type length_of_file = ifs.tellg();
    buff.expandTo(length_of_file);
    char* mem = &buff[0];
    ifs.seekg(0, ios::beg);
    ifs.read(mem, length_of_file);
    ifs.close();
  } else {
    throw MidasException("Trouble reading the file:"+filename);
  }
  LoadOpalValueFromArray(buff, result, ser, arrdisp, 
		   perform_conversion_of_OTabTupBigInt_to_TabArrStr,
		   endian);
}


#define M2CHOOSESER_H_
#endif // M2CHOOSESER_H_
