

#include "m2pickleloader.h"
#include <limits>
#include "m2globalevaluator.h"

#include "m2opalpython.h"


OpalValue Eval(const char* c)
{
  OpalTable env;
  return GlobalEvaluator::instance().evaluate(env, c); 
}

// Testing of the new P0 and P2 Loader routines: just about as fast,
// but significantly easier to maintain and read.
OpalValue Loading (const char* str, const OpalValue& expected, int len=-1, 
		   FactoryFunction ssf = 0, string*name = 0)
{
  OpalValue v;
  if (len==-1) len = strlen(str);
  PickleLoaderImpl<OpalValue> pp(str, len);
  OpalValue& ov = pp.env();
  EXTRACT_DICT(ot, ov);
  DICT_PUT(ot, "supportsNumeric", true); // (array_disposition==AS_NUMERIC);
  if (ssf) pp.registerFactory(*name, ssf);
  pp.loads(v);
  
  if (v.type()!=expected.type()) {
    cout << "Oops! Expecting different tag: saw '" << expected.type() 
	 << "' but expected '" << v.type() << "'" << endl;
  }

  if (v.type()==OpalValueA_NUMBER && expected.type()==OpalValueA_NUMBER) {
    Number n1 = UnOpalize(v, Number);
    Number n2 = UnOpalize(expected, Number);
    if (n1.type() != n2.type()) {
      cout << "Oops! Expecting different tag: saw '" << n1.type()
	   << "' but expected '" << n2.type() << "'" << endl;
    }
  }

  if (v.type()==OpalValueA_VECTOR && expected.type()==OpalValueA_VECTOR) {
    Vector v1 = UnOpalize(v, Vector);
    Vector v2 = UnOpalize(expected, Vector);
    if (v1.type() != v2.type()) {
      cout << "Oops! Expecting different tag: saw '" << v1.type()
	   << "' but expected '" << v2.type() << "'" << endl;
    }
  }

  if (v.type()==OpalValueA_TABLE && expected.type()==OpalValueA_TABLE) {
    OpalTable v1 = UnOpalize(v, OpalTable);
    OpalTable v2 = UnOpalize(expected, OpalTable);
    if (v1.type() != v2.type()) {
      cout << "Oops! Expecting different tag: saw '" << v1.implementation().type()
	   << "' but expected '" << v2.implementation().type() << "'" << endl;
    } else {
      cout << "matching " << v1.implementation().type() << endl;
    }
  }

  if (v!=expected) {
    cout << "Oops! Expected:" << endl << expected << endl << " but saw " 
	 << endl << v << endl;
    if (str!=string("Fnan\n.")) {
	throw MidasException("*******ERROR***********");
    }
  } else {
    cout << " ... okay: '" << v.type();
    if (v.type()==OpalValueA_VECTOR) {
      Vector v1 = UnOpalize(v, Vector);
      cout << v1.type();
    }
    cout << "'" << expected << endl;
  }
  return v;
}


void Dumping (const char* mesg, const OpalValue& test_thing)
{
  {
    cout << mesg << "P0" << endl;
    Array<char> aaa(1024);
    PythonBufferPickler<OpalValue> pbp(aaa, AS_NUMPY);
    pbp.dump(test_thing);
    //cout << aaa << endl;
    Loading(aaa.data(), test_thing, aaa.length());
  }
  {
    cout << mesg << "P2" << endl;
    int bytes = P2TopLevelBytesToDumpOpal(test_thing, AS_NUMPY);
    char* start = new char[bytes];
    char* end = P2TopLevelDumpOpal(test_thing, start, AS_NUMPY);
    int len = end - start;
    Loading(start, test_thing, len);
    delete [] start;
  }

}


