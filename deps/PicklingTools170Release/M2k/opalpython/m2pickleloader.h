#ifndef M2PICKLELOADER_H_
#define M2PICKLELOADER_H_

// For M2k, we don't WANT the namespaces!
#define OC_BEGIN_NAMESPACE
#define OC_END_NAMESPACE
#define PTOOLS_BEGIN_NAMESPACE
#define PTOOLS_END_NAMESPACE


// If you are compiling with M2k, these are the set of macros
// and functions you need to get the PickleLoader working


#include "m2opalvalue.h"
#include "m2opalgraphhash.h"
#include "m2opalgrapharray.h"
#include "m2opaltable.h"
#include "m2opalvaluet.h"
#include "m2pythontools.h"
#include "m2table.h"


//#include <iostream>
using std::endl; using std::cerr; using std::cout;

inline OpalGraphHash& M2ExtractDict_ (OpalValue& ov) 
{  
  OpalTable*t=dynamic_cast(OpalTable*, ov.data()); 
  OpalGraphHash& ogh = dynamic_cast(OpalGraphHash&, t->implementation());
  return ogh;
}

inline OpalValue& M2DictSubScript_ (OpalGraphHash& ogh, const string& key) 
{ 
  OpalEdge oe;
  ogh.findEdge(key, oe); 
  return oe.nonconstRef();
}

inline OpalGraphArray& M2ExtractList_ (OpalValue& ov) 
{  
  OpalTable*t=dynamic_cast(OpalTable*, ov.data()); 
  OpalGraphArray& oga = dynamic_cast(OpalGraphArray&, t->implementation());
  return oga;
}

inline OpalGraphArray& M2ExtractList_ (const OpalValue& ov) 
{  
  const OpalTable*t=dynamic_cast(const OpalTable*, ov.data()); 
  const OpalGraphArray& oga_const = 
    dynamic_cast(const OpalGraphArray&, t->implementation());
  OpalGraphArray& oga = const_cast(OpalGraphArray&, oga_const);
  return oga;
}

// Mirror the EXACT structure of OpalValueT so we can reference
// the underlying data
template<class T>
struct MirrorOpalValueT : public OpalValueA {
  T value_;
  string* classType_;  
};
inline Vector& M2ExtractVector_ (OpalValue& ov) 
{  
  OpalValueT<Vector>*t=dynamic_cast(OpalValueT<Vector>*, ov.data()); 
  MirrorOpalValueT<Vector>*mt = reinterpret_cast(MirrorOpalValueT<Vector>*,t);
  return mt->value_;
}

inline bool M2IsTableType_ (const OpalValue& ov,OpalTableImpl_Type_e look_for) 
{  
  if (ov.type() != OpalValueA_TABLE) 
    return false;
  const OpalTable*t=dynamic_cast(const OpalTable*, ov.data()); 
  return t->implementation().type()==look_for;
}


inline OpalValue& M2ArraySubScript_ (OpalGraphArray& oga, int index) 
{ 
  OpalEdge oe=oga.at(index); 
  return oe.nonconstRef();
}

inline OpalValue TupleMake1 (const OpalValue& o1)
{
  OpalTable tup = OpalTable(OpalTableImpl_GRAPHARRAY);
  tup.append(o1);
  return OpalValue(tup);
}

inline OpalValue TupleMake2 (const OpalValue& o1, const OpalValue& o2)
{
  OpalTable tup = OpalTable(OpalTableImpl_GRAPHARRAY);
  tup.append(o1);
  tup.append(o2);
  return OpalValue(tup);
}

inline OpalValue TupleMake3 (const OpalValue& o1, const OpalValue& o2,
			     const OpalValue& o3)
{
  OpalTable tup = OpalTable(OpalTableImpl_GRAPHARRAY);
  tup.append(o1);
  tup.append(o2);
  tup.append(o3);
  return OpalValue(tup);
}

// Convert a string to an int
template <class I>
inline I StringToInt (const char* data, int len)
{
  I result=0;  // RVO
  char c = ' ';
  char sign = '\0';
  int ii;
  // Skip white space
  for (ii=0; ii<len; ii++) {
    c = data[ii];
    if (isspace(c)) continue;
    else if (isdigit(c) || c=='-' || c=='+') break;
    else ii=len; // Done
  }
  // Only accept sign after white space
  if (c=='+' || c=='-') {
    ii++;
    sign=c;
  }
  for (; ii<len; ii++) {
    c = data[ii];
    if ( !isdigit(c) ) break; // Only keep going if digit
    result*= 10;
    result+= (c-'0');
  }
  if (sign=='-') {
    result = -result;
  }
  return result;
}

