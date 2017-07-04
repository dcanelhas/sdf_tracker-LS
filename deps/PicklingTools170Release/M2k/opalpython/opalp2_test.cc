
#include "m2opalprotocol2.h"
#include "m2image.h"
#include "m2opaltable.h"

void dumpMemory (char* start, char* end)
{
  int len = end-start;
  string im = Image(string(start, len));
  cout << im << endl;
  //for (int ii=0; ii<len; ii++) {
  //  cout << int(start[ii]) << " " << endl;
  //}
}

void typePrint (ostream& os, const string& label, const OpalValue& ov)
{
  os << label << ov.type();
  OpalTable ot; ot.append(ov);
  os << ot << endl;
}

void comparing (char* dump_end, char* load_end, 
		const OpalValue& original, const OpalValue& result)
{
  typePrint(cout, "Original:", original);
  typePrint(cout, "Result:", result);
  if (dump_end != load_end) { cerr << "Didn't get back the same amount of memory" << endl; exit(1); }
  cout << "Dumped " << original << " and got back:" << result << " ==?" << (bool(result==original)) << endl;
}


int main ()
{
  char *memory;
  OpalValue result;

  OpalValue ov = Opalize(1);
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  char* mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);


  result = OpalValue(); // clear
  
  char* mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);


  ov = Opalize(255);
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  result = OpalValue(); // clear
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  ov = Opalize(256);
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  result = OpalValue(); // clear
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  ov = Opalize(127);
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  result = OpalValue(); // clear
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  ov = Opalize(128);
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  result = OpalValue(); // clear
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  ov = Opalize(256);
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  result = OpalValue(); // clear
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  ov = Opalize(-255);
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  result = OpalValue(); // clear
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  ov = Opalize(-127);
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  result = OpalValue(); // clear
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  ov = Opalize(-128);
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  result = OpalValue(); // clear
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  int_8 largesti8 = 9223372036854775807LL;
  ov = Opalize(largesti8); // Largest int_8
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  result = OpalValue(); // clear
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  ov = Opalize(-largesti8); // smallest int_8 + 1
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  result = OpalValue(); // clear
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  ov = Opalize(-largesti8+1); // smallest int_8
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  result = OpalValue(); // clear
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  int_u8 largest_iu8 = 18446744073709551615ULL;
  ov = Opalize(largest_iu8); // smallest int_8
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  result = OpalValue(); // clear
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  ov = Opalize(-128);
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  result = OpalValue(); // clear
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  ov = Opalize(int_u8(0x00000000000000000001));
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  result = OpalValue(); // clear
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  ov = Opalize(int_u8(0xFFFFFFFFFFFFFFFFLL));
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  result = OpalValue(); // clear
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  ov = Opalize(int_8(0xFFFFFFFFFFFFFFFFLL));
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  result = OpalValue(); // clear
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  ov = Opalize(int_u4(32768));
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  result = OpalValue(); // clear
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  ov = Opalize(real_8(3.141592));
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  result = OpalValue(); // clear
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  ov = Opalize(string("hello there"));
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  result = OpalValue(); // clear
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  ov = Opalize(true);
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  result = OpalValue(); // clear
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  ov = Opalize(false);
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  result = OpalValue(); // clear
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  ov = OpalValue("None", OpalValueA_TEXT);;
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  result = OpalValue(); // clear
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  OpalTable ot;
  ot.put(1, Opalize(Stringize("hello")));
  ot.put(2, Opalize(Stringize("there")));
  //cout << "OpalTable looks like:" << ot << endl;
  ov = ot;
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  result = OpalValue(); // clear
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  ot = OpalTable();
  ot.append(Opalize(Stringize("hello")));
  ot.append(Opalize(Stringize("there")));
  //cout << "OpalTable looks like:" << ot << endl;
  ov = ot;
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ot, memory);
  dumpMemory(memory, mem);
  result = OpalValue(); // clear
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  complex_16 c16(200, 300);
  ov = OpalValue(c16);;
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  result = OpalValue(); // clear
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  ot = OpalTable();
  ot.append(Opalize(c16));
  ot.append(Opalize(c16));
  //cout << "OpalTable looks like:" << ot << endl;
  OpalTable above;
  above.put("100", ot);
  ov = above;
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  result = OpalValue(); // clear
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  Vector v;

  v = Vector(Number(real_8(15)), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  v = Vector(Number(int_u1(15)), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  v = Vector(Number(c16), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  complex_8 c8(666,777);
  v = Vector(Number(c8), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  v = Vector(M2Time(100), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov)];
  mem = P2TopLevelDumpOpal(ov, memory);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  // As python array

  v = Vector(Number(real_8(15)), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov, AS_PYTHON_ARRAY)];
  mem = P2TopLevelDumpOpal(ov, memory, AS_PYTHON_ARRAY);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  v = Vector(Number(int_u1(15)), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov, AS_PYTHON_ARRAY)];
  mem = P2TopLevelDumpOpal(ov, memory, AS_PYTHON_ARRAY);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  v = Vector(Number(c16), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov, AS_PYTHON_ARRAY)];
  mem = P2TopLevelDumpOpal(ov, memory, AS_PYTHON_ARRAY);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  v = Vector(Number(c8), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov, AS_PYTHON_ARRAY)];
  mem = P2TopLevelDumpOpal(ov, memory, AS_PYTHON_ARRAY);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  v = Vector(M2Time(100), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov, AS_PYTHON_ARRAY)];
  mem = P2TopLevelDumpOpal(ov, memory, AS_PYTHON_ARRAY);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  // As Numeric Arrays

  v = Vector(Number(real_8(15)), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov, AS_NUMERIC)];
  mem = P2TopLevelDumpOpal(ov, memory, AS_NUMERIC);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  v = Vector(Number(int_u1(15)), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov, AS_NUMERIC)];
  mem = P2TopLevelDumpOpal(ov, memory, AS_NUMERIC);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  v = Vector(Number(c16), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov, AS_NUMERIC)];
  mem = P2TopLevelDumpOpal(ov, memory, AS_NUMERIC);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  v = Vector(Number(c8), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov, AS_NUMERIC)];
  mem = P2TopLevelDumpOpal(ov, memory, AS_NUMERIC);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  v = Vector(M2Time(100), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov, AS_NUMERIC)];
  mem = P2TopLevelDumpOpal(ov, memory, AS_NUMERIC);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;


  // Try some things that use Python 2.2
  OpalTable ooo;
  ooo.append(OpalValue(Number(1)));
  ooo.append(OpalValue(Number(-1)));
  ooo.append(OpalValue(Number(127)));
  ooo.append(OpalValue(Number(128)));
  ooo.append(OpalValue(Number(-128)));
  ooo.append(OpalValue(Number(-129)));
  ooo.append(OpalValue(Number(255)));
  ooo.append(OpalValue(Number(256)));
  ooo.append(OpalValue(Number(-255)));
  ooo.append(OpalValue(Number(2147483647)));
  ooo.append(OpalValue(Number(int_u8(2147483648UL))));
  ooo.append(OpalValue(Number(int_8(-2147483648LL))));
  ooo.append(OpalValue(Number(int_8(-2147483649LL))));
  ooo.append(OpalValue(Number(int_8(9223372036854775807ULL))));
  ooo.append(OpalValue(Number(int_8(-9223372036854775807LL))));
  ooo.append(OpalValue(Number(int_8(-9223372036854775808ULL))));
  ooo.append(OpalValue(Number(int_u8(9223372036854775808ULL))));
  ooo.append(OpalValue(Number(int_u8(18446744073709551615ULL))));
  //ooo.append(OpalValue(Number(int_u8(18446744073709551616ULL))));
  ov = Opalize(ooo);
  memory = new char[P2TopLevelBytesToDumpOpal(ov, AS_LIST, AS_PYTHON_2_2)];
  mem = P2TopLevelDumpOpal(ov, memory, AS_LIST, AS_PYTHON_2_2);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  // Python 2.2 arrays as lists
  v = Vector(Number(real_8(15)), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov, AS_LIST, AS_PYTHON_2_2)];
  mem = P2TopLevelDumpOpal(ov, memory, AS_LIST, AS_PYTHON_2_2);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  v = Vector(Number(int_u1(15)), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov, AS_LIST, AS_PYTHON_2_2)];
  mem = P2TopLevelDumpOpal(ov, memory, AS_LIST, AS_PYTHON_2_2);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  v = Vector(Number(c16), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov, AS_LIST, AS_PYTHON_2_2)];
  mem = P2TopLevelDumpOpal(ov, memory, AS_LIST, AS_PYTHON_2_2);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  c8 = complex_8(666,777);
  v = Vector(Number(c8), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov, AS_LIST, AS_PYTHON_2_2)];
  mem = P2TopLevelDumpOpal(ov, memory, AS_LIST, AS_PYTHON_2_2);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  v = Vector(M2Time(100), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov, AS_LIST, AS_PYTHON_2_2)];
  mem = P2TopLevelDumpOpal(ov, memory, AS_LIST, AS_PYTHON_2_2);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  // As python array

  v = Vector(Number(real_8(15)), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov, AS_PYTHON_ARRAY, AS_PYTHON_2_2)];
  mem = P2TopLevelDumpOpal(ov, memory, AS_PYTHON_ARRAY, AS_PYTHON_2_2);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  v = Vector(Number(int_u1(15)), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov, AS_PYTHON_ARRAY, AS_PYTHON_2_2)];
  mem = P2TopLevelDumpOpal(ov, memory, AS_PYTHON_ARRAY, AS_PYTHON_2_2);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  v = Vector(Number(c16), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov, AS_PYTHON_ARRAY, AS_PYTHON_2_2)];
  mem = P2TopLevelDumpOpal(ov, memory, AS_PYTHON_ARRAY, AS_PYTHON_2_2);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  v = Vector(Number(c8), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov, AS_PYTHON_ARRAY, AS_PYTHON_2_2)];
  mem = P2TopLevelDumpOpal(ov, memory, AS_PYTHON_ARRAY, AS_PYTHON_2_2);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  v = Vector(M2Time(100), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov, AS_PYTHON_ARRAY, AS_PYTHON_2_2)];
  mem = P2TopLevelDumpOpal(ov, memory, AS_PYTHON_ARRAY, AS_PYTHON_2_2);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  // As Numeric Arrays

  v = Vector(Number(real_8(15)), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov, AS_NUMERIC, AS_PYTHON_2_2)];
  mem = P2TopLevelDumpOpal(ov, memory, AS_NUMERIC, AS_PYTHON_2_2);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  v = Vector(Number(int_u1(15)), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov, AS_NUMERIC, AS_PYTHON_2_2)];
  mem = P2TopLevelDumpOpal(ov, memory, AS_NUMERIC, AS_PYTHON_2_2);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  v = Vector(Number(c16), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov, AS_NUMERIC, AS_PYTHON_2_2)];
  mem = P2TopLevelDumpOpal(ov, memory, AS_NUMERIC, AS_PYTHON_2_2);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  v = Vector(Number(c8), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov, AS_NUMERIC, AS_PYTHON_2_2)];
  mem = P2TopLevelDumpOpal(ov, memory, AS_NUMERIC, AS_PYTHON_2_2);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;

  v = Vector(M2Time(100), 3);
  ov = Opalize(v);
  memory = new char[P2TopLevelBytesToDumpOpal(ov, AS_NUMERIC, AS_PYTHON_2_2)];
  mem = P2TopLevelDumpOpal(ov, memory, AS_NUMERIC, AS_PYTHON_2_2);
  dumpMemory(memory, mem);
  mem_back = P2TopLevelLoadOpal(result, memory);
  comparing(mem, mem_back, ov, result);
  delete [] memory;
}
