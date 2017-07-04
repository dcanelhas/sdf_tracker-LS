#ifndef M2OPALREADER_H_

// NOTE: Due to weird include dependencies, this file includes nothing
// itself.  If you wish to use it, make sure you include ocval.h
// first.

// A class to help read "human-readable" strings of OpalValues (OpalTables,
// numbers, Numeric arrays).  Format is straight-forward:
//
//   OpalValueReader v(" { 'a':'nice', 'table':1 }");
//   OpalValue t;
//   v.expectTab(t);
//
// From Files (streams):
//   StreamOpalValueReader sv(istream& is);
//   OpalTable t;
//   sv.expectTable(t);
//

#include "m2ocstringtools.h"
#include "m2ocreader.h"
#include "m2ocnumerictools.h"
#include "m2prettypython.h"
#include <limits> // for Nan and Inf and -inf
using std::numeric_limits;

// Abstract base class: All the code for parsing the letters one by
// one is here.  The code for actually getting the letters (from a
// string, stream, etc.) defers to the derived class.

// Make it so we can just return quickly without a throw
#define VAL_SYNTAXERROR(MESG) { if (!throwing_) return false; else { syntaxError_(MESG); }}

#define EXPECT_CHAR(EXP) { int get=getNWSChar_(); if (get!=EXP) { if (!throwing_) return false; \
else { string get_string, expected_string; if (get==EOF) get_string="EOF"; else get_string=get;expected_string=EXP; \
syntaxError_("Expected:'"+expected_string+"', but saw '"+get_string+"' on input"); } } }


class OpalValueReaderA { 

  // Indicate which special value we have
  enum OpalValueReaderEnum_e { VR_NOT_SPECIAL, VR_NAN, VR_INF, VR_NEGINF }; 

 public: 

  OpalValueReaderA (ReaderA* adopted_reader, bool throwing=true) :
    reader_(adopted_reader), throwing_(throwing) { }

  virtual ~OpalValueReaderA () { delete reader_; }

  // Look ahead and see that that next thing coming is an EOF
  bool EOFComing () { return reader_->EOFComing(); }

  // Expect any number (including complex)
  bool expectNumber (Number& n)
  {
    consumeWS_();
    int c=peekChar_();
    if (c=='(') {
      return expectComplex_(n);
    }
    return expectSimpleNumber(n);
  }

  // Expect any "non-complex" number, i.e., any positive or negative
  // int float or real: just not any complex numbers with ()!
  bool expectSimpleNumber (Number& n)
  {
    consumeWS_();
    
    // Get the integer part, if any, of the number
    OpalValueReaderEnum_e special = VR_NOT_SPECIAL;
    string integer_part;
    bool result = getSignedDigits_('.', integer_part, &special);
    if (!result) return false;
    
    switch (special) { // will print as nan, -inf and inf
    case VR_INF: { 
      real_8 inf = numeric_limits<double>::infinity();
      n = Number(inf);
      return true;
    } 
    case VR_NEGINF: { 
      real_8 ninf = -numeric_limits<double>::infinity();
      n = Number(ninf);
      return true;
    } 
    case VR_NAN: { 
      real_8 nan = numeric_limits<double>::quiet_NaN();
      n = Number(nan);
      return true;
    }
    case VR_NOT_SPECIAL: default: break; // really, just fall through
    }
    
    // Get the fractional part, if any
    string fractional_part;
    int c = peekChar_();
    if (c=='.') {     
      c = getChar_(); // consume the '.'
      fractional_part = "."+getDigits_();
      if (fractional_part.length()==1) { 
	const int i_len = integer_part.length();
	if (i_len==0 || (i_len>0 && !isdigit(integer_part[i_len-1]))) {
	  VAL_SYNTAXERROR("Expecting some digits after a decimal point, but saw '"+saw_(peekChar_())+"'");
	}
      }
      c = peekChar_();
    }
    
    // Get the exponent part, if any
    string exponent_part;
    if (c=='e' || c=='E') {
      c = getChar_();  // consume the 'e'
      if (!getSignedDigits_(' ', exponent_part)) return false;
      exponent_part = "e"+exponent_part;
      
      if (exponent_part.length()==1) { // only an e
	VAL_SYNTAXERROR("Expected '+', '-' or digits after an exponent, but saw '"+saw_(peekChar_())+"'");
      }
      c = peekChar_();
    }
    
    // At this point, we are (mostly) finished with the number, and we
    // have to build the proper type of number.
    if (fractional_part.length()>0 || exponent_part.length()>0) {
      // If we have either a fractional part or an exponential part,
      // then we have a floating point number
      string stringized_number = integer_part+fractional_part+exponent_part;
      real_8 num = UnStringize(stringized_number, real_8);
      n = Number(num);
      return true;
    }
    
    // Well, no fractional part or exponential.  There had better be
    // some digits!
    if (integer_part.length()==0 || 
	(integer_part.length()>0 && 
	 !isdigit(integer_part[integer_part.length()-1])))
      VAL_SYNTAXERROR("Expected some digits for a number");
    
    c=peekChar_();
    if (c=='l' || c=='L') { // Okay, it's a long
      getChar_();  // consume long
      if (integer_part[0]=='-') {
	int_8 long_int = UnStringize(integer_part, int_8); 
	n = Number(long_int);
	return true;
      } else {
	int_u8 long_int = UnStringize(integer_part, int_u8); 
	n = Number(long_int);
	return true;
      } 
    } else { // plain integer
      n = convertInt_(integer_part); // assumes some int, w/string inside v
      return true;
    }
    return true;
  }
  
