//----------------------------------------------------------------------------
// Sample X-Midas C++ primitive showing how to write a MidasListener.
// This is used in conjunction with a MidasYeller.
// You need at least OpenContainers 1.5.1 and set some flags
// correctly (see the cfg directory)
//----------------------------------------------------------------------------
#include <primitive.h>
#include <midaslistener.h>

#if defined(OC_FORCE_NAMESPACE)
using namespace OC;
#endif

#define DEF_SER     SERIALIZE_P0
#define DEF_ARRDISP AS_LIST
#define DEF_CONVERT false

int_4 gl_ser;
int_4 gl_arr;
int_4 gl_convert;

void mainroutine()
{
  // Create the socket server
  string host = m_apick(1);
  int port = m_lpick(2);
  int udp_max_packet_length = 1024;

  gl_ser   = m_get_switch_def("SER", DEF_SER);
  gl_arr   = m_get_switch_def("ARRDISP", DEF_ARRDISP);
  gl_convert = m_get_switch_def("CONVERT", DEF_CONVERT);

  // Change the serialization to SERIALIZE_P2 for faster serialization,
  // Change the array_disposition to either AS_PYTHON_ARRAY or AS_NUMERIC
  //       for faster array/Vector serialization
  Serialization_e serialization = Serialization_e(gl_ser);  
  ArrayDisposition_e array_disposition = ArrayDisposition_e(gl_arr); 

  MidasListener ml(host, port, udp_max_packet_length, 
		   serialization, array_disposition);
  if (gl_convert) {
    cout << "... you have selected forced conversion mode, which means all\n"
         << "    OTab, Tup, BigInt will be converted to Tab, Arr, Str:\n"
         << "    This is for preserving legacy with pre-existing PTOOLS"
         << "    installations (i.e., backwards compatibility)."
         << endl;
    ml.compatibilityMode(gl_convert);
  }


  ml.open();
  cout << "Starting to listen on " << host << " " << port << endl;

  // We know the server has been created
  m_sync();

  // Wake up every so often to see if we are quitting
  while (!Mc->break_) { // DO THIS IN A LOOP SO WE RECOGNIZE Mc-BREAK!!!

    // Try to listen for something on the UDP port.  If nothing there
    // after 1.5 seconds, return None and try again
    Val result = ml.recv(1.5);
    result.prettyPrint(cout);  // print nested things well
    cout << endl;
  }

  ml.close(); // Done by destructor, but for good measure
}

