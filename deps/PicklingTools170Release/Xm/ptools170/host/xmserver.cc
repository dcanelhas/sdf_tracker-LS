//----------------------------------------------------------------------------
// Sample X-Midas C++ primitive showing how to write a MidasServer.
// You need at least OpenContainers 1.5.1 and set some flags
// correctly (see the cfg directory)
//----------------------------------------------------------------------------
#include <primitive.h>
#include <midasserver.h>

#if defined(OC_FORCE_NAMESPACE)
using namespace OC;
#endif

#define DEF_SER     SERIALIZE_P0
#define DEF_SOCK    2
#define DEF_ARRDISP AS_LIST
#define DEF_CONVERT false

int_4 gl_ser;
int_4 gl_sock;
int_4 gl_arr;
int_4 gl_convert;

// To write your own socket socket, you inherit from MidasServer,
// and there is a callback event whenever one of three events
// happen:  (1) New client connects 
//          (2) Client has a request/data for the server 
//          (3) Client disconnects
// The MidasServer runs in a thread (yes a *thread*) that wakes up and
// animates the callback whenever one of the above events occurs.

// There are two modes, single-socket and dual-socket mode
// (corresponding to X-Midas history).  Even though dual-socket can
// introduce a race condition when there are frequent connects and
// disconnects, it is the default because most X-Midas and M2k
// components use dual socket mode.  Dual-socket mode means there is
// one socket to read from client-to-host and a different socket to
// read from host-to-client.  In single socket mode, the same socket
// can read/write both directions (and thus only needs one file
// descriptor).

// This example is very simple to show how to use the callbacks:
// there is a more complicated example showing how to use the
// server with multiple threads called the PermutationServer.

class MyServer : public MidasServer {
  public:

    // Provide port and host.  Your host is almost always the same
    // as the localhost, unless you wish to use a different interface.
    MyServer (string host="localhost", int port=9111,
	      Serialization_e ser=SERIALIZE_P0, 
	      bool socket_mode=DUAL_SOCKET,
	      ArrayDisposition_e dis=AS_LIST) :
      MidasServer(host, port, ser, socket_mode, dis),
      count_(0)
    { }

    // Destructor
    virtual ~MyServer () { }

  protected:
  
    int count_;   // Number of messages sent 


    // User Callback for when a new client is added.  Each client can
    // be distiguished by its read_fd (a unique number for each
    // client).  The read_addr (and write_addr) gives more information
    // about the connection: See the man page for accept and struct
    // defintion above. In dual-socket mode, the read_fd and write_fd
    // are different numbers, in single-socket mode, they are the
    // same.
    virtual void acceptNewClient_ (int read_fd, const SockAddr_& read_addr, 
				   int write_fd, const SockAddr_& write_addr) 
    { 
      cout << "MYSERVER: Connection (" <<read_fd<<","<< write_fd<<")" << endl 
	   << "   made from:" 
	   << read_addr.addr.sa_data << " " << write_addr.addr.sa_data 
	   << endl;

      // Send out a test table to client who just connected
      Val data = Tab("{ 'TEST': 'at connect', 'a':[1,2,3] }");

      try {
	sendBlocking_(write_fd, data);
      } catch (const exception& e) {
	// Don't want to bring down the server if the send fails
	cerr << "Troubling writing back to client?  Probably disconnected:" 
	     << e.what() << " ... continuing and keeping server up." << endl;
      }

    }

    // User Callback for when client data is posted.  The read_fd
    // identifies the client that sent the data.  The write_fd is the
    // file descriptor to use to send data back to that client.  In
    // dual-socket mode, the read_fd and write_fd are different
    // numbers, in single-socket mode, they are the same.  Use
    // 'sendBlocking_(write_fd, result)' to send data back to the
    // client as a response.
    virtual void readClientData_ (int read_fd, int write_fd, const Val& data) 
    { 
      cout << "MYSERVER: Client ("<< read_fd << "," << write_fd << ")" << endl
	   << "          saw data:" << data << endl;

      try {
	// Send the same data back to the client who sent it
	sendBlocking_(write_fd, data);
	cout << " ... and send the same data back!" << endl;
	
	// Show how to allow shutdown
	int max_count = 10000;
	count_+=1;
	if (count_>max_count) {
	  cout << "... saw " << max_count << " messages ... shutting down"<<endl;
	  shutdown();
	}

      } catch (const exception& e) {
	// Don't want to bring down the server if the send fails
	cerr << "Troubling writing back to client?  Probably disconnected:" 
	     << e.what() << " ... continuing and keeping server up." << endl;
      }

    }

    // User Callback for when a client disconnects.  The read_fd
    // distiguishes which client is disconnecting.  The read_fd and
    // write_fd will be closed almost immediately after this method
    // returns (the user does not have to close the descriptors).
    virtual void disconnectClient_ (int read_fd, int write_fd) 
    {
      cout << "MYSERVER: Client ("<<read_fd<<","<<write_fd << ")"<< endl
	   << "          disconnected." << endl;
    }
  
}; // MyServer


void mainroutine()
{
  // Create the socket server
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

  MyServer mt(host, port, serialization, socket_duplex, array_disposition);
  cout << "Opening MyServer on host " << host << " port " << port << endl;
  if (gl_convert) {
    cout << "... you have selected forced conversion mode, which means all\n"
         << "    OTab, Tup, BigInt will be converted to Tab, Arr, Str:\n"
         << "    This is for preserving legacy with pre-existing PTOOLS"
         << "    installations (i.e., backwards compatibility)."
         << endl;
    mt.compatibilityMode(gl_convert);
  }

  try {
    mt.open();
  } catch (const exception &e) {
    m_error("" + Stringize(e.what()));
  }

  // We know the server has been created
  m_sync();

  // Wake up every so often to see if we are quitting
  while (!Mc->break_) {
    m_pause(1);    
  }
  mt.shutdown();
  mt.waitForMainLoopToFinish();
}