  // Read a string from the current place in the input
  virtual bool expectString (string& s)
  { 
    consumeWS_();

    char quote_mark = peekNWSChar_();
    if (quote_mark!='\'' && quote_mark!='"') {
      VAL_SYNTAXERROR("A string needs to start with ' or \"");
    }
    
    EXPECT_CHAR(quote_mark); // Start quote

    // Read string, keeping all escapes, and let DeImage handle escapes
    Array<char> a(80);
    for (int c=getChar_(); c!=quote_mark; c=getChar_()) {
      if (c==EOF) VAL_SYNTAXERROR("Unexpected EOF inside of string");
      a.append(c);
      if (c=='\\') { // escape sequence
	int next = getChar_(); // Avoid '
	if (next==EOF) VAL_SYNTAXERROR("Unexpected EOF inside of string");
	a.append(next);
      } 
    }    
    string temp = string(a.data(), a.length());
    s = DeImage(temp, false); // Do escapes 
    return true;  
  }


  // Expect Table on the input
  bool expectTable (OpalTable& table) { return expectSomeTable_(table, "{"); }

  // Expect an Arr
  bool expectArray (OpalTable& a) { return expectSomeCommaList_(a, '[', ']'); }


  // Read in a Numeric Array
  bool expectNumericArray (Vector& a)
  {
    consumeWS_();
    if (!expect_("array([")) return false; 
    a = Vector(LONG, 0); // Array<int_4>(); // By default, int array

    // Special case, empty list
    char peek = peekNWSChar_();
    if (peek!=']') {
      
      Number n(666);
      if (!expectNumber(n)) return false;

      if (n.type()==M2_LONG) { 
	if (!readNumericArray_<int_4>(a, n)) return false; 
      } else if (n.type()==M2_CX_DOUBLE) { 
	if (!readNumericArray_<complex_16>(a, n)) return false; 
      } else if (n.type()==M2_DOUBLE) { 
	if (!readNumericArray_<real_8>(a, n)) return false; 
      } else VAL_SYNTAXERROR("Only support Numeric arrays of cx16, real_8, and int_4");
    }
    EXPECT_CHAR(']');
    peek=peekNWSChar_();
    if (peek==')') {
      EXPECT_CHAR(')');
      return true;
    }
    // Otherwise, expecting a type tag
    EXPECT_CHAR(',');
    string typetag;
    if (!expectString(typetag)) return false;
    EXPECT_CHAR(')');

    // Now convert if it makes sense.
    if (typetag.length()!=1) VAL_SYNTAXERROR("Expected single char type tag");
    // Convert the array to the given type
    char numeric_tag = typetag[0];
    Numeric_e m2_tag = NumericTagToM2(numeric_tag);
    ConvertArray(a, m2_tag);
    return true;
  }


