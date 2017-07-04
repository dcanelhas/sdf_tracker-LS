//----------------------------------------------------------------------------
// Sample X-Midas C++ primitive showing how to use an HTTP Client
// You need at least OpenContainers 1.4.1 and set some flags
// correctly (see the m2krec.b file to see how to set flags).
//----------------------------------------------------------------------------
#include <primitive.h>
#include <httplib.h>

#if defined(OC_FORCE_NAMESPACE)
using namespace OC;
#endif


// Main loop to receive messages to the HTTPConnection
void recvloop (HTTPConnection& conn)
{
  // See if we can receive, wait 3 seconds
  while (!Mc->break_) {

    try {
      conn.request("GET", "/index.html");
      cout << "Sent a GET" << endl;
    }
    catch (const runtime_error& e) {
      cout << "Problem: " << e.what() << endl;
      cout << "...couldn't open right away, backing off 3 seconds" << endl;
      sleep(3);
      continue;
    }

    
    HTTPResponse r1 = conn.getresponse();
    cout << r1.status() << " " << r1.reason() << endl;
    // // output: 200 OK
    Array<char> data1 = r1.read();
    cout << "data from chunk 1" << data1 << endl;
    
    Array<char> data2 = r1.read();
    cout << "data from chunk 2" << data2 << endl;
    
    conn.close();
  } 
  
}


void mainroutine()
{
  // input from socket
  string host = m_apick(1);
  int port    = m_lpick(2);

  cout << "Trying to connect to server " << host << " on port " << port<<endl;

  m_sync();
  
  // The connection 
  HTTPConnection conn(host, port);
  try {
    recvloop(conn);
  }
  catch (const runtime_error& e) {
    cout << "Problem: " << e.what() << endl;
    cout << "Server appears to have gone away..." << endl;
  }
    
}