int main ()
{
  cout << "***Make sure we can load integers for Protocol 0" << endl;
  Loading("I0\n.", OpalValue(Number(int_8(0))));
  Loading("I1\n.", OpalValue(Number((int_8(1)))));
  Loading("I-1\n.", OpalValue(Number((int_8(-1)))));
  Loading("I255\n.", OpalValue(Number((int_8(255)))));
  Loading("I-255\n.", OpalValue(Number((int_8(-255)))));
  Loading("I32768\n.", OpalValue(Number((int_8(32768)))));
  Loading("I-32768\n.", OpalValue(Number((int_8(-32768)))));

  cout << "**Make sure we can do LONG integers for Protocol 0" << endl;  
  Loading("L10L\n", OpalValue(string("10L"))); 
  Loading("L1L\n", OpalValue(string("1L"))); 
  Loading("L0L\n", OpalValue(string("0L"))); 
  Loading("L-1L\n", OpalValue(string("-1L"))); 
  Loading("L-10L\n", OpalValue(string("-10L"))); 
  const char* q1 = "-340282366920938463463374607431768211456";
  Loading("L340282366920938463463374607431768211456L\n", 
	  //OpalValue(StringToBigInt(q1+1, strlen(q1+1)))); 
	  OpalValue(string(q1+1, strlen(q1+1))+"L")); 
  Loading("L-340282366920938463463374607431768211456L\n", 
	  OpalValue(string(q1, strlen(q1))+"L")); 



  cout << "**Make sure we can do string for P0" << endl;
  Loading("S'abc'\np1\n.", OpalValue("abc"));
  Loading("S''\n.", OpalValue(""));
  Loading("S'\\x00'\n.", OpalValue(string("\0", 1)), 8);
  Loading("S'\\x00\\x01\\n'\n.", OpalValue(string("\0\001\n", 3)), 15);
  Loading("S'\\x00\\x01\\n\"'\n.", OpalValue(string("\0\001\n\"", 4)), 15);
  Loading("S'\\''\n.", OpalValue(string("'", 1)), 7);
  Loading("S'\\\"'\n.", OpalValue(string("\"", 1)), 7);
  Loading("S'\"Don\\'t!\"'\n.", OpalValue(string("\"Don't!\"")));
  Loading("S'\\xfe\\xcd\\xab\\x98\\x76\\x54\\x32\\x10'\n.", 
	  OpalValue(string("\xfe\xcd\xab\x98\x76\x54\x32\x10")));

  cout << "**Make sure we can do reals for P0" << endl;
  Loading("F1.1\n.", OpalValue(Number(real_8(1.1))));
  Loading("F1.23456789123456\n.", OpalValue(Number(real_8(1.23456789123456))));
  Loading("F1.234567891234567\n.", OpalValue(Number(real_8(1.234567891234567))));
  Loading("F1.2345678912345678\n.", OpalValue(Number(real_8(1.2345678912345678))));
  Loading("F-1.2\n.", OpalValue(Number(real_8(-1.2))));
  Loading("F0\n.", OpalValue(Number(real_8(0.0))));
  Loading("F1.2384589726357624e+18\n.", OpalValue(Number(real_8(1238458972635762345.0))));

  real_8 ttt = std::numeric_limits<double>::infinity();
  Loading("Finf\n.", OpalValue(Number(ttt)));
  Loading("F-inf\n.", OpalValue(Number(real_8(-std::numeric_limits<double>::infinity()))));
  Loading("Fnan\n.", OpalValue(Number(real_8(-std::numeric_limits<double>::quiet_NaN()))));
  cout << " ... we actually expect nan != nan, so this error okay" << endl;


  cout << "**Make sure we can do complexes for P0" << endl;
  Loading("c__builtin__\ncomplex\np1\n(F1\nF2\ntRp2\n.", OpalValue(Number(complex_16(1,2))));
  Loading("c__builtin__\ncomplex\np1\n(F-827345872.78634787\nF2394750234\n"
	  "tRp2\n.", OpalValue(Number(complex_16(-827345872.78634787, 2394750234.0))));
  Loading("c__builtin__\ncomplex\np1\n(F0\nF-1\ntRp2\n.", OpalValue(Number(complex_16(0,-1))));
  Loading("c__builtin__\ncomplex\np1\n(F0\nF0\ntRp2\n.", OpalValue(Number(complex_16(0,0))));

  cout << "**Make sure we can do tuples for P0" << endl;
  Loading("(t.", OpalValue(OpalTable()));

  OpalTable tup1;
  tup1.append(OpalValue(Number(int_8(1))));
  Loading("(I1\nt.", tup1); // Tup(1));

  OpalTable tup12;
  tup12.append(OpalValue(Number(int_8(1))));
  tup12.append(OpalValue(Number(int_8(2))));
  Loading("(I1\nI2\nt.", tup12); // Tup(1,2));

  OpalTable tup123;
  tup123.append(OpalValue(Number(int_8(1))));
  tup123.append(OpalValue(Number(real_8(2.2))));
  tup123.append(OpalValue("three"));
  Loading("(I1\nF2.2000000000000002\nS'three'\np1\nt.", tup123); // Tup(1,2.2,"three"));

  OpalTable tup1234;
  tup1234.append(OpalValue(OpalTable()));
  { 
    OpalTable sub; 
    sub.append(OpalValue(Number(int_8(1))));
    sub.append(OpalValue(Number(int_8(2))));
    sub.append(OpalValue(OpalTable()));
    sub.append(OpalValue(Number(int_8(3))));
    tup1234.append(sub);    
  }
  tup1234.append(OpalValue(OpalTable()));
  tup1234.append(OpalValue(Number(int_8(4))));
  Loading("((t(I1\nI2\n(tI3\nt(tI4\ntp1\n.", tup1234);
                                 // Tup(Tup(), Tup(1,2, Tup(), 3), Tup(), 4));


  OpalTable ttup;
  ttup.append(OpalValue(Number(int_8(1))));
  ttup.append(tup1);
  Loading("(I1\n(I1\ntp1\nt.", ttup); //Tup(1, Tup(1)));

  ttup.append(tup1);
  Loading("(I1\n(I1\ntp1\n(I1\ntp2\nt.", ttup); // Tup(1, Tup(1), Tup(1)));

  cout << "**Make sure we can do dict for P0" << endl;
  Loading("(d.", OpalTable());


  OpalTable d123;
  d123.put("a", OpalValue(Number(int_8(1))));
  d123.put("b", OpalValue(Number(real_8(2.2))));
  d123.put("c", OpalValue("ree"));
  Loading("(dp1\nS'a'\nI1\nsS'c'\nS'ree'\np2\nsS'b'\nF2.2000000000000002\ns.", 
	  d123);// Tab("{'a':1, 'b':2.2, 'c':'ree'}"));


  OpalTable d1;
  d1.put("a", OpalValue(Number(int_8(1))));
  Loading("(dp1\nS'a'\nI1\ns.", d1); // Tab("{'a':1}"));

  OpalTable d12;
  d12.put("a", OpalValue(Number(int_8(1))));
  {
    OpalTable sub;
    sub.put("n", OpalValue(Number(int_8(6))));
    d12.put("b", sub);
  }
  Loading("(dp1\nS'a'\nI1\nsS'b'\n(dp2\nS'n'\nI6\nss.", 
	  d12); // Tab("{'a':1, 'b':{'n':6}}"));


  cout << "**Make sure we can do None for P0" << endl;
  Loading("N.", NONE_VALUE);


  cout << "**Make sure we can do list for P0" << endl;
  Loading("(l.", OpalValue(OpalTable(OpalTableImpl_GRAPHARRAY)));


  OpalTable e0(OpalTableImpl_GRAPHARRAY);
  OpalTable e1(OpalTableImpl_GRAPHARRAY);
  e1.append(e0);
  OpalTable e2(OpalTableImpl_GRAPHARRAY);
  e2.append(e0);
  e2.append(e0);
  OpalTable e3(OpalTableImpl_GRAPHARRAY);
  OpalTable e4(OpalTableImpl_GRAPHARRAY);
  OpalTable e(OpalTableImpl_GRAPHARRAY);
  e.append(e0);
  e.append(e2);
  e.append(e1);
  e.append(e0);
  Loading("(lp1\n(lp2\na(lp3\n(lp4\na(lp5\naa(lp6\n(lp7\naa(lp8\na.", e);
        //Arr( "[ [], [[],[]], [[]], [] ]"));

  OpalTable ep(OpalTableImpl_GRAPHARRAY);
  ep.append(OpalValue(Number(int_8(1))));
  ep.append(OpalValue(Number(int_8(2))));
  ep.append(OpalValue(Number(int_8(3))));
  Loading("(lp1\nI1\naI2\naI3\na.", ep); // Arr("[1,2,3]"));


  OpalValue ee = 
    Eval("{ {\"a\"=x:1}, {\"b\"= {}}, {x:1,x:2}, (1,2), 17.2348979283}");
  Loading("(lp1\n(dp2\nS'a'\nI1\nsa(dp3\nS'b'\n(lp4\nsa(lp5\nI1\naI2\naa"
	  "c__builtin__\ncomplex\np6\n(F1\nF2\ntRp7\naF17.234897928300001\n"
	  "a.", 
	  ee); // Arr("[ {'a':1}, {'b':[]}, [1,2], (1+2j), 17.2348979283 ]"));


  cout << "**Make sure we can do longs for P2" << endl;
  Loading("\x8a\x01\x01.", OpalValue(string("1L")));
  Loading("\x8a\x01\xff.", OpalValue(string("-1L")));
  Loading("\x8a\x01\x7f.", OpalValue(string("127L")));
  Loading("\x8a\x02\x81\x00.", OpalValue(string("129L")), 5);
  Loading("\x8a\x08\x15\x81\xe9}\xf4\x10\"\x11.", OpalValue(string("1234567890123456789L")));
  const char big1[] = "32317006071311007300714876688669951960444102669715484"
    "03213034542752465513886789089319720141152291346368871796092189801949411"
    "95591504909210950881523864482831206308773673009960917501977503896521067"
    "96057638384067568276792218642619756161838094338476170470581645852036305"
    "04288757589154106580860755239912393038552191433338966834242068497478656"
    "45694948561760353263220580778056593310261927084603141502585928641771167"
    "25943603718461857357598351152301645904403697613233287231227125684710820"
    "20972515710172693132346967854258065669793504599726835299863821552516638"
    "9437335543602135433229604645318478604952148193555853611059596230656";
  Loading("\x8b\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	  "\x00\x00\x00\x00\x00\x01.",
	  OpalValue(string(big1)+"L"),263);


  cout << "**Make sure we can put gets and puts with P0" << endl;
  OpalValue same = OpalTable(OpalTableImpl_GRAPHARRAY); //Val same = new Arr();
  OpalTable aa = OpalTable(OpalTableImpl_GRAPHARRAY); // Val aa = Arr();
  aa.put(0, same);
  aa.put(1, same);
  OpalValue l = Loading("(lp1\n(lp2\nag2\na.", aa);
  OpalTable ll = UnOpalize(l, OpalTable);
  OpalValue l1 = ll.get(0);
  OpalValue l2 = ll.get(1);
  cout << l << l1 << l2 << endl;
  //cout << l1.type() << l1.implementation().type() << l1.isproxy << endl;
  //cout << l2.type() << l2.implementation().type() << l2.isproxy << endl;
  //cout << is(l1, l2) << endl;

  cout << "**Make sure the Protocol note works for P2" << endl;
  Loading("\x80\x02K\x01.", OpalValue(Number(int_u1(1))));



  cout << "***Make sure we can load all the integers for Protocol 2" << endl;
  Loading("K\001", OpalValue(Number(int_u1(1))));
  Loading("K\x7f", OpalValue(Number(int_u1(127))));
  Loading("K\x80", OpalValue(Number(int_u1(128))));
  Loading("K\xff", OpalValue(Number(int_u1(255))));
  Loading("M\x00\x01", OpalValue(Number(int_u2(256))), 3);
  Loading("M\xff\x7f", OpalValue(Number(int_u2(32767))), 3);
  Loading("M\x00\x80", OpalValue(Number(int_u2(32768))), 3);
  Loading("M\xff\xff", OpalValue(Number(int_u2(65535))), 3);
  Loading("J\x00\x00\x01\x00", OpalValue(Number(int_4(65536))), 5);
  Loading("J\x00\x00\xff\xff", OpalValue(Number(int_4(-65536))), 5);
  Loading("J\xff\xff\xff\x7f", OpalValue(Number(int_4(2147483647))), 5);
  Loading("J\x00\x00\x00\x80", OpalValue(Number(int_4(-2147483648LL))), 5);
  
  cout << "***Misc" << endl;
  Loading("N", NONE_VALUE);
  Loading("}", OpalValue(OpalTable()));
  Loading("]", OpalValue(OpalTable(OpalTableImpl_GRAPHHASH)));
  Loading("\x88", OpalValue(true));
  Loading("\x89", OpalValue(false));

  cout << "***Strings for Protocol 2" << endl;
  Loading("U\03abc", OpalValue("abc"));
  Loading("U\0", OpalValue(""), 2);


  string result;
  //for (int ii=0; ii<255; ii++) result.append("*");
  result.append(255, '*');
  string to_load = "U\xff" + result;
  Loading(to_load.data(), OpalValue(result));

  result+="*";
  to_load = string("T\x00\x01\x00\x00",5)+result;
  Loading(to_load.data(), OpalValue(result), result.length()+5);


  cout << "***tuples for Protocol 2" << endl;
  Loading(")", OpalValue(OpalTable())); // Tup());

  OpalTable o1;
  o1.append(OpalValue(Number(int_u1(1))));
  Loading("K\001\x85", o1); // Tup(1));

  OpalTable o12;
  o12.append(OpalValue(Number(int_u1(1))));
  o12.append(OpalValue(Number(int_u1(2))));
  Loading("K\001K\002\x86", o12); // Tup(1,2));

  OpalTable o123;
  o123.append(OpalValue(Number(int_u1(1))));
  o123.append(OpalValue(Number(int_u1(2))));
  o123.append(OpalValue(Number(int_u1(3))));
  Loading("K\001K\002K\003\x87", o123); // Tup(1,2,3));

  OpalTable o1234;
  o1234.append(OpalValue(Number(int_u1(1))));
  o1234.append(OpalValue(Number(int_u1(2))));
  o1234.append(OpalValue(Number(int_u1(3))));
  o1234.append(OpalValue(Number(int_u1(4))));
  Loading("(K\001K\002K\003K\004t", o1234); // Tup(1,2,3,4));



  cout << "*** nastier tuples" << endl;
  OpalTable o45;
  o45.append(OpalValue(Number(int_u1(4))));
  o45.append(OpalValue(Number(int_u1(5))));
  o123.append(OpalValue(o45));
  Loading("(K\001K\002K\003K\004K\005\x86t", o123); // Tup(1,2,3,Tup(4,5)));

  OpalTable o6;
  o6.append(OpalValue(Number(int_u1(6))));
  OpalTable o7890;
  o7890.append(OpalValue(Number(int_u1(7))));
  o7890.append(OpalValue(Number(int_u1(8))));
  o7890.append(OpalValue(Number(int_u1(9))));
  o7890.append(OpalValue(Number(int_u1(10))));
  o123.append(o6);
  o123.append(o7890);
  Loading("(K\001K\002K\003K\004K\005\x86K\006\x85(K\007K\010K\011K\012tt", 
	  o123); // Tup(1,2,3,Tup(4,5),Tup(6),Tup(7,8,9,10)));

  cout << "*** dictionaries" << endl;
  Loading("}U\001aK\001s", Eval("{\"a\"=UB:1}")); // Tab("{'a':1}"));
  Loading("}(U\001aK\001U\001bK\002u", Eval("{\"a\"=UB:1, \"b\"=UB:2}")); // Tab("{'a':1, 'b':2}"));
  Loading("}(U\001aK\001U\001bK\002U\001cK\003u", 
	  Eval("{\"a\"=UB:1, \"b\"=UB:2, \"c\"=UB:3}")); // Tab("{'a':1, 'b':2, 'c':3}"));


  cout << "*** nested dictionaries" << endl;
  OpalValue temp = Eval("{\"a\"={}, \"b\"={\"N\"=UB:8}, \"c\"={\"O\"=\"None\", \"P\"={UB:1,UB:2,UB:3}}}");
  OpalTable ot = UnOpalize(temp, OpalTable);
  ot.put("c.O", NONE_VALUE);
  OpalValue tt = ot;
  Loading(
    "}(U\001a}U\001c}(U\001PK\001K\002K\003\x87U\001ONuU\001b}U\001NK\010su", 
    tt);
     //Tab("{'a':{}, 'b':{'N':8}, 'c':{'O':None, 'P':(1,2,3)}}"));

  OpalTable gg;
  gg.put("5", OpalTable());
  gg.put("1", OpalTable());
  gg.put("2", OpalTable());
  Loading("}(K\005}K\001}K\002}u", gg); 
  //Tab("{5:{},1:{},2:{}}"));


  cout << "*** real numbers" << endl;
  string dbl = string("G?\xf0\x00\x00\x00\x00\x00\x00",9);
  Loading(dbl.c_str(), OpalValue(Number(real_8(1.0))), dbl.length());
  dbl = string("G\xbf\xf0\x00\x00\x00\x00\x00\x00",9);
  Loading(dbl.c_str(), OpalValue(Number(real_8(-1.0))), dbl.length());
  dbl = string("G@Y\x10\x00\x00\x00\x00\x00",9);
  Loading(dbl.c_str(), OpalValue(Number(real_8(100.25))), dbl.length());


  cout << "*** complex numbers" << endl;
  Loading("c__builtin__\ncomplex\nG?\xf0\x00\x00\x00\x00\x00\x00G"
	  "@\x00\x00\x00\x00\x00\x00\x00\x86R", OpalValue(Number(complex_16(1,2))), 41);
  Loading("c__builtin__\ncomplex\nG\xbf\xf8\x00\x00\x00\x00\x00\x00G"
	  "\xbf\xe0\x00\x00\x00\x00\x00\x00\x86R", OpalValue(Number(complex_16(-1.5,-.5))), 41);


  cout << "*** lists" << endl;
  Loading("]", OpalTable(OpalTableImpl_GRAPHARRAY));
  Loading("]K\001a", o1); // Arr("[1]"));
  Loading("](K\001K\002e", o12); // Arr("[1,2]"));

  OpalTable l123;
  l123.append(OpalValue(Number(int_u1(1))));
  l123.append(OpalValue(Number(int_u1(2))));
  l123.append(OpalValue(Number(int_u1(3))));
  Loading("](K\001K\002K\003e", l123); // Arr("[1,2,3]"));


  cout << "*** nested lists for P2" << endl;
  Loading("](K\x01K\x02](K\x03K\x04]K\005aK\x06K\x07K\x08\x87]]ae]]K\001ae", 
	  Eval("{UB:1, UB:2, {UB:3, UB:4, {UB:5}, {UB:6,UB:7,UB:8}, {{}}}, {}, {UB:1}}")); 
  // Arr("[1,2,[3,4,[5],(6,7,8),[[]]],[],[1]]"));


  cout << "*** orderered dicts for P0" << endl;
  Loading("ccollections\nOrderedDict\np1\n((ltRp2\n", OpalTable()); // OTab());
  Loading("ccollections\nOrderedDict\np1\n((lp2\n(lp3\nS'a'\naI1\naatRp4\n.", 
	  Eval("{\"a\"=X:1}")); // OTab("o{'a':1}"));
  Loading("ccollections\nOrderedDict\np1\n((lp2\n(lp3\nS'a'\naI1\naa(lp"
	  "4\nS'b'\naI2\naatRp5\n.", 
	  Eval("{\"a\"=X:1, \"b\"=X:2}")); // Tab("{'a':1, 'b':2}"));
                                             // OTab("o{'a':1, 'b':2}"));


  cout << "*** orderered dicts for P2" << endl;
  Loading("ccollections\nOrderedDict\n]\x85R", OpalTable()); // OTab());
  Loading("ccollections\nOrderedDict\n]](U\001aK\001ea\x85R", 
	  Eval("{\"a\"=UB:1}"));
	  //OTab("o{'a':1}"));
  Loading("ccollections\nOrderedDict\n](](U\001aK\001e](U\001bK\002ee\x85R", 
	  Eval("{\"a\"=UB:1, \"b\"=UB:2}")); // Tab("{'a':1, 'b':2}"));
                                             //OTab("o{'a':1, 'b':2}"));

  cout << "*** proxies and replicated data (puts/gets)" << endl;
  OpalValue a = OpalTable(OpalTableImpl_GRAPHARRAY); // Val a = new Arr();
  string sp = "]q\001h\001\x86";
  OpalTable ttuple;
  ttuple.append(a);
  ttuple.append(a);
  Loading(sp.c_str(), ttuple); // Tup(a,a));
  PickleLoaderImpl<OpalValue> pp(sp.c_str(), sp.length());
  OpalValue res;
  pp.loads(res);
  cout << res << endl;
  // Don't "really' have puts and gets, because everything is copied by
  // COW (copy on write) so it's a pointer until a write is requested
  //cout << IsProxy(res[0]) << IsProxy(res[1]) << is(res[0], res[1]) << endl;
  
  a = Eval("{ (1,2), (3,4) }"); // Tup(complex_16(1,2), complex_16(3,4));
  Loading("c__builtin__\ncomplex\nq\x01G?\xf0\x00\x00\x00\x00\x00\x00G@"
	  "\x00\x00\x00\x00\x00\x00\x00\x86Rq\x02h\x01G@\x08\x00\x00\x00"
	  "\x00\x00\x00G@\x10\x00\x00\x00\x00\x00\x00\x86Rq\x03\x86", a, 70);


  OpalValue r1=Eval("{UB:1,{UB:2}, \"three\"}"); //Val r1=new Arr("[1,(2,),'three']");
  OpalValue r2=Eval("{\"a\"=UB:1, \"b\"=UB:2}");//Val r2=new Tab("{'a':1, 'b':2 }");
  OpalValue r3=Eval("{UB:1,{UB:2,UB:2},\"three\"}");//Val r3=new Tup(1,Tup(2,2),"three");
  OpalTable rr;// Val rr = new Tup(r1, r1, r2, r2, r3, r3, r3);
  rr.append(r1);
  rr.append(r1);
  rr.append(r2);
  rr.append(r2);
  rr.append(r3);  
  rr.append(r3);  
  rr.append(r3);  
  const char* sss = 
    "(]q\001(K\001K\002\x85q\002U\005threeq\003eh\001}q\004(U\001aK"
    "\001U\001bK\002uh\004K\001K\002K\002\x86h\003\x87q\005h\005h\005"
    "tq\006";
  Loading(sss, rr);

  cout << "*** Numeric arrays: P0 and P2" << endl;
  OpalValue a1=Eval("B:<1,2,3>");
  //Array<int_1> a1(3); 
  //for (int ii=0; ii<3; ii++) {
  //  a1.append(ii+1);
  //}
  Loading("cNumeric\narray_constructor\nq\x01(K\x03\x85U\0011U\x03\x01\x02"
	  "\x03\x88tRq\x02", a1);
  Loading("\x80\002cNumeric\narray_constructor\nq\001(K\003\x85U\0011U\x03\x01\x02\x03K\x01tRq\002.",
	  a1, 50);

  OpalValue au1 = Eval("UB:<1,2,3>");
  //Array<int_u1> au1(3);
  //for (int ii=0; ii<3; ii++) {
  //  au1.append(ii+1);
  //}
  Loading("cNumeric\narray_constructor\nq\x01(K\x03\x85U\001bU\x03\x01\x02"
	  "\x03\x88tRq\x02", au1);
  Loading("\x80\002cNumeric\narray_constructor\nq\001(K\003\x85U\001bU\x03\x01\x02\x03K\x01tRq\x02.", 
	  au1, 50);

  OpalValue a2=Eval("I:<1,2,3>");
  //Array<int_2> a2(3);
  //for (int ii=0; ii<3; ii++) {
  //  a2.append(ii+1);
  //}
  Loading("cNumeric\narray_constructor\nq\x01(K\x03\x85U\001sU\x06\x01\x00"
	  "\x02\x00\x03\x00K\x01tRq\x02", a2, 50);
  Loading("\x80\002cNumeric\narray_constructor\nq\001(K\x03\x85U\001sU\x06\x01\x00\x02\x00\x03\x00K\001tRq\002.", 
	  a2, 53);

  OpalValue au2=Eval("UI:<1,2,3>");
  //Array<int_u2> au2(3);
  //for (int ii=0; ii<3; ii++) {
  //  au2.append(ii+1);
  //}
  Loading("cNumeric\narray_constructor\nq\x01(K\x03\x85U\x01wU\x06\x01\x00"
	  "\x02\x00\x03\x00K\x01tRq\x02", au2, 50);
  Loading("\x80\002cNumeric\narray_constructor\nq\x01(K\x03\x85U\x01wU\x06"
	  "\x01\x00\x02\x00\x03\x00K\x01tRq\x02.",
	  au2, 53);

  OpalValue a4=Eval("L:<1,2,3>");
  //Array<int_4> a4(3);
  //for (int ii=0; ii<3; ii++) {
  //  a4.append(ii+1);
  //}
  Loading("cNumeric\narray_constructor\nq\x01(K\x03\x85U\x01iU\x0c\x01\x00"
	  "\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00K\x01tRq\x02", a4,56);
  Loading("\x80\002cNumeric\narray_constructor\nq\x01(K\x03\x85U\x01iU"
	  "\x0c\x01\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00K\x01tRq\x02.", 
	  a4, 59);

  OpalValue au4=Eval("UL:<1,2,3>");
  //Array<int_u4> au4(3);
  //for (int ii=0; ii<3; ii++) {
  //  au4.append(ii+1);
  //}
  Loading("cNumeric\narray_constructor\nq\x01(K\x03\x85U\x01uU\x0c\x01\x00"
	  "\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00K\x01tRq\x02", au4,56);
  Loading("\x80\002cNumeric\narray_constructor\nq\x01(K\x03\x85U\x01uU\x0c\x01\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00K\x01tRq\x02.",
	  au4, 59);

  OpalValue a8=Eval("X:<1,2,3>");
  //Array<int_8> a8(3);
  //for (int ii=0; ii<3; ii++) {
  //  a8.append(ii+1);
  //}
  Loading("cNumeric\narray_constructor\nq\x01(K\x03\x85U\x01lU\x18\x01\x00"
	  "\x00\x00\x00\x00\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x03\x00"
	  "\x00\x00\x00\x00\x00\x00K\x01tRq\x02", a8, 68);
  Loading("\x80\002cNumeric\narray_constructor\nq\x01(K\x03\x85U\x01lU\x18"
	  "\x01\x00\x00\x00\x00\x00\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00"
	  "\x03\x00\x00\x00\x00\x00\x00\x00K\x01tRq\x02.", 
	  a8, 71);


  // Numeric doesn't seem to dist. 8 unsigned and signed
  OpalValue au8=Eval("UX:<1,2,3>");

  OpalValue ar4=Eval("F:<1,2,3>");
  //Array<real_4> ar4(3);
  //for (int ii=0; ii<3; ii++) {
  //  ar4.append(ii+1);
  //}
  Loading("cNumeric\narray_constructor\nq\001(K\x03\x85U\001fU\x0c\x00\x00"
	  "\x80?\x00\x00\x00@\x00\x00@@K\x01tRq\x02", ar4, 56);

  OpalValue ar8=Eval("D:<1,2,3>");
  //Array<real_8> ar8(3);
  //for (int ii=0; ii<3; ii++) {
  //  ar8.append(ii+1);
  //}
  Loading("cNumeric\narray_constructor\nq\x01(K\x03\x85U\001dU\x18\x00"
	  "\x00\x00\x00\x00\x00\xf0?\x00\x00\x00\x00\x00\x00\x00@\x00"
	  "\x00\x00\x00\x00\x00\x08@K\x01tRq\x02", ar8, 68);

  OpalValue c8=Eval("CF:<1,2,3>");
  //Array<complex_8> c8(3);
  //for (int ii=0; ii<3; ii++) {
  //  c8.append(ii+1);
  //}
  Loading("cNumeric\narray_constructor\nq\x01(K\x03\x85U\001FU\x18\x00"
	  "\x00\x80?\x00\x00\x00\x00\x00\x00\x00@\x00\x00\x00\x00\x00"
	  "\x00@@\x00\x00\x00\x00K\x01tRq\x02",
	  c8, 68);

  OpalValue c16=Eval("CD:<1,2,3>");
  //Array<complex_16> c16(3);
  //for (int ii=0; ii<3; ii++) {
  //  c16.append(ii+1);
  //}
  Loading("cNumeric\narray_constructor\nq\x01(K\x03\x85U\001DU0\x00\x00\x00"
	  "\x00\x00\x00\xf0?\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	  "\x00\x00\x00@\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
	  "\x00\x08@\x00\x00\x00\x00\x00\x00\x00\x00K\x01tRq\x02", c16, 92);


  OpalTable bigt;
  bigt.append(a1);
  bigt.append(au1);
  bigt.append(a1);
  OpalValue big = bigt; // new Tup(a1, au1, a1);
  Loading("cNumeric\narray_constructor\nq\x01(K\x03\x85U\0011U\x03\x01\x02"
	  "\x03K\x01tRq\x02h\x01(K\x03\x85U\001bU\x03\x01\x02\x03K\x01tR"
	  "q\x03h\x02\x87", big, 70);


  cout << "*** Functions with no registered reducer" << endl;
  Loading("c__main__\nJacob\nq\x01}q\x02U\x01xKds\x85R", 
	  Eval("{\"__main__\nJacob\n\", {{\"x\"=UB:100}}}"));
  //     Tup("__main__\nJacob\n", Tup(Tab("{'x':100}"))));

  //Loading("(c__main__\nScott\nq\x01oq\x02}q\x03U\x01xKdsb", 
  //	  Tup("__main__\nScott\n", Tup(Tab())));


  cout << "** Functions with no registered BUILD function" << endl;
  Loading("\x80\002c__main__\nScott\nq\001)\x81q\002}q\003U\004dataq\004Kdsb.",
	  Eval("{{\"__main__\nScott\n\", {}}, {\"data\"=UB:100}}"));
   	       //Eval("(('__main__\nScott\n', ()),{'data':100})"));
  
  cout << "** Functions with registered BUILD functions" << endl;
  string name = "__main__\nScott\n";
  Loading("\x80\002c__main__\nScott\nq\001)\x81q\002}q\003U\004dataq\004Kdsb.",
	  OpalValue(Number(int_u1(100))), -1,  
	  BuildScottFactoryFunction, &name);

  cout << "**P0 doesn't intercept this just yet, but at least it parses ..." 
       << endl;
  OpalValue com=Eval("{{\"copy_reg\n_reconstructor\n\", {\"__main__\nScott\n\",\"__builtin__\nobject\n\", \"None\"}}, {\"data\"=X:100}}");
  OpalTable ooo = UnOpalize(com, OpalTable);
  ooo.put("0.1.2", NONE_VALUE);
  Loading("ccopy_reg\n_reconstructor\np1\n(c__main__\nScott\np2\n"
	  "c__builtin__\nobject\np3\nNtRp4\n(dp5\nS'data'\np6\nI100\nsb.", 
	  OpalValue(ooo));
	  //Eval("(('copy_reg\n_reconstructor\n', ('__main__\nScott\n',"
	  //    " '__builtin__\nobject\n', None)), {'data': 100})"));


  cout << "** Complex tables with P0" << endl;
  char inp[] = "(dp1\nS'a'\ncNumeric\narray_constructor\np2\n((tS'F'\nS'\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00'\nI1\ntRp3\nsS'b'\ng2\n((I3\ntS'F'\nS'\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00'\nI1\ntRp4\nsS'proxy2'\np5\n(dp6\nS'shared'\np7\nI1\nssS'proxy1'\np8\ng6\nsS'nest'\np9\n(dp10\nS'cc'\np11\nI17\nssS'hello'\np12\n(lp13\nF3.2999999999999998\naF4.4000000000000004\naF5.5\nas."; 
  OpalValue p0tab = Eval("{proxy1={shared=X:1}, proxy2={shared=X:1}, a=CF:<0>, b= CF:<0,0,0>, nest= {cc=X:17}, hello={ 3.3, 4.4, 5.5}}");
  Loading(inp, p0tab, sizeof(inp));
  //Val v = Loading(inp, Tab("{'proxy1': {'shared': 1}, 'proxy2': {'shared': 1}, 'a': array([(0+0j)], 'F'), 'b': array([(0+0j),(0+0j),(0+0j)], 'F'), 'nest': {'cc': 17}, 'hello': [3.3, 4.4, 5.5]}"), sizeof(inp));
//if (! is(v["proxy1"], v["proxy2"])) cout << "Oops! Should have been proxies" << endl;

  cout << "** Complex tables with P2" << endl;
  char input[] = "\x80\002}q\001(U\001acNumeric\narray_constructor\nq\002()U\001FU\x08\000\000\000\000\000\000\000\000K\001tRq\003U\001bh\002(K\003\x85U\001FU\x18\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000K\001tRq\004U\006proxy2q\005}q\006U\006sharedq\007K\001sU\006proxy1q\x08h\006U\004nestq\t}q\nU\002ccq\x0bK\x11sU\005helloq\x0c]q\r(G@\nffffffG@\x11\x99\x99\x99\x99\x99\x9aG@\x16\000\000\000\000\000\000eu.";
  OpalValue p2tab = Eval("{proxy1={shared=UB:1}, proxy2={shared=UB:1}, a=CF:<0>, b=CF:<0,0,0>, nest={cc=UB:17}, hello={3.3, 4.4, 5.5}}");
  Loading(input, p2tab, sizeof(input));
  //v = Loading(input, Tab("{'proxy1': {'shared': 1}, 'proxy2': {'shared': 1}, 'a': array([(0+0j)], 'F'), 'b': array([(0+0j),(0+0j),(0+0j)], 'F'), 'nest': {'cc': 17}, 'hello': [3.3, 4.4, 5.5]}"), sizeof(input));
  //if (! is(v["proxy1"], v["proxy2"])) cout << "Oops! Should have been proxies" << endl;



  cout << "** array module" << endl;

  cout << "..This is how Python 2.7 does it: P0" << endl;
  //Array<int_8> qqa; qqa.append(100); qqa.append(200); qqa.append(300);
  OpalValue qqa=Eval("X:<100,200,300>");
  Loading("carray\narray\np1\n(S'l'\n(lp2\nI100\naI200\naI300\natRp3\n.",
	  qqa);


  cout << "..This is how Python 2.6 does it: P0" << endl;
  Loading("\x80\002carray\narray\nq\001U\001lU\030d\x00\x00\x00\x00\x00\x00\x00\xc8\x00\x00\x00\x00\x00\x00\x00,\x01\x00\x00\x00\x00\x00\x00\x86Rq\002.",
	  qqa, 51);

  cout << "..This is how Python 2.7 does it: P2" << endl;
  //Array<int_1> aqa; aqa.append('A'); aqa.append('7');
  OpalValue aqa = Eval("B:<65, 55>");
  //Loading("\x80\002carray\narray\nq\001U\001c]q\002(U\001AU\0017e\x86Rq\003.",
  Loading("\x80\002carray\narray\nq\x01U\001b]q\x02(KAK7e\x86Rq\x03.",
	  aqa);

  //Array<int_8> ap; ap.append(100); ap.append(200); ap.append(300);
  OpalValue ap = Eval("X:<100,200,300>");
  Loading("\x80\002carray\narray\nq\001U\001l]q\002(KdK\xc8M,\001e\x86Rq\003.",
	  ap);

  cout << "..This is how Python 2.6 does it: P2" << endl;
  Loading("\x80\002carray\narray\nq\001U\001cU\002A7\x86Rq\002.",
	  aqa);
  Loading("\x80\002carray\narray\nq\001U\001lU\030d\x00\x00\x00\x00"
	  "\x00\x00\x00\xc8\x00\x00\x00\x00\x00\x00\x00,\x01\x00\x00"
	  "\x00\x00\x00\x00\x86Rq\002.",
	  ap, 51);

  cout << "**Numeric module P2 FROM A 32_BIT MACHINE" << endl;
  //Array<int_4> rt = Eval("array([1,2,3], 'i')");
  OpalValue rt = Eval("L:<1,2,3>");
  cout << "...The load below represents loading a Numeric array from a \n"
    "   32-bit machine: it had the Numeric typecode 'l', which means 'long'\n"
    "   in Numeric land, but sizeof(long)==4 on the original 32-bit machine\n"
    "   and 'more frequently', the sizeof(long)==8).\n"
    "   We can detect this on the load and correct the problem.\n" 
    "   When we dump, however, we currently (at least from C++)\n"
    "   ALWAYS dump a Numeric 'l' as a int_8 array." << endl;
  Loading("\x80\002cNumeric\narray_constructor\nq\001(K\003\x85U\001lU\x0c\x01\x00\x00\x00\x02\x00\x00\x00\x03\x00\x00\x00\x88tRq\x02.", 
	  rt, 58);


  
  cout << "**NumPy, empty array test pickle 2" << endl;
  // Array<real_8> r7 = Eval("array([], 'd')");
  OpalValue r7=Eval("D:<>");
  char somethingelse[] = "\x80\002cnumpy.core.multiarray\n_reconstruct\nq\001cnumpy\nndarray\nq\002K\000\x85U\001b\x87Rq\x03(K\x01K\000\000cnumpy\ndtype\nq\x04U\002f8K\000K\x01\x87Rq\x05(K\x03U\x01<NNNJ\xff\xff\xff\xffJ\xff\xff\xff\xffK\000tb\x89U\000tb.";
  somethingelse[72] = '\x85';  // \x85c is considered a longer escape sequence
  //for (int ii=0; ii<129; ii++) {
  //  cerr << ii << " " << somethingelse[ii] << " " << int(somethingelse[ii]) << endl;
  //}
  Loading(somethingelse, r7, 129);
  cout << "**NumPy, empty array test pickle 0" << endl;
  char p1some[] = "cnumpy.core.multiarray\n_reconstruct\np1\n(cnumpy\nndarray\np2\n(I0\ntS'b'\ntRp3\n(I1\n(I0\ntcnumpy\ndtype\np4\n(S'f8'\nI0\nI1\ntRp5\n(I3\nS'<'\nNNNI-1\nI-1\nI0\ntbI00\nS''\ntb.";
  Loading(p1some, r7);


  cout << "**NumPy floats test pickle 2" << endl;
  //Array<real_4> r8 = Eval("array([100,200,300,400], 'f')");
  OpalValue r8 = Eval("F:<100,200,300,400>");
  char so[] = "\x80\002cnumpy.core.multiarray\n_reconstruct\nq\001cnumpy\nndarray\nq\x02K\x00\x85U\001b\x87Rq\x03(K\x01K\x04\x85\x63numpy\ndtype\nq\x04U\002f4K\x00K\x01\x87Rq\x05(K\x03U\x01<NNNJ\xff\xff\xff\xffJ\xff\xff\xff\xffK\x00tb\x89U\x10\x00\x00\xc8\x42\x00\x00HC\x00\x00\x96\x43\x00\x00\xc8\x43tb.";
  //for (int ii=0; ii<145; ii++) {
  //  cerr << ii << " " << so[ii] << " " << int(so[ii]) << endl;
  //}
  Loading(so, r8, 145);



  cout << "**NumPy floats test pickle 0" << endl;
  char sop0[] = "cnumpy.core.multiarray\n_reconstruct\np1\n(cnumpy\nndarray\np2\n(I0\ntS'b'\ntRp3\n(I1\n(I4\ntcnumpy\ndtype\np4\n(S'f4'\nI0\nI1\ntRp5\n(I3\nS'<'\nNNNI-1\nI-1\nI0\ntbI00\nS'\\x00\\x00\\xc8B\\x00\\x00HC\\x00\\x00\\x96C\\x00\\x00\\xc8C'\ntb.";
  Loading(sop0, r8);


  cout << "**NumPy complex_16 test pickle 2" << endl;
  //Array<complex_16> r9 = Eval("array([(1+0j),(2+0j),(3+0j),(4+0j)], 'D')");
  OpalValue r9 = Eval("CD:<(1,0),(2,0),(3,0),(4,0)>");
  char so1[] = "\x80\002cnumpy.core.multiarray\n_reconstruct\nq\001cnumpy\nndarray\nq\002K\x00\x85U\001b\x87Rq\x03(K\x01K\x04\x85\x63numpy\ndtype\nq\x04U\x03\x63\x31\x36K\x00K\x01\x87Rq\x05(K\x03U\x01<NNNJ\xff\xff\xff\xffJ\xff\xff\xff\xffK\x00tb\x89U@\x00\x00\x00\x00\x00\x00\xf0?\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00@\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x08@\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x10@\x00\x00\x00\x00\x00\x00\x00\x00tb.";
  //for (int ii=0; ii<194; ii++) {
  //  cerr << ii << " " << so1[ii] << " " << int(so1[ii]) << endl;
  // }
  Loading(so1, r9, 194);

  cout << "**NumPy complex_16 test pickle 0" << endl;
  char so1p0[] = "cnumpy.core.multiarray\n_reconstruct\np1\n(cnumpy\nndarray\np2\n(I0\ntS'b'\ntRp3\n(I1\n(I4\ntcnumpy\ndtype\np4\n(S'c16'\nI0\nI1\ntRp5\n(I3\nS'<'\nNNNI-1\nI-1\nI0\ntbI00\nS'\\x00\\x00\\x00\\x00\\x00\\x00\\xf0?\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00@\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x08@\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x10@\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00'\ntb.";
  Loading(so1p0, r9);

  cout << "**NumPy complex_8 test pickle 2" << endl;
  //Array<complex_8> r10 = Eval("array([(1+0j),(2+0j),(3+0j),(4+0j)], 'F')");
  OpalValue r10 = Eval("CF:<(1,0),(2,0),(3,0),(4,0)>");
  char so2[] = "\x80\002cnumpy.core.multiarray\n_reconstruct\nq\001cnumpy\nndarray\nq\x02K\x00\x85U\001b\x87Rq\x03(K\x01K\x04\x85\x63numpy\ndtype\nq\x04U\002c8K\x00K\x01\x87Rq\x05(K\x03U\x01<NNNJ\xff\xff\xff\xffJ\xff\xff\xff\xffK\x00tb\x89U \x00\x00\x80?\x00\x00\x00\x00\x00\x00\x00@\x00\x00\x00\x00\x00\x00@@\x00\x00\x00\x00\x00\x00\x80@\x00\x00\x00\x00tb.";
  //for (int ii=0; ii<161; ii++) {
  //  cerr << ii << " " << so2[ii] << " " << int(so2[ii]) << endl;
  //}
  Loading(so2, r10, 161);

  cout << "**NumPy complex_8 test pickle 0" << endl;
  char qw[] = "cnumpy.core.multiarray\n_reconstruct\np1\n(cnumpy\nndarray\np2\n(I0\ntS'b'\ntRp3\n(I1\n(I4\ntcnumpy\ndtype\np4\n(S'c8'\nI0\nI1\ntRp5\n(I3\nS'<'\nNNNI-1\nI-1\nI0\ntbI00\nS'\\x00\\x00\\x80?\\x00\\x00\\x00\\x00\\x00\\x00\\x00@\\x00\\x00\\x00\\x00\\x00\\x00@@\\x00\\x00\\x00\\x00\\x00\\x00\\x80@\\x00\\x00\\x00\\x00'\ntb.";
  Loading(qw, r10);


  cout << "**NumPy int_u1 test pickle 2" << endl;
  //Array<int_u1> r11 = Eval("array([1,2,3,4], 'b')");
  OpalValue r11 = Eval("UB:<1,2,3,4>");
  char so3[] = "\x80\002cnumpy.core.multiarray\n_reconstruct\nq\001cnumpy\nndarray\nq\002K\x00\x85U\001b\x87Rq\x03(K\x01K\x04\x85\x63numpy\ndtype\nq\x04U\x02u1K\x00K\x01\x87Rq\x05(K\x03U\x01|NNNJ\xff\xff\xff\xffJ\xff\xff\xff\xffK\x00tb\x89U\x04\x01\x02\x03\x04tb.";
  //for (int ii=0; ii<133; ii++) {
  //  cerr << ii << " " << so3[ii] << " " << int(so3[ii]) << endl;
  //}
  Loading(so3, r11, 133);

  cout << "**NumPy int_u1 test pickle 0" << endl;
  Loading("cnumpy.core.multiarray\n_reconstruct\np1\n(cnumpy\nndarray\np2\n(I0\ntS'b'\ntRp3\n(I1\n(I4\ntcnumpy\ndtype\np4\n(S'u1'\nI0\nI1\ntRp5\n(I3\nS'|'\nNNNI-1\nI-1\nI0\ntbI00\nS'\\x01\\x02\\x03\\x04'\ntb.", r11);

  cout << "**NumPy int_1 test pickle 2" << endl;
  //Array<int_1> r12 = Eval("array([1,2,3,4], '1')");
  OpalValue r12 = Eval("B:<1,2,3,4>");
  char so4[] = "\x80\002cnumpy.core.multiarray\n_reconstruct\nq\001cnumpy\nndarray\nq\x02K\x00\x85U\001b\x87Rq\x03(K\x01K\x04\x85\x63numpy\ndtype\nq\x04U\x02i1K\x00K\x01\x87Rq\x05(K\x03U\x01|NNNJ\xff\xff\xff\xffJ\xff\xff\xff\xffK\x00tb\x89U\x04\x01\x02\x03\x04tb.";
  //for (int ii=0; ii<133; ii++) {
  //  cerr << ii << " " << so4[ii] << " " << int(so4[ii]) << endl;
  //}
  Loading(so4, r12, 133);

  cout << "**NumPy int_1 test pickle 0" << endl;
  Loading("cnumpy.core.multiarray\n_reconstruct\np1\n(cnumpy\nndarray\np2\n(I0\ntS'b'\ntRp3\n(I1\n(I4\ntcnumpy\ndtype\np4\n(S'i1'\nI0\nI1\ntRp5\n(I3\nS'|'\nNNNI-1\nI-1\nI0\ntbI00\nS'\\x01\\x02\\x03\\x04'\ntb.", r12);


  cout << "**NumPy int_u2 test pickle 2" << endl;
  //Array<int_u2> r13 = Eval("array([1,2,3,4], 'w')");
  OpalValue r13 = Eval("UI:<1,2,3,4>");
  char so5[] = "\x80\002cnumpy.core.multiarray\n_reconstruct\nq\001cnumpy\nndarray\nq\x02K\x00\x85U\001b\x87Rq\x03(K\x01K\x04\x85\x63numpy\ndtype\nq\x04U\x02u2K\x00K\x01\x87Rq\x05(K\x03U\x01<NNNJ\xff\xff\xff\xffJ\xff\xff\xff\xffK\x00tb\x89U\x08\x01\x00\x02\x00\x03\x00\x04\x00tb.";
  //for (int ii=0; ii<137; ii++) {
  //  cerr << ii << " " << so5[ii] << " " << int(so5[ii]) << endl;
  //}
  Loading(so5, r13, 137);

  cout << "**NumPy int_u2 test pickle 2" << endl;
  Loading("cnumpy.core.multiarray\n_reconstruct\np1\n(cnumpy\nndarray\np2\n(I0\ntS'b'\ntRp3\n(I1\n(I4\ntcnumpy\ndtype\np4\n(S'u2'\nI0\nI1\ntRp5\n(I3\nS'<'\nNNNI-1\nI-1\nI0\ntbI00\nS'\\x01\\x00\\x02\\x00\\x03\\x00\\x04\\x00'\ntb.", r13);

  cout << "**NumPy int_2 test pickle 2" << endl;
  //Array<int_2> r14 = Eval("array([1,2,3,4], 's')");
  OpalValue r14 = Eval("I:<1,2,3,4>");
  char so6[] = "\x80\002cnumpy.core.multiarray\n_reconstruct\nq\001cnumpy\nndarray\nq\x02K\x00\x85U\001b\x87Rq\x03(K\x01K\x04\x85\x63numpy\ndtype\nq\x04U\x02i2K\x00K\x01\x87Rq\x05(K\x03U\x01<NNNJ\xff\xff\xff\xffJ\xff\xff\xff\xffK\x00tb\x89U\x08\x01\x00\x02\x00\x03\x00\x04\x00tb.";
  //for (int ii=0; ii<137; ii++) {
  //  cerr << ii << " " << so6[ii] << " " << int(so6[ii]) << endl;
  //}
  Loading(so6, r14, 137);

  cout << "**NumPy int_2 test pickle 0" << endl;
  Loading("cnumpy.core.multiarray\n_reconstruct\np1\n(cnumpy\nndarray\np2\n(I0\ntS'b'\ntRp3\n(I1\n(I4\ntcnumpy\ndtype\np4\n(S'i2'\nI0\nI1\ntRp5\n(I3\nS'<'\nNNNI-1\nI-1\nI0\ntbI00\nS'\\x01\\x00\\x02\\x00\\x03\\x00\\x04\\x00'\ntb.", r14);

  cout << "**NumPy, int_u4 pickle 2" << endl;
  //Array<int_u4> r6 = Eval("array([15,16,17], 'u')");
  OpalValue r6 = Eval("UL:<15,16,17>");
  //char sssomething[] = "\x80\002cnumpy.core.multiarray\n_reconstruct\nq\001cnumpy\nndarray\nq\002K\x00\x85U\001b\x87Rq\x03(K\x01K\x03\025cnumpy\ndtype\nq\x04U\x02u4K\x00K\x01\x87Rq\x05(K\x03U\x01<NNNJ\xff\xff\xff\xffJ\xff\xff\xff\xffK\x00tb\x89U\x0c\x0f\x00\x00\x00\x10\x00\x00\x00\x11\x00\x00\x00tb.";
  char something[] = "\x80\002cnumpy.core.multiarray\n_reconstruct\nq\001cnumpy\nndarray\nq\002K\x00\x85U\001b\x87Rq\x03(K\x01K\x03\x85\x63numpy\ndtype\nq\x04U\x02u4K\x00K\x01\x87Rq\x05(K\x03U\x01<NNNJ\xff\xff\xff\xffJ\xff\xff\xff\xffK\x00tb\x89U\x0c\x0f\x00\x00\x00\x10\x00\x00\x00\x11\x00\x00\x00tb.";
  //for (int ii=0; ii<141; ii++) {
  //  cerr << ii << " " << something[ii] << " " << int(something[ii]) << endl;
  //}
  Loading(something, r6, 141);

  cout << "**NumPy, int_u4 pickle 0" << endl;
  //Loading("cnumpy.core.multiarray\n_reconstruct\np1\n(cnumpy\nndarray\np2\n(I0\ntS'b'\ntRp3\n(I1\n(I4\ntcnumpy\ndtype\np4\n(S'i2'\nI0\nI1\ntRp5\n(I3\nS'<'\nNNNI-1\nI-1\nI0\ntbI00\nS'\\x01\\x00\\x02\\x00\\x03\\x00\\x04\\x00'\ntb.", r6);
  Loading("cnumpy.core.multiarray\n_reconstruct\np1\n(cnumpy\nndarray\np2\n(I0\ntS'b'\ntRp3\n(I1\n(I3\ntcnumpy\ndtype\np4\n(S'u4'\nI0\nI1\ntRp5\n(I3\nS'<'\nNNNI-1\nI-1\nI0\ntbI00\nS'\\x0f\\x00\\x00\\x00\\x10\\x00\\x00\\x00\\x11\\x00\\x00\\x00'\ntb.", r6);
  

  cout << "**NumPy, int_4 pickle 2" << endl;
  //Array<int_4> r22 = Eval("array([15,16,17], 'i')");
  OpalValue r22 = Eval("L:<15,16,17>");
  char somethin[] = "\x80\002cnumpy.core.multiarray\n_reconstruct\nq\001cnumpy\nndarray\nq\x02K\x00\x85U\001b\x87Rq\x03(K\x01K\x03\x85\x63numpy\ndtype\nq\x04U\x02i4K\x00K\x01\x87Rq\x05(K\x03U\x01<NNNJ\xff\xff\xff\xffJ\xff\xff\xff\xffK\x00tb\x89U\x0c\x0f\x00\x00\x00\x10\x00\x00\x00\x11\x00\x00\x00tb.";
  //for (int ii=0; ii<141; ii++) {
  //  cerr << ii << " " << somethin[ii] << " " << int(somethin[ii]) << endl;
  //}
  Loading(somethin, r22, 141);

  cout << "**NumPy, int_4 pickle 0" << endl;
  Loading("cnumpy.core.multiarray\n_reconstruct\np1\n(cnumpy\nndarray\np2\n(I0\ntS'b'\ntRp3\n(I1\n(I3\ntcnumpy\ndtype\np4\n(S'i4'\nI0\nI1\ntRp5\n(I3\nS'<'\nNNNI-1\nI-1\nI0\ntbI00\nS'\\x0f\\x00\\x00\\x00\\x10\\x00\\x00\\x00\\x11\\x00\\x00\\x00'\ntb.", r22);


  cout << "**NumPy, int_u8 pickle 2" << endl;
  //Array<int_u8> r23;r23.append(1);r23.append(-1);r23.append(0);
  OpalValue r23 = Eval("UX:<1,-1,0>");
  char gar[] = "\x80\002cnumpy.core.multiarray\n_reconstruct\nq\001cnumpy\nndarray\nq\x02K\x00\x85U\001b\x87Rq\x03(K\x01K\x03\x85\x63numpy\ndtype\nq\x04U\x02u8K\x00K\x01\x87Rq\x05(K\x03U\x01<NNNJ\xff\xff\xff\xffJ\xff\xff\xff\xffK\x00tb\x89U\x18\x01\x00\x00\x00\x00\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff\x00\x00\x00\x00\x00\x00\x00\x00tb.";
  //for (int ii=0; ii<153; ii++) {
  //  cerr << ii << " " << gar[ii] << " " << int(gar[ii]) << endl;
  //}
  Loading(gar, r23, 153);

  cout << "**NumPy, int_u8 pickle 0" << endl;
  Loading("cnumpy.core.multiarray\n_reconstruct\np1\n(cnumpy\nndarray\np2\n(I0\ntS'b'\ntRp3\n(I1\n(I3\ntcnumpy\ndtype\np4\n(S'u8'\nI0\nI1\ntRp5\n(I3\nS'<'\nNNNI-1\nI-1\nI0\ntbI00\nS'\\x01\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\xff\\xff\\xff\\xff\\xff\\xff\\xff\\xff\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00'\ntb.", r23);

  cout << "**NumPy, int_8 pickle 2" << endl;
  //Array<int_8> r24; r24.append(1); r24.append(-1);r24.append(0);
  OpalValue r24=Eval("X:<1,-1,0>");
  char gar1[] = "\x80\002cnumpy.core.multiarray\n_reconstruct\nq\001cnumpy\nndarray\nq\x02K\x00\x85U\001b\x87Rq\x03(K\x01K\x03\x85\x63numpy\ndtype\nq\x04U\x02i8K\x00K\x01\x87Rq\x05(K\x03U\x01<NNNJ\xff\xff\xff\xffJ\xff\xff\xff\xffK\x00tb\x89U\x18\x01\x00\x00\x00\x00\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff\x00\x00\x00\x00\x00\x00\x00\x00tb.";
  //for (int ii=0; ii<153; ii++) {
  //  cerr << ii << " " << gar1[ii] << " " << int(gar1[ii]) << endl;
  //}
  Loading(gar1, r24, 153);

  cout << "**NumPy, int_8 pickle 0" << endl;
  Loading("cnumpy.core.multiarray\n_reconstruct\np1\n(cnumpy\nndarray\np2\n(I0\ntS'b'\ntRp3\n(I1\n(I3\ntcnumpy\ndtype\np4\n(S'i8'\nI0\nI1\ntRp5\n(I3\nS'<'\nNNNI-1\nI-1\nI0\ntbI00\nS'\\x01\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\xff\\xff\\xff\\xff\\xff\\xff\\xff\\xff\\x00\\x00\\x00\\x00\\x00\\x00\\x00\\x00'\ntb.", r24);

  cout << "**NumPy, bool pickle 2" << endl;
  //Array<bool> r25; r25.append(1); r25.append(1); r25.append(0);
  OpalValue r25 = Eval("UB:<1,1,0>");
  char gar2[] = "\x80\002cnumpy.core.multiarray\n_reconstruct\nq\001cnumpy\nndarray\nq\x02K\x00\x85U\001b\x87Rq\x03(K\x01K\x03\x85\x63numpy\ndtype\nq\x04U\002b1K\x00K\x01\x87Rq\x05(K\x03U\x01|NNNJ\xff\xff\xff\xffJ\xff\xff\xff\xffK\x00tb\x89U\x03\x01\x01\x00tb.";
  //for (int ii=0; ii<132; ii++) {
  //  cerr << ii << " " << gar2[ii] << " " << int(gar2[ii]) << endl;
  //}
  Loading(gar2, r25, 132);

  cout << "**NumPy, bool pickle 0" << endl;
  Loading("cnumpy.core.multiarray\n_reconstruct\np1\n(cnumpy\nndarray\np2\n(I0\ntS'b'\ntRp3\n(I1\n(I3\ntcnumpy\ndtype\np4\n(S'b1'\nI0\nI1\ntRp5\n(I3\nS'|'\nNNNI-1\nI-1\nI0\ntbI00\nS'\\x01\\x01\\x00'\ntb.", r25);


  // Test dumping, now that loading works

  Dumping("*** Dump NumPy real_8 ", ar8);
  Dumping("*** Dump NumPy real_4 ", ar4);
  Dumping("*** Dump NumPy complex_8 ", c8);
  Dumping("*** Dump NumPy complex_16 ", c16);
  Dumping("*** Dump NumPy int_1  ", a1);
  Dumping("*** Dump NumPy int_u1 ", au1);
  Dumping("*** Dump NumPy int_2 ",  a2);
  Dumping("*** Dump NumPy int_u2 ", au2);
  Dumping("*** Dump NumPy int_4 ",  a4);
  Dumping("*** Dump NumPy int_u4 ", au4);
  Dumping("*** Dump NumPy int_8 ",  a8);
  Dumping("*** Dump NumPy int_u8 ", au8);
  Dumping("*** Dump NumPy bool ", a1);
  //OpalValue empty = Opalize(Vector());
  //Dumping("*** Dump NumPy empty ", empty);


  cout << "**NumPy, sharing pickle 0" << endl;
  OpalValue shar = Eval("{ a=L:<1>, b=L:<1> }");
  OpalValue resultme = Loading("(dp1\nS'a'\ncnumpy.core.multiarray\n_reconstruct\np2\n(cnumpy\nndarray\np3\n(I0\ntS'b'\ntRp4\n(I1\n(I1\ntcnumpy\ndtype\np5\n(S'i4'\nI0\nI1\ntRp6\n(I3\nS'<'\nNNNI-1\nI-1\nI0\ntbI00\nS'\\x01\\x00\\x00\\x00'\ntbsS'b'\ng4\ns.", shar);
  //cout << is(resultme["a"], resultme["b"]) << endl;

  cout << "**NumPy, sharing pickle 2" << endl;
  resultme = Loading("\x80\x02}q\x01(U\001acnumpy.core.multiarray\n_reconstruct\nq\002cnumpy\nndarray\nq\x03K\x00\x85U\001b\x87Rq\x04(K\x01K\x01\x85\x63numpy\ndtype\nq\x05U\x02i4K\x00K\x01\x87Rq\x06(K\x03U\x01<NNNJ\xff\xff\xff\xffJ\xff\xff\xff\xffK\x00tb\x89U\x04\x01\x00\x00\x00tbU\001bh\x04u.", shar, 146);
  //cout << is(resultme["a"], resultme["b"]) << endl;

  cout << "**NumPy, multiple sharing pickle 0" << endl;
  OpalTable shar1 = shar; 
  //Tab("{'a':None, 'b':None}");
  //Array<int_4>* aap1 = new Array<int_4>(1);
  //aap1->append(1);
  //Proxy p11 = aap1;
  //shar1["a"] = p11;
  //Array<int_4>* aap2 = new Array<int_4>(1);
  //aap2->append(1);
  //Proxy p22 = aap2;
  //shar1["b"] = p22;
  resultme = Loading("(dp1\nS'a'\ncnumpy.core.multiarray\n_reconstruct\np2\n(cnumpy\nndarray\np3\n(I0\ntS'b'\ntRp4\n(I1\n(I1\ntcnumpy\ndtype\np5\n(S'i4'\nI0\nI1\ntRp6\n(I3\nS'<'\nNNNI-1\nI-1\nI0\ntbI00\nS'\\x01\\x00\\x00\\x00'\ntbsS'b'\ng2\n(g3\n(I0\ntS'b'\ntRp7\n(I1\n(I1\ntg6\nI00\nS'\\x01\\x00\\x00\\x00'\ntbs.",
		     shar1);
  //cout << is(resultme["a"], resultme["b"]) << endl;

  cout << "**NumPy, multiple sharing pickle 2" << endl;
  resultme = Loading("\x80\x02}q\x01(U\001acnumpy.core.multiarray\n_reconstruct\nq\002cnumpy\nndarray\nq\x03K\x00\x85U\001b\x87Rq\x04(K\x01K\x01\x85\x63numpy\ndtype\nq\x05U\x02i4K\x00K\x01\x87Rq\x06(K\x03U\x01<NNNJ\xff\xff\xff\xffJ\xff\xff\xff\xffK\x00tb\x89U\x04\x01\x00\x00\x00tbU\001bh\x02h\x03K\x00\x85U\001b\x87Rq\x07(K\x01K\x01\x85h\x06\x89U\x04\x01\x00\x00\x00tbu.", 
		     shar1, 175);
  //cout << is(resultme["a"], resultme["b"]) << endl;

}