  // Expect one of two syntaxes:
  // o{ 'a':1, 'b':2 }
  //    or
  // OrderedDict([('a',1),('b',2)])
  // .. of course, in Opal parlance, this is just an OpalTable
  bool expectOTab (OpalTable& o) 
  {
    char c = peekNWSChar_();
    // Short syntax
    if (c=='o') { // o{ } syntax
      return expectSomeTable_(o, "o{");
    } 
    // Long syntax
    if (!expect_("OrderedDict(")) return false;
    OpalTable kvpairs;
    // We let the Arr and Tup parse themselves correctly, but since we
    // are using swapKeyOpalValueueInto, this just plops them into the table
    // pretty quickly, without excessive extra deep copying.
    if (!expectArray(kvpairs)) return false;
    const int len = int(kvpairs.length());
    for (int ii=0; ii<len; ii++) {
      //Tup& t = kvpairs[ii];
      //o.swapInto(t[0], t[1]);
      OpalValue pair = kvpairs.get(ii);
      OpalTable pairt = UnOpalize(pair, OpalTable);
      const string key = pairt.get(0).stringValue();
      const OpalValue value = pairt.get(1);
      o.put(key, value);
    } 
    EXPECT_CHAR(')');
    return true;
  }

  // Know a Tup is coming: starts with '(' and ends with ')'.
  // A tuple in Opal parlance is just an OpalTable GRAPHARRAY impl.
  bool expectTup (OpalTable& u) 
  {
    u = OpalTable(OpalTableImpl_GRAPHARRAY);
    return expectSomeCommaList_(u, '(', ')'); 
  } 

  // In general, a '(' starts either a complex number such as (0+1j)
  // or a tuple such as (1,2.2,'three'): This parses and places the
  // appropriate thing in the return value.
  bool expectTupOrComplex (OpalValue& v)
  {
    EXPECT_CHAR('(');
    
    // Handle empty tuple
    int peek = peekNWSChar_();
    if (peek == EOF) VAL_SYNTAXERROR("Saw EOF after seeing '('");
    if (peek==')') {
      v = OpalTable(OpalTableImpl_GRAPHARRAY); // Empty tuple, empty array
      EXPECT_CHAR(')');
      return true;
    }

    // Anything other than empty, has a first component
    OpalValue first;
    if (!expectAnything(first)) return false;
    
    // Well, plain jane number: if we see a + or -, it must be a complex
    peek = peekNWSChar_();
    if ( (first.type() == OpalValueA_NUMBER) && // OC_IS_NUMERIC(first) && !OC_IS_CX(first) && 
	 (peek=='+' || peek=='-')) {
      // make sure we keep the "-" or "+" so we get the right sign!
      Number first_num = UnOpalize(first, Number);
      Number second(666);
      if (!expectSimpleNumber(second)) return false;
      if (!expect_("j)")) return false;
      if (first_num.type()==M2_FLOAT || second.type()==M2_FLOAT) {
	v = OpalValue(Number(complex_8(first_num, second)));
      } else {
	v = OpalValue(Number(complex_16(first_num, second)));
      }
      return true;
    }

    // If we don't see a plain number, it has to be a Tuple
    v = OpalTable(OpalTableImpl_GRAPHARRAY);
    OpalTable *otp = dynamic_cast(OpalTable*, v.data());
    otp->put(0, first);
    return continueParsingCommaList_(*otp, '(', ')');
  }

  