// No BigInt in M2k, so make a String 
inline string MakeBigIntFromBinary (char* start, int len)
{
  return string(start, len); // TODO: should print out pretty?
}

// Ugh, put this back into Arr
template <class OBJ>
void SwapIntoAppend (OpalGraphArray& a, const OBJ& v)
{
  a.append(new OpalValue(v));
}


// Some swap routine
template <class T>
void M2Swap(T& l1, T& l2) 
{
  T temp(l1);
  l1 = l2;
  l2 = temp;
}

// HACK: we should probably just make OpalValue contain a swap.
// This is what the guts of an OpalValue are.
struct LikeOpal {

  static void swap (LikeOpal& l1, LikeOpal& l2)
  {
    if (l1.value_==(OpalValueA*)&l1.data_ && 
	l2.value_==(OpalValueA*)&l2.data_) {
      // Both l1 and l2 in lookaside buffer, swap
      // VALUES but not pointers.
      char buffer[M2_OPALVALUE_MAX_LENGTH];
      memcpy(&buffer[0], &l1.data_,  M2_OPALVALUE_MAX_LENGTH); 
      memcpy(&l1.data_,  &l2.data_,  M2_OPALVALUE_MAX_LENGTH); 
      memcpy(&l2.data_,  &buffer[0], M2_OPALVALUE_MAX_LENGTH); 
    } else if (l1.value_!=(OpalValueA*)&l1.data_ && 
	       l2.value_!=(OpalValueA*)&l2.data_) {
      // Both l1 and l2 are EXTERNAL, just swap pointers
      M2Swap(l1.value_, l2.value_); 
    } else {  // one is EXTERNAL, other in lookaside.

      // Canonicalize so lp1 points to EXTERNAL, lp2 to lookaside
      LikeOpal *lp1 = &l1;
      LikeOpal *lp2 = &l2;
      if (l1.value_==(OpalValueA*)&l1.data_) {
	M2Swap(lp1, lp2);
      }
      // Assertion: lp1 points to EXTERNAL, lp2 points to lookaside
      lp2->value_ = lp1->value_;
      memcpy(&lp1->data_,  &lp2->data_,  M2_OPALVALUE_MAX_LENGTH); 
      lp1->value_ = (OpalValueA*)&lp1->data_;
      // Assertion: lp1 points to lookaside (copied), lp2 to EXTERNAL, swapped!
    }
  }

  union {
    most_restrictive_alignment_t forcealignment_;
    char buffer[M2_OPALVALUE_MAX_LENGTH];
  } data_;
  
  // This usually points to data_ below.  OpalValue is a "union"
  // type container, where it has enough space to hold any of the
  // canonical OpalValueTs that are constructed.  If value_ points
  // into data below, we know the value was constructed in place.
  // If it wasn't constructed in place, then it was off the heap and
  // the destructor will need to delete it from the heap.
  OpalValueA* value_;
};

void M2OpalSwap (OpalValue& o1, OpalValue& o2)
{
  LikeOpal* lo1 = (LikeOpal*)&o1;
  LikeOpal* lo2 = (LikeOpal*)&o2;
  LikeOpal::swap(*lo1, *lo2);
}

void ErrorMe(const string& s)
{
  throw MidasException(s); 
}

void ErrorBadToken(char c)
{
  string ss;
  ss =c ;
  ErrorMe("Don't know how to handle "+ss); 
}

void RangeError (const string& s)
{
  throw MidasException(s);
}

// Macros to pull stuff out of memory (binary format) either as
// BIG-ENDIAN or LITTLE-ENDIAN.  Most Intel machines are little-endian by
// default.
#if !defined(MACHINE_BIG_ENDIAN)
#define LOAD_FROM(T, IN, OUT) { T* rhs=((T*)(IN)); (*(OUT))=*rhs; }
#define LOAD_DBL(IN, OUT) { char* rhs=((char*)(IN)); char* lhs=((char*)(OUT)); lhs[0]=rhs[7]; lhs[1]=rhs[6]; lhs[2]=rhs[5]; lhs[3]=rhs[4]; lhs[4]=rhs[3]; lhs[5]=rhs[2]; lhs[6]=rhs[1]; lhs[7]=rhs[0]; }
#else

#define LOAD_FROM(T, IN, OUT) { char*rhs=((char*)(IN)); char*lhs=((char*)(OUT));  for (int ii=0; ii<sizeof(T); ii++) lhs[ii]=rhs[sizeof(T)-1-ii]; }
//#define LOAD_FROM_4(RHS, LHS) { char*lhs=(char*)LHS; char*rhs=(char*)rhs; lhs[0] = rhs[3]; lhs[1] = rhs[2]; lhs[2]=rhs[1]; lhs[3]=rhs[0]; }
#define LOAD_DBL(IN, OUT) { real_8* rhs=((real_8*)(IN)); real_8* lhs=((real_8*)(V)); *lrhs=*rhs; }


