//----------------------------------------------------------------------------
// More complex X-Midas C++ primitive showing how to write a MidasServer.
// You need at least OpenContainers 1.5.0 and set some flags
// correctly (see the cfg directory)
//----------------------------------------------------------------------------
#include <primitive.h>
#include <midasserver.h>
#include <ocpermutations.h>
#include <iostream>
using namespace std;

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

// In this example, whenever a new client connects, we create a new
// thread to handle the processing.  The client then sends an Arr
// (such as [1,2,3] ) and the client responds with n!  responses,
// where each of the responses are all the different permutations of
// the table (n is the length of the table, so there are n! different
// permutations).  In the case of the request being [1,2,3], the
// server would respond 6 times with [1,2,3], [2,1,3], [3,2,1],
// [1,3,2], [3,1,2], [2,3,1] then disconnect the client.


// PermutationsServer demonstrates how to write your own MidasServer for
// communicating with MidasTalkers
class PermutationServer : public MidasServer {

  public:

  // This is a simple MainLoop which serves out the permutations of
  // its data in order to a client until it finishes.  Each client
  // gets its own thread which computes permutations.

  // When we start the Permutation Main Loop, this is the data we pass in
  struct PermutationData {
    PermutationData (PermutationServer* s, int rfd, int wfd, Arr& t) :
      csp(s), read_fd(rfd), write_fd(wfd), original_data(t), forceDone_(false) { }
    PermutationServer* csp;
    int read_fd, write_fd;
    Arr original_data;
    int n;
    Mutex lock;                   // Lock for checking if done early
    volatile bool forceDone_;  // In case client goes away early
  }; // PermutationData
  
  static void* PermutationMainLoop (void* data) 
  {
    try {
      // Capture all the relevant data in local variables, and delete
      // this stucture for passing data
      PermutationData* pd = (PermutationData*)data;
      //int read_fd  = pd->read_fd; // NOT USED, but could
      int write_fd = pd->write_fd;
      PermutationServer& server = *(pd->csp);
      Arr original_data = pd->original_data;
      
      // Let's permute the Array<Val> (aka Arr) given to us
      PermutationsT<Val> p(original_data);
      while (p.next()) { // Advance to the next Permutation
	Arr a = p.currentPermutation();    // Get the current
	
	// Check and see if we have a premature end
	{ 
	  ProtectScope ps(pd->lock);
	  if (pd->forceDone_) break;
	}
	
	server.sendBlocking_(write_fd, a); // ... send it out
      }
      if (!pd->forceDone_) server.sendBlocking_(write_fd, "EOF");

    } catch (const exception& e) {
      // Don't want to bring down the server if the send fails
      cerr << "Troubling writing back to client?  Probably disconnected:" 
	   << e.what() << " ... continuing and keeping server up." << endl;
    }

    return 0;
  }

  // Constructor: Running the PermutationServer on this host, port
  PermutationServer (const string& host, int port,
		     Serialization_e ser=SERIALIZE_P0, 
		     bool socket_mode=DUAL_SOCKET,
		     ArrayDisposition_e dis=AS_LIST) :
    MidasServer(host, port, ser, socket_mode, dis)
  { }

  virtual void acceptNewClient_ (int read_fd, const SockAddr_& read_addr,
                                 int write_fd, const SockAddr_& write_addr)
  {
    // Create a new thread to be associated with this client.  All we
    // know is that a socket connected, so we can't actually start 
    // the thread to start processing until we get the first request.
    OCThread* tp = new OCThread("client"+Stringize(read_fd), false);
    clientList_[read_fd] = tp;

    // And show the world that we saw the connect
    cout<<"Permutations:Connection ("<<read_fd<<","<< write_fd<<")"<<endl;
  }

  virtual void readClientData_ (int read_fd, int write_fd, const Val& data)
  {
    // Clients are mapped to their thread by their read_fd
    cout<<"Permutations:Client ("<<read_fd<<","<<write_fd<<")"<<endl;
    
    OCThread* client_thread = clientList_[read_fd];

    // Make sure legal request
    Arr t;
    try {
      cerr << "DATA TAG for data is " << data.tag << " " << data << endl;
      t = data;  // Get the array to permute
    } catch (const logic_error& e) {
      cerr << e.what() << endl;
      cerr << "Warning!  Can only send Arrs of data to the Permutation "
        " Server.  Your request of " << data << " will be ignored." << endl;
      return;
    } 
    
    if (client_thread->started()) {
      ; // Don't do anything if we get another request and have started
    } else {
      PermutationData* pd = new PermutationData(this, read_fd, write_fd, t);
      clientData_[read_fd] = pd;
      client_thread->start(PermutationMainLoop, pd);
    }
  }

  virtual void disconnectClient_ (int read_fd, int write_fd) 
  { 
    // Simple threads: we want to make sure resources shut down a
    // little more cleanly, so are more careful and do joins so we
    // know when the thread resources are gone.
    OCThread* client_thread  = clientList_[read_fd];
    clientList_.remove(read_fd); 
    PermutationData* client_data = clientData_[read_fd];
    clientData_.remove(read_fd);
    {
      ProtectScope ps(client_data->lock);
      client_data->forceDone_ = true;
    }
    // The delete client_thread does the join
    delete client_thread;
    delete client_data;

    cout << "Permutations: Client ("<<read_fd<<","<<write_fd<<")"<< endl
         << "          disconnected." << endl;
  }

  protected:
  
  AVLHashT<Val, OCThread*, 8> clientList_;
  AVLHashT<Val, PermutationData*, 8> clientData_;

}; // PermutationServer


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

  cout << "Opening up the server on " << host << " " << port << endl;
  PermutationServer mt(host, port, 
		       serialization, socket_duplex, array_disposition);
  if (gl_convert) {
    cout << "... you have selected forced conversion mode, which means all\n"
         << "    OTab, Tup, BigInt will be converted to Tab, Arr, Str:\n"
         << "    This is for preserving legacy with pre-existing PTOOLS"
         << "    installations (i.e., backwards compatibility)."
         << endl;
    mt.compatibilityMode(gl_convert);
  }
  mt.open();

  // We know the server has been created
  m_sync();

  // Wake up every so often to see if we are quitting
  while (!Mc->break_) {
    m_pause(1);
  }
  mt.shutdown();
  mt.waitForMainLoopToFinish();
}