  // We could be starting ANYTHING ... we have to dispatch to the
  // proper thing
  virtual bool expectAnything (OpalValue& v)
  {
    char c = peekNWSChar_();
    switch (c) {
    case '{' : { v=OpalTable(); OpalTable*otp=dynamic_cast(OpalTable*,v.data());
	         OpalTable& table = *otp; return expectTable(table); }

    case '[' : { v=OpalTable(); OpalTable*otp=dynamic_cast(OpalTable*,v.data());
	         OpalTable& table = *otp; return expectArray(table); }

    case '(' : {                            return expectTupOrComplex(v); }
    case '\'':
    case '"' : { string s; if (!expectString(s)) return false;v=s;return true; }

    case 'a' : { Vector vec;
      if (!expectNumericArray(vec)) return false; v=OpalValue(vec); return true; }

    case 'N' : { v=OpalValue("None", OpalValueA_TEXT); return expect_("None"); }
    case 'T' : { v=OpalValue(bool(true));  return expect_("True");             }
    case 'F' : { v=OpalValue(bool(false)); return expect_("False");            }
    case 'o' : // o{ and OrderedDict start OTab
    case 'O' : { v=OpalTable(); OpalTable*otp=dynamic_cast(OpalTable*,v.data());
                 OpalTable& table = *otp;  return expectOTab(table);         } 
    default:   { Number n(777);                 
        if (!expectNumber(n)) return false; v=OpalValue(n); return true;       }
    }
    
  }


 protected:


  // Helper Methods for syntax error: multiple evals would
  // construct way too many strings when all we pass is ptrs
  void syntaxError_ (const char* s) 
  {
    reader_->syntaxError(s);
  }

  void syntaxError_ (const string& s) 
  {
    reader_->syntaxError(s);
  }


  string saw_ (int cc)
  {
    if (cc==EOF) return "EOF";
    char s[2] = { 0 }; 
    s[0] = cc;
    return string(s);
  }


  bool expect_ (const string& s)
  {
    int s_len = s.length();
    const char* data = s.data();
    for (int ii=0; ii<s_len; ii++) 
      EXPECT_CHAR(data[ii])
    return true;
  }

  bool expect_ (const char* s)
  {
    while (*s) {
      EXPECT_CHAR(*s);
      s++;
    }
    return true;
  }

  // Expect a complex number:  assumes it will have form (#+#j)
  bool expectComplex_ (Number& n)
  {
    complex_16 result;
    EXPECT_CHAR('(');
    Number real_part(0xdead);
    Number imag_part(0xcafe);
    if (!expectNumber(real_part)) return false;
    char c=peekChar_();
    if (c=='+' || c=='-') {
      if (!expectNumber(imag_part)) return false;
    }
    if (!expect_("j)")) return false;

    result.re = real_part;
    result.im = imag_part;
    n = Number(result);
    return true;
  }
  
  // From current point of input, consume all digits until
  // next non-digit.  Leaves non-digit on input
  string getDigits_ ()
  {
    string digits;
    while (1) {
      int c = peekChar_();
      if (c==EOF) 
	break;
      if (isdigit(c)) {
	digits+=char(c);
	getChar_();
      }
      else 
	break;
    }
    return digits;
  }

  // From current point in input, get the signed sequence of 
  // digits
  bool getSignedDigits_ (char next_marker, string& digits,
			 OpalValueReaderEnum_e* inf_nan_okay=0)
  {
    // Get the sign of the number, if any
    char c=peekChar_();
    char the_sign='\0'; // Sentry

    // Add ability to recognize nan, inf, and -inf
    if (inf_nan_okay) {
      if (c=='n') { // expecting nan
	if (expect_("nan")) {
	  *inf_nan_okay = VR_NAN;
	  digits = "nan";
	  return true;
	} else {
	  return false;
	}
      } else if (c=='i') {
	if (expect_("inf")) {
	  *inf_nan_okay = VR_INF;
	  digits = "inf";
	  return true;
	} else {
	  return false;
	}
      }  
    }
    
    char saw_sign = ' ';
    if (c=='+'||c=='-') {
      saw_sign = c;
      the_sign = c;
      getChar_();    // consume the sign
      c=peekChar_(); // .. and see what's next
    }
    
    if (inf_nan_okay) {    
      if (saw_sign!=' ' && c=='i') { // must be -inf
	if (!expect_("inf")) return false;
	if (saw_sign=='-') {
	  *inf_nan_okay = VR_NEGINF;
	  digits = "-inf";
	  return true;
	} else { // if (saw_sign=='+') {
	  *inf_nan_okay = VR_INF;
	  digits = "+inf";
	  return true;
	}
      }
    }
    
    // Assertion: passed sign, now should see number or .
    if (!isdigit(c)&&c!=next_marker) {
      if (!throwing_) return false;
      syntaxError_("Expected numeric digit or '"+saw_(next_marker)+"' for number, but saw '"+saw_(c)+"'");
    }
    
    if (the_sign) {
      string g; g = the_sign;
      digits = g+getDigits_();
    }
    else digits = getDigits_();
    //Str digits = sign+getDigits_();
    //return digits;
    return true;
  }
  