#endif

# define INSERT_STRING(s, start, len)    s = string(start, len);
# define NONE_VALUE                      OpalValue("None", OpalValueA_TEXT)
# define CHEAP_VALUE                     OpalValue()
# define MAKE_DICT()                     OpalValue(OpalTable(OpalTableImpl_GRAPHHASH))
# define EXTRACT_DICT(DICTNAME, OV)      OpalGraphHash& DICTNAME=M2ExtractDict_(OV)
# define DICT_CONTAINS(D,V)              (D).contains(V)
# define DICT_GET(D,V)                   M2DictSubScript_((D), (V))
# define DICT_PUT(D,KEY,VALUE)           (D).put(KEY,new OpalValue(VALUE))
# define MAKE_LIST()                     OpalValue(OpalTable(OpalTableImpl_GRAPHARRAY))
# define MAKE_LIST1(EXPECT_LEN)          OpalValue(OpalTable(OpalTableImpl_GRAPHARRAY))
# define EXTRACT_LIST(LISTNAME, OV)      OpalGraphArray& LISTNAME=M2ExtractList_(OV)
# define LIST_SUB(LST,I)                 M2ArraySubScript_(LST,I)
# define LIST_LENGTH(LST)                M2ExtractList_(LST).length()
# define LIST_LENGTH1(LST)               (LST).surfaceEntries()

# define MAKE_OBJ_FROM_NUMBER(N)         new OpalValue(Number(N))

# define MAKE_TUP0()                     MAKE_LIST()
# define MAKE_TUP1(A)                    TupleMake1(A)
# define MAKE_TUP2(A,B)                  TupleMake2(A,B)
# define MAKE_TUP3(A,B,C)                TupleMake3(A,B,C)
# define MAKE_TUPN(EXPECT_LEN)           MAKE_LIST()
# define EXTRACT_TUP(TUPNAME, OV)        OpalGraphArray& TUPNAME=M2ExtractList_(OV)
# define TUP_SUB(TPLE, INDX)             M2ArraySubScript_(TPLE, INDX)
# define TUP_LENGTH(LST)                 M2ExtractList_(LST).surfaceEntries()
# define TUP_LENGTH1(LST)                (LST).surfaceEntries() 
# define EXTRACT_TUP_AS_LIST(A, U)       OpalGraphArray& A = (U)

#define DICT_SWAP_INTO(DICT,KEY,VALUE)   (DICT).put(Stringize(KEY), new OpalValue(VALUE))
#define TRUE_VALUE                       OpalValue(true)
#define FALSE_VALUE                      OpalValue(false)

# define LIST_SWAP_INTO_APPEND(A, V)     SwapIntoAppend(A, V);
# define MAKE_BIGINT_FROM_STRING(S,START,LEN)   S = OpalValue(string(START, LEN))

# define MAKE_COMPLEX(R,I)               OpalValue(Number(complex_16(R,I)))

# define MAKE_VECTOR1(T, LEN)            OpalValue(Vector(NumericTypeLookup((T*)0),(LEN)))
# define EXTRACT_VECTOR(T, A, FROM)      Vector& A = M2ExtractVector_(FROM)
# define VECTOR_EXPAND(T,A,LENGTH) // Nothing ... already expanded to!
# define VECTOR_RAW_PTR(T, A)            ((T*)(A.writeData()))

# define EXTRACT_STRING(O)               UnOpalize((O), string)
# define EXTRACT_INT(O)                  UnOpalize((O), int)
# define EXTRACT_BOOL(O)                 UnOpalize((O), bool)
# define EXTRACT_T(T, O)                 UnOpalize((O), T)
# define EXTRACT_NUMBERT(T, O)           (UnOpalize((O), Number).operator T())

# define IS_STRING(O)                    ((O).type()==OpalValueA_STRING)
# define IS_LIST(O)                      M2IsTableType_(O, OpalTableImpl_GRAPHARRAY)
# define IS_TUP(O)                       M2IsTableType_(O, OpalTableImpl_GRAPHARRAY)

# define GENERIC_GET(CONT, KEY)          (CONT).get(KEY)

# define MAKE_BIGINT(NAME)               string NAME
# define MAKE_BIGINT1(NAME,VALUE)        string NAME=VALUE
# define MAKE_BIGINT_FROM_BIN(START,LEN, RESULT)  string RESULT=SignedDecimalPrint(START,LEN,false)
# define MAKE_NUMBER(T, THING)           OpalValue(Number(T(THING)))

