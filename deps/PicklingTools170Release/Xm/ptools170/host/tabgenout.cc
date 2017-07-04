#include "primitive.h"
#include "ocval.h"
#include "chooseser.h"
#include "t4tab.h"
#include "xmtime.h"

// Example showing how we can write T4000 to a pipe
// so a Python primitive can read them as dictionaries on
// the other side.  (Take a look at "tabinpytester.py"
// in the host area for an example of a Python primitive
// that reads from the other side, and mcr/t4pytest.txt
// as an X-Midas macro that shows how to read)

#define DEF_SER  SERIALIZE_P0

void mainroutine()
{
  // Get the serialization: for python to understand
  // it, it pretty much has to be /SER=2 (SERIALIZE_P2)
  // or /SER=0 (SERIALIZE_P0)
  CPHEADER hout;
  initT4Tab(hout, m_apick(1), HCBF_OUTPUT);
  int_4 ser = m_get_switch_def("SER", DEF_SER);
  Serialization_e serialization = Serialization_e(ser);

  Tab t;
  t["TIME"] = 0;

  m_sync();

  // Deliver timestamps every second as a T4000 record
  while (!Mc->break_) {
    XMTime xmt;
    t["TIME"] = xmt.asString();
    sendT4Val(hout, t, DEFAULT_VRBKEY, serialization);
    m_pause(1.0);
  }
  m_close(hout);
}