  template <class T>
  bool readNumericArray_ (Vector& arr, const Number& first_value)
  { 
    // Vectors are TERRIBLE at appending (O(n^2)) but Arrays aren't
    Array<T> a;
    T first = first_value;
    a.append(first);
    
    // Continue getting each item one at a time
    while (1) {
      char peek = peekNWSChar_();
      if (peek==',') {
	EXPECT_CHAR(',');
	continue;
      } else if (peek==']') {
	break;
      } 

      Number n(888); 
      if (!expectNumber(n)) return false;
      T next = n;
      a.append(next);
    }
    // Gotten all of the array: memcpy into Vector
    Vector v(NumericTypeLookup((T*)0), a.length());
    memcpy(v.writeData(), &a[0], sizeof(T)*a.length());
    arr = v;
    return true;
  }

  // the string of +- and digits: convert to some int
  Number convertInt_ (const string& v) 
  {
    // Strip into sign and digit part 
    char sign_part = '+';
    string digit_part = v;
    if (!isdigit(digit_part[0])) {
      sign_part = digit_part[0];
      digit_part = digit_part.substr(1);
    }
    const int len = digit_part.length();

    // Try to turn into int_4 in general, otherwise slowly move from
    // int_8 to real_8
    if (sign_part=='-') { // negative number
      static const string smallest_int_4 = "2147483648";
      static const string smallest_int_8 = "9223372036854775808";
      if (len<int(smallest_int_4.length()) ||
	  (len==int(smallest_int_4.length()) && digit_part<=smallest_int_4)) { 
	int_4 i4_val = UnStringize(v, int_4);
	return Number(i4_val);
      } else if (len<int(smallest_int_8.length()) ||
		(len==int(smallest_int_8.length()) && digit_part<=smallest_int_8)) {
	int_8 i8_val = UnStringize(v, int_8);
	return Number(i8_val);
      } else {
	//int_n r8_val = UnStringize(v, int_un);
	real_8 r8_val = UnStringize(v, real_8);
	return Number(r8_val);
      }

    } else { // positive number
      static const string biggest_int_4 = "2147483647";
      static const string biggest_int_8 = "9223372036854775807";
      static const string biggest_int_u8 = "18446744073709551615";
      if (len<int(biggest_int_4.length()) || 
	  (len==int(biggest_int_4.length()) && digit_part<=biggest_int_4)) {
	int_4 i4_val = UnStringize(v, int_4);
	return Number(i4_val);
      } else if (len<int(biggest_int_8.length()) ||
		 (len==int(biggest_int_8.length()) && digit_part<=biggest_int_8)) {
	int_8 i8_val = UnStringize(v, int_8);
	return Number(i8_val);
      } else if (len<int(biggest_int_u8.length()) ||
		(len==int(biggest_int_u8.length()) && digit_part<=biggest_int_u8)) {
	int_u8 iu8_val = UnStringize(v, int_u8);
	return Number(iu8_val);
      } else {
	//int_un r8_val = UnStringize(v, int_un);
	real_8 r8_val = UnStringize(v, real_8);
	return Number(r8_val);
      }
    }
    
  }

