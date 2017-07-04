//----------------------------------------------------------------------------
// Sample X-Midas C++ primitive showing how to use a MidasTalker.
// You need at least OpenContainers 1.4.1 and set some flags
// correctly (see the m2krec.b file to see how to set flags).
//----------------------------------------------------------------------------
#include <primitive.h>
#include <midastalker.h>

#define DEF_SER     SERIALIZE_P0
#define DEF_SOCK    2
#define DEF_ARRDISP AS_LIST
#define DEF_CONVERT false

int_4 gl_ser;
int_4 gl_sock;
int_4 gl_arr;
int_4 gl_convert;

#if defined(OC_FORCE_NAMESPACE)
using namespace OC;
#endif

// Main loop to send/recv messages to the Midastalker (mt)
void sendloop (MidasTalker& mt, Arr& data)
{
  // Send data to permute
  cout << "Sending request:" << data << endl;
  mt.send(data);

  // See if we can receive, wait 3 seconds
  while (!Mc->break_) {

    Val res = mt.recv(3.0);

    if (res == None) {
      cout << "...retrying to receive after 3 seconds ..." << endl;
      // Maybe try to do some other work
      continue;
    }

    else {
      // Do something with the result
      if (res=="EOF") return;
      cout << res << endl;
    }
  }
}

// Try to open up the MidasTalker, in case not immediately available
void openup (MidasTalker& mt)
{
  while (!Mc->break_) {

    try {
      mt.open();
      cout << "Opened connection to host:"
	   << mt.host() << " port:" << mt.port() << endl;
      break;
    }
    catch (const runtime_error& e) {
      cout << "Problem: " << e.what() << endl;
      cout << "...couldn't open right away, backing off 3 seconds" << endl;
      sleep(3);
    }
  }
}

void mainroutine()
{
  // input from socket
  string host = m_apick(1);
  int port    = m_lpick(2);

  gl_ser   = m_get_switch_def("SER", DEF_SER);
  gl_sock  = m_get_switch_def("SOCK", DEF_SOCK);
  gl_arr   = m_get_switch_def("ARRDISP", DEF_ARRDISP);
  gl_convert = m_get_switch_def("CONVERT", DEF_CONVERT);

  // Change the serialization to SERIALIZE_P2 for faster serialization,
  // Change the sockets to 1 for single socket mode
  // Change the array_disposition to either AS_PYTHON_ARRAY or AS_NUMERIC
  //       for faster array/Vector serialization
  Serialization_e serialization = Serialization_e(gl_ser);
  SocketDuplex_e socket_duplex = DUAL_SOCKET;
  {
    Tab options("{ 1:0, 2:1, 777:777 }");
    int_4 val = options(gl_sock);
    socket_duplex = SocketDuplex_e(val);
  }
  ArrayDisposition_e array_disposition = ArrayDisposition_e(gl_arr); 

  cout << "Trying to connect to server " << host << " on port " << port<<endl;
  MidasTalker mt(host, port, serialization, socket_duplex, array_disposition);
  if (gl_convert) {
    cout << "... you have selected forced conversion mode, which means all\n"
         << "    OTab, Tup, BigInt will be converted to Tab, Arr, Str:\n"
         << "    This is for preserving legacy with pre-existing PTOOLS"
         << "    installations (i.e., backwards compatibility)."
         << endl;
    mt.compatibilityMode(gl_convert);
  }

  // Create array to Permute: [1,2..,n]
  int n = m_lpick(3);
  Arr a;
  for (int ii=0; ii<n; ii++)
    a.append(ii+1);

  m_sync();

  openup(mt);
  
  while (!Mc->break_) {

    try {
      sendloop(mt, a);
      break;
    }
    catch (const runtime_error& e) {
      cout << "Problem: " << e.what() << endl;
      cout << "Server appears to have gone away? Attempting to reconnect" << endl;
      sleep(3);
      openup(mt);
    }
  }
  
  // m_close(hout);
}

