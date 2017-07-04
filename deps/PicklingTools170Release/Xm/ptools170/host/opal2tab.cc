//----------------------------------------------------------------------------
// Sample X-Midas C++ primitive showing how to read opaltables
//----------------------------------------------------------------------------
#include <primitive.h>
#include "opalutils.h"

#if defined(OC_FORCE_NAMESPACE)
using namespace OC;
#endif

void mainroutine()
{
  // input from socket
  string filename = m_apick(1);
  cerr << "Reading:" << filename << endl;

  m_sync();

  
  Tab t;
  try {
    ReadTabFromOpalFile(filename, t);
  } catch (const logic_error& le) {
    cerr << le.what() << endl;  // Should show a good error message
  }
  t.prettyPrint(cout);
  
  // m_close(hout);
}

