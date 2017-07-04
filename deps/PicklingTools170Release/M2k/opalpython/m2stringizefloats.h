#ifndef M2STRINGIZEFLOATS_H_
#define M2STRINGIZEFLOATS_H_

#define OC_NEW_STYLE_INCLUDES

#define OC_FLT_DIGITS  7
#define OC_DBL_DIGITS 16
#define OC_USE_OC_STRING

// Stringize of floats and doubles should look like Python more, with
// .0 added to distinguish it from an int.  This isn't super fast
// right now: it needs to be cleaned up a little.
inline string StringizeReal4 (const real_4& orig)
{
#if defined(OC_NEW_STYLE_INCLUDES)
  ostringstream os;

  os.precision(OC_FLT_DIGITS);
  if (orig<0) {
    volatile int_8 con=int_8(orig);
    volatile real_4 convert_back = con;
    if (convert_back==orig) {
      os << con << +".0";
    } else {
      os << orig;
    }
  } else {
    volatile int_u8 con=int_u8(orig);
    volatile real_4 convert_back = con;
    if (convert_back==orig) {
      os << con << ".0";
    } else {
      os << orig;
    }
  }

#if defined(OC_USE_OC_STRING)
  std::string s = os.str();
  string ret_val(s.data(), s.length());
#else
  string ret_val = os.str();
#endif
  return ret_val;
#else
  ostrstream os;

  os.precision(OC_FLT_DIGITS);
  if (orig<0) {
    volatile int_8 con=int_8(orig);
    volatile real_4 convert_back = con;
    if (convert_back==orig) {
      os << con << +".0";
    } else {
      os << orig;
    }
  } else {
    volatile int_u8 con=int_u8(orig);
    volatile real_4 convert_back = con;
    if (convert_back==orig) {
      os << con << ".0";
    } else {
      os << orig;
    }
  }
  // os << v << ends; // operator<< will do precision correctly
  os << ends;
  string ret_val = string(os.str());
  delete [] os.str(); // Clean up ostrstream's mess
  return ret_val;
#endif
}


inline string StringizeReal8 (const real_8& orig)
{
#if defined(OC_NEW_STYLE_INCLUDES)
  ostringstream os;

  os.precision(OC_DBL_DIGITS);
  if (orig<0) {
    volatile int_8 con=int_8(orig);
    volatile real_8 convert_back = con;
    if (convert_back==orig) {
      os << con << +".0";
    } else {
      os << orig;
    }
  } else {
    volatile int_u8 con=int_u8(orig);
    volatile real_8 convert_back = con;
    if (convert_back==orig) {
      os << con << ".0";
    } else {
      os << orig;
    }
  }
#if defined(OC_USE_OC_STRING)
  std::string s = os.str();
  string ret_val(s.data(), s.length());
#else
  string ret_val = os.str();
#endif

  return ret_val;
#else
  ostrstream os;

  os.precision(OC_DBL_DIGITS);
  if (orig<0) {
    volatile int_8 con=int_8(orig);
    volatile real_8 convert_back = con;
    if (convert_back==orig) {
      os << con << +".0";
    } else {
      return os << orig;
    }
  } else {
    volatile int_u8 con=int_u8(orig);
    volatile real_8 convert_back = con;
    if (convert_back==orig) {
      os << con << ".0";
    } else {
      os << orig;
    }
  }
  // os << v << ends; // operator<< will do precision correctly
  os << ends;
  string ret_val = string(os.str());
  delete [] os.str(); // Clean up ostrstream's mess
  return ret_val;
#endif
}

#endif // M2STRINGIZEFLOATS_H_