# define SWAP(O1, O2)                    M2OpalSwap(O1, O2);

// When registering things with the factory, they take in some tuple
// and return a Val: REDUCE tend to be more for built-in complicated
// types like Numeric, array and complex.  BUILD tends to more for
// user-defined types.
typedef void (*FactoryFunction)(const OpalValue& name, 
				const OpalValue& input_tuple, 
				OpalValue& environment,  
				OpalValue& output_result);



#include "m2opallink.h"

// name is __main__\nOpalLink\n
// input_tuple is a table: { path = "linkname" } 
void OpalLinkFactoryFunction (const OpalValue& name, 
			      const OpalValue& input_tuple, 
			      OpalValue& environment,  
			      OpalValue& output_result)
{ 
  // The input tuple is just the actual link
  OpalTable d = UnOpalize(input_tuple, OpalTable);
  string link_path = UnOpalize(d.get("path"), string);
  output_result = OpalValue(new OpalLink(link_path));
}

// Routines for long integer support

// This divides the big-endian repr by a single digit
template <class BI, class I>
I DigitDivide (const I* in_stream, int in_len, I digit,
	       I* div, int& out_len)
{
  BI remainder = 0;

  static const BI placemax = I(~I(0)) + 1; 

  // Zero input
  if (in_len==0) {
    out_len = 0;
    return remainder;
  }

  // First divide may be zero, and we don't want a leading zero.
  int jj = 0;
  BI first = in_stream[0];
  remainder =  first % digit;
  BI first_div = first / digit;
  if (first_div != 0) {
    div[jj++] = first_div;
  }

  // Continue dividing
  for (int ii=1; ii<in_len; ii++) {

    BI input = placemax * remainder + in_stream[ii];
    remainder = input % digit;
    BI divver = input / digit;
   
    div[jj++] = divver;
  }
  out_len = jj;
  return remainder;
}


// Give an unsigned stream of a  base 256 number, print it
string DecimalPrint (const char* const_in, int in_len, 
		     bool big_endian = true)
{  
  // Choose whether input is coming in as big-endian or little-endian
  char* in = new char[in_len];
  if (big_endian) {
    memcpy(in, const_in, in_len);
  } else {
    for (int ii=0; ii<in_len; ii++) {
      in[in_len-ii-1] = const_in[ii];
    }
  }
  // Buffers get swapped routinely
  char* out = new char[in_len];
  int out_len = in_len;
  
  // Continue while we can keep getting a remainder from the division
  Array<char> ares;
  while (out_len>0) {
    int_u1 rem = DigitDivide<int_u2, int_u1>((int_u1*)in, in_len, 10, 
					     (int_u1*)out,out_len);
    ares.append(rem+'0');
    
    char* temp = in;
    in = out;
    out = temp;

    in_len=out_len;
  }
  delete [] out;
  delete [] in;

  // Reverse, wrong order: also trim too many zeros
  char* res = &ares[0];
  int end = ares.length()-1;
  for (; res[end]=='0'; end--) 
    ;  // set end 
  if (end==-1) { 
    // Special case, 0
    return "0";
  } else {
    string result(res, end+1);
    for (int ii=0; ii<end+1; ii++) {
      result[ii] = ares[end-ii];
    }
    return result;
  }
}


// Assuming the stream is 2s complement number, print it!
string SignedDecimalPrint (const char* const_in, int in_len, 
			   bool big_endian=true)
{
  string result;
  if (in_len==0) return "0";

  // Turn into big-endian stream
  char* in = new char[in_len];
  if (big_endian) {
    memcpy(in, const_in, in_len);
  } else {
    for (int ii=0; ii<in_len; ii++) {
      in[in_len-ii-1] = const_in[ii];
    }
  }
  // Assumption: big-endian, sign-bit in in[0]
  if (in[0] & 0x80) { // negative!
    // negate and add 1 to turn into "positive"
    int_2 carry = 1;
    for (int ii=in_len-1; ii>=0; ii--) {
      int_u2 temp = int_u1(~in[ii]) + carry;
      carry = (temp & 0xff00)>>8;
      in[ii] = temp;
    }
    result = "-"+DecimalPrint(in, in_len, true)+"L";
  } else {
    // positive, just print as is
    result = DecimalPrint(in, in_len, true)+"L";
  }

  // Clean up
  delete [] in;
  return result;
}



// Includes needed BEFORE
#include <stdexcept>
using std::runtime_error;
using std::out_of_range;


#include <genericpickleloader.h>

#endif // M2PICKLELOADER_H_


