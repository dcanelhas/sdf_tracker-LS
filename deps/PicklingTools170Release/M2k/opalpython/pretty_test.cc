
#include "m2opalreader.h"

#include <iostream>
using namespace std;

void test (const char* in)
{
  OpalTable env;
  OpalTable ot = GlobalEvaluator::instance().evaluate(env, in);
  ot.prettyPrint(cout);
  cout << "..." << endl;
  PrettyPrintPython(ot, cout);
  cout << "-----------" << endl;
}

int main()
{
  PrettyPrintPython(OpalValue(Number(1.2345678901234567890)), cout);
  cout << endl;
  PrettyPrintPython(OpalValue(Number(3.141592658)), cout);
  cout << endl;
  PrettyPrintPython(OpalValue(Number(1)), cout);
  cout << endl;
  PrettyPrintPython(OpalValue(Number(complex_8(1.2345678901234, -10))), cout);
  cout << endl;
  PrettyPrintPython(OpalValue(Number(complex_16(1.2345678901234, -10))), cout);
  cout << endl;
  PrettyPrintPython(OpalValue(Number(M2Duration(100))), cout);
  cout << endl;
  PrettyPrintPython(OpalValue(Number(M2Time(100))), cout);
  cout << endl;
  PrettyPrintPython(OpalValue(bool(1)), cout);
  cout << endl;
  PrettyPrintPython(OpalValue(bool(0)), cout);
  cout << endl;

  OpalHeader oh;
  PrettyPrintPython(oh, cout);


  test("{1,2,3}");
  test("{a=1, b=2}");
  test("{t=12345678.90123456789, b=f:3.14159265}");
  test("{(1,2)}");
  test("{(1,-2)}");
  test("{\"the quick brown\"=100000000 }");
  test("{\"the quick brown\"=UX:10000000000000 }");
  test("{\"the quick brown\"=UX:100000000000000000 }");
  test("{\"the \\\"quick\\\" brown\"= b:1}");
  test("{\"the '\\\"quick\\\"' brown\"= \"hello\"}");
  test("{a=d:<1,2,3>}");
  test("{a=f:<1,2,3>}");
  test("{a=b:<1,2,3>}");
  test("{a=ub:<1,2,3>}");
  test("{a=i:<1,2,3>}");
  test("{a=ui:<1,2,3>}");
  test("{a=l:<1,2,3>}");
  test("{a=ul:<1,2,3>}");
  test("{a=x:<1,2,3>}");
  test("{a=ux:<1,2,3>}");
  test("{a=t:<1,2,3>}");
  test("{a=DUR:<1,2,3>}");  

  {
    TimePacket tp;
    OpalValue otp = OpalValue(tp);
    PrettyPrintPython(otp, cout);
  }

  {
    EventData tp;
    OpalValue otp = OpalValue(tp);
    PrettyPrintPython(otp, cout);
  }

  test("{a=1, b={d=d:<456.6789123456789, 99999999999999>, f=f:<1.23456789123>}, c={1,2,3, \"abc\", {1,2,3}}}");
  test("{a=1, b={f=f:<456.6789123456789, 99999999999999>, f2=f:<1.23456789123>}, c={1,2,3, \"abc\", {1,2,3}}}");


  const char * b = 
    "{ 'a':1, 'b':2.2, 'c':'three',"
    "  'nest': { 1:True, 2: False}, "
    "  'big':1111111111111,"
    "  'bigger':111111111111111111111111111111111111,"
    "  'longint': 100L," 
    "  'a1': array([1,2,3,4,5,6,7,8,9,10], '1'), "
    "  'a2': array([1,2,3,4,5,6,7,8,9,10], 'b'), "
    "  'a3': array([1,2,3,4,5,6,7,8,9,10], 's'), "
    "  'a4': array([1,2,3,4,5,6,7,8,9,10], 'w'), "
    "  'a5': array([1,2,3,4,5,6,7,8,9,10], 'i'), "
    "  'a6': array([1,2,3,4,5,6,7,8,9,10], 'u'), "
    "  'a7': array([1,2,3,4,5,6,7,8,9,10], 'l'), "
    "  'a8': array([1,2,3,4,5,6,7,8,9,10], 'f'), "
    "  'a9': array([1,2,3,4,5,6,7,8,9,10], 'd'), "
    "  'aa': array([1,2,3,4,5,6,7,8,9,10], 'F'), "
    "  'ab': array([1,2,3,4,5,6,7,8,9,10], 'D'), "
    "  'e': { }, "
    "  'd': [], "
    "  'f': [ [], {}, [], [[]], {0:[{}]} ], "
    "  'g': [ [1,2,3], {}, [] ], "
    "  'h': [ [1,2,3], {}, [] ], "
    "  'i': [ '\x20'], "
    "  'j': [ '\\x00\\x01\\'\"\\n\\r'], "
    "  'k': [ \"abc'123\" ], "
    "  'l': o{ 'a':1, 'b':2 },"
    "  'm': (),"
    "  'n': (1),"
    "  'o': (1,),"
    "  'p': (1,2.2,'three'),"
    "  'q1': OrderedDict([('a', 1),('b', 2)]),"
    "  'q2': OrderedDict([]),"
    "  'q3': OrderedDict([('a',1)]),"
    "  'q4': OrderedDict([]),"
    "}";
  OpalValue oov;
  istringstream is(b);
  ReadOpalValueFromStream(is, oov);
  cout << "Python dict:" << b << endl;
  oov.prettyPrint(cout);
  cout << endl;

  PrettyPrintPython(oov, cout);
}