  // Expect Table on the input
  template <class TABLE>
  bool expectSomeTable_ (TABLE& table, const char* start_table)
  {
    if (!expect_(start_table)) return false;
    // Special case, empty Table
    char peek = peekNWSChar_();
    if (peek!='}') {
      
      for (;;) { // Continue getting key value pairs
	{ 
	  OpalValue key;   if (!expectAnything(key)) return false;
	  EXPECT_CHAR(':');
	  OpalValue value; if (!expectAnything(value)) return false;
	  
	  table.put(key.stringValue(), value); // Assumes we done with key-value
	}
	
	int peek = peekNWSChar_();
	if (peek==',') {
	  EXPECT_CHAR(',');         // Another k-v pair, grab comma and move on
	  peek = peekNWSChar_();
          if (peek=='}') break;  // CAN have extra , at the end of the table
	  continue;
	} else if (peek=='}') { // End of table
	  break;
	} else {
	  if (!throwing_) return false;
	  syntaxError_("Expecting a '}' or ',' for table, but saw '"+saw_(peek)+"'");
	}
      }
    }
    EXPECT_CHAR('}');
    return true;
  }

  template <class OLIST> 
  bool continueParsingCommaList_ (OLIST& a, char start, char end)
  {
    // Continue getting each item one at a time
    while (1) {         
      int peek = peekNWSChar_();
      if (peek==',') {
	EXPECT_CHAR(',');
	peek = peekNWSChar_();
	if (peek==end) break;  // CAN have extra , at the end of the table
	
	//a.append(OpalValue());
	//if (!expectAnything(a[a.length()-1])) return false;
	OpalValue next_thing;
	if (!expectAnything(next_thing)) return false;
	a.append(next_thing);

	continue;
      } else if (peek==end) {
	break;
      } else {
	if (!throwing_) return false;
	char c[4] = { start, end, '\0' };
	syntaxError_("Problems parsing around '"+saw_(peek)+" for "+string(c));
      }
    }
    EXPECT_CHAR(end);
    return true;
  }

  template <class OLIST> 
  bool expectSomeCommaList_ (OLIST& a, char start, char end)
  {
    EXPECT_CHAR(start);
    
    // Special case, empty list
    char peek = peekNWSChar_();
    if (peek==end) {
      EXPECT_CHAR(end);
    } else {
      // Continue getting each item one at a time, after gotten first
      //a.append(OpalValue());
      //if (!expectAnything(a[a.length()-1])) return false; 
      OpalValue next_thing;
      if (!expectAnything(next_thing)) return false;
      a.append(next_thing);

      if (!continueParsingCommaList_(a, start, end)) return false;
    }
    return true;
  }
  

  // Dispatch for input
  int getNWSChar_ ()  { return reader_->getNWSChar_(); }
  int peekNWSChar_ () { return reader_->peekNWSChar_(); }
  int getChar_ ()     { return reader_->getChar_(); }
  int peekChar_ ()    { return reader_->peekChar_(); }
  int consumeWS_ ()   { return reader_->consumeWS_(); }

  // Defer IO to another class.  All sorts of discussion on why
  // didn't we inherit, etc.  Look at the Design Patterns book.
  ReaderA* reader_; 
  bool throwing_; 

}; // OpalValueReaderA


// The OpalValueReader reads OpalValues from strings. 
// Options: 
// make_copy: Allows a OpalValueReader to share the original input 
// supports_context: allows OpalValueReader to turn on/off context in an 
//   error mesg
class OpalValueReader : public OpalValueReaderA {

 public:

  OpalValueReader (Array<char>& a, 
	     bool make_copy=false, bool supports_context=true,
	     bool throwing_exceptions=true) :
    OpalValueReaderA(new StringReader(a, make_copy, supports_context), 
	       throwing_exceptions)
  { }

  OpalValueReader (const char* s, int len=-1, 
	     bool make_copy=false, bool supports_context=true,
	     bool throwing_exceptions=true) :
    OpalValueReaderA(new StringReader(s, len, make_copy, supports_context), 
	       throwing_exceptions)
  { }

