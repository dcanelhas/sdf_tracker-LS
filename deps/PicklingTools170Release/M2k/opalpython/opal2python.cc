
#include "m2opalvalue.h"
#include "m2opalpython.h"
#include "m2menvironment.h"

static OpalValue readTable (const string& arg)
{
  const string file_name = arg;
  const string resolved_fname = M2Env().dataPath().getDirectory(file_name);

  OpalValue ov;
  try {
    ov.readFromFile(resolved_fname, OpalValueA_TABLE);
  } catch (const NestedEvalEx& ne) {
    string message =
      "  while evaluating from line " + IntToString(ne.lineNumber());
    if (ne.filename().length() > 0)
      message += ", in file " + ne.filename();
    GlobalLog.message(message);
    throw;
  } catch (const EvalException& e) {
    GlobalLog.error(EvalExceptionMessage(e));
    throw NestedEvalEx(e.lineNumber(), e.charNumber(), e.errorLine(),
                       e.context(), e.filename());
  } 
  return ov.table();
}                                       // readTable


#include "m2opalswap.h"
int main (int argc, char* argv[])
{
  cerr << sizeof(Number) << endl;
  cerr << sizeof(OpalTable) << endl;
  OpalTable ot1;
  ot1.put("hello", OpalValue("there"));
  OpalValue o1 = ot1;
  OpalTable ot2;
  ot2.put("Other thangs", OpalValue("here"));
  OpalValue o2 = ot2;
  cout << o1 << " before " << o2 << endl;
  Swap(o1, o2);
  cout << o1 << " after " << o2 << endl;

  OpalValue oc1 = Opalize(true);
  OpalValue oc2 = Opalize(false);
  cout << oc1 << " before " << oc2 << endl;
  Swap(oc1, oc2 );
  cout << oc1 << " after " << oc2 << endl;


  cout << oc1 << " before " << o2 << endl;
  Swap(oc1, o2);
  cout << oc1 << " after " << o2 << endl;

  cout << o1 << " before " << oc2 << endl;
  Swap(o1, oc2);
  cout << o1 << " after " << oc2 << endl;

  exit(1);
 
  if (argc!=3) {
    cerr << "Usage:" << argv[0] << " file_with_opal_table_strings python_pickle_file" << endl;
  }
 
  OpalValue ov = readTable(argv[1]);
  Array<char> a;
  {
    PythonPickler<OpalValue> pp(argv[2], false);
    pp.dump(ov);
  }
}
