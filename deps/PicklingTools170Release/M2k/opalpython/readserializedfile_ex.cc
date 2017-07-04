
#include "m2chooseser.h"
#include "m2globalevaluator.h"

void usage (char **argv)
{
  fprintf(stderr, 
	  "usage: %s filename read|write serialization\n"
	  "   where serialization is one of: \n"
	  "     p0 p2 oc m2k pythonpretty pythontext opalpretty opaltext none\n", argv[0]);
  exit(1);
}

int main (int argc, char **argv)
{
  if (argc !=4 ) {
    usage(argv);
  }
  
  const string filename = argv[1];
  const string mode = argv[2];
  string serialization = argv[3];
  OpalValue result;
  Serialization_e ser;

  if (serialization=="p0") {
    ser = SERIALIZE_P0; 
  } else if (serialization=="p2") {
    ser =  SERIALIZE_P2;
  } else if (serialization=="m2k") {
    ser = SERIALIZE_M2K;
  } else if (serialization=="oc") {
    ser = SERIALIZE_OC;
  } else if (serialization=="pythontext") {
    ser = SERIALIZE_PYTHONTEXT;
  } else if (serialization=="pythonpretty") {
    ser = SERIALIZE_PYTHONPRETTY;
  } else if (serialization=="opaltext") {
    ser = SERIALIZE_OPALTEXT;
  } else if (serialization=="opalpretty") {
    ser = SERIALIZE_OPALPRETTY;
  } else if (serialization=="none") {
    ser = SERIALIZE_NONE;
  } else {
    usage(argv);
  }

  // READ
  if (mode=="read") { 
    try {
      LoadOpalValueFromFile (filename, result, ser, AS_LIST,
			     false, MachineRep_EEEI);
      result.prettyPrint(cout);
    } catch (MidasException& me) {
      cerr << "*********Caught Exception***********: " << me.problem() << endl;
      cerr << "...rethrowing... " << endl;
      throw;
    }
  }
  
  // WRITE 
  else if (mode=="write") {
    OpalTable env;
    result = GlobalEvaluator::instance().evaluate(env, "{a=1, b=2, c={d=B:1, e=UB:1, f=I:1, g=UI:2}, }");
    result.prettyPrint(cout); 
    DumpOpalValueToFile(result, filename, ser, 
			AS_LIST, false, MachineRep_EEEI);
    
  } else {
    usage(argv);
  }
}