  OpalValueReader (const string& s, 
	     bool make_copy=false, bool supports_context=true, 
	     bool throwing_exceptions=true) :
    OpalValueReaderA(new StringReader(s, make_copy, supports_context),
	       throwing_exceptions)
  { }

    
 protected:

}; // OpalValueReader


// Read a val from a stream
class StreamOpalValueReader : public OpalValueReaderA {

 public:

  // Open the given file, and attempt to read OpalValues out of it
  StreamOpalValueReader (istream& is) :
    OpalValueReaderA(new StreamReader(is))
  { }

}; // StreamOpalValueReader


// Read the given OpalTable from a TEXT file: if there are any problems,
// throw a MidasException indicating we had trouble reading the file,
// or a logic_error if the input is malformed.
inline void ReadOpalTableFromFile (const string& filename, OpalTable& t)
{
  ifstream ifs(filename.c_str());
  if (ifs.good()) {
    StreamOpalValueReader sv(ifs);
    sv.expectTable(t);
  } else {
    throw MidasException("Trouble reading file:"+filename);
  }
}


// Write the given Table to a TEXT file: if there are problems, throw a
// MidasException indicating we had trouble writing out the file.
inline void WriteOpalTableToFile (const OpalTable& t, const string& filename)
{
  ofstream ofs(filename.c_str());
  if (ofs.good()) {
    PrettyPrintPython(t, ofs);
  } else {
    throw MidasException("Trouble writing the file:"+filename);
  }
}

// Read the given OpalValue from a TEXT file: if there are any problems,
// throw a MidasException indicating we had trouble reading the file,
// or a logic_error if the input is malformed.
inline void ReadOpalValueFromFile (const string& filename, OpalValue& v)
{
  ifstream ifs(filename.c_str());
  if (ifs.good()) {
    StreamOpalValueReader sv(ifs);
    sv.expectAnything(v);
  } else {
    throw MidasException("Trouble reading file:"+filename);
  }
}

// Read the given OpalValue from a TEXT file: if there are any problems,
// throw a MidasException indicating we had trouble reading the file,
// or a logic_error if the input is malformed.
inline void ReadOpalValueFromStream (istream& is, OpalValue& v)
{
  if (is.good()) {
    StreamOpalValueReader sv(is);
    sv.expectAnything(v);
  } else {
    throw MidasException("Trouble reading from stream");
  }
}

inline void ReadOpalValueFromFile (const char* filename, OpalValue& v)
{ ReadOpalValueFromFile(string(filename), v); }


// Write the given OpalValue to a TEXT file: if there are problems, throw a
// MidasException indicating we had trouble writing out the file.

inline void WriteOpalValueToFile (const OpalValue& v, const string& filename)
{
  ofstream ofs(filename.c_str());
  if (ofs.good()) {
    PrettyPrintPython(v, ofs);
  } else {
    throw MidasException("Trouble writing the file:"+filename);
  }
}

inline void WriteOpalValueToFile (const OpalValue& v, const char* filename)
{ WriteOpalValueToFile(v, string(filename)); }


// Write the given OpalValue to a TEXT file: if there are problems, throw a
// MidasException indicating we had trouble writing out the file.
inline void WriteOpalValueToStream (const OpalValue& v, ostream& os)
{
  if (os.good()) {
    PrettyPrintPython(v, os);
  } else {
    throw MidasException("Trouble writing to stream");
  }
}


// Evaluate the given string (OpalValue literal) and return the OpalValue 
// underneath
inline OpalValue Eval (const string& code)
{
  OpalValue v;
  OpalValueReader c(code.data(), code.length());
  c.expectAnything(v);
  return v;
}

inline OpalValue Eval (const char* code, int len=-1)
{
  OpalValue v;
  OpalValueReader c(code, len);
  c.expectAnything(v);
  return v;
}



#define VALREADER_H_
#endif // VALREADER_H_


