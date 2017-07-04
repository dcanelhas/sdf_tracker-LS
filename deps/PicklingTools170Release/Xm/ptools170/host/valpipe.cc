//////////////////////////////////////////////////////////////////////////////
//  CLASSIFICATION: UNCLASSIFIED
//
//  FILENAME: valpipe.cc
//
//  DESCRIPTION:
//    zpipe is used to tx/rx X-Midas pipes via TCP/IP but only supports
//    sending T1000 files.  This primitive is used to tx/rx Vals via TCP/IP
//    and uses T4000 files to tx/rx the Val across X-Midas pipes.
//
//  SWITCHES:
//    See explain page for switch listing.
//
//  REVISION HISTORY:
//    NAME    DATE        DESCRIPTION OF CHANGE
//  -------------------------------------------------------------------------
//    jdg     29Sep06     Initial using PTOOLS 082
//    jdg     19Feb07     Fixed to make operational with PTOOLS 083
//    jdg     27Feb07     Fixed XM_OUT/XM_IN reversal
//
//////////////////////////////////////////////////////////////////////////////
#include <primitive.h>
#include <midasserver.h>
#include <midastalker.h>
#include <t4val.h>
#include <ocvalreader.h>

#define DEF_SER     SERIALIZE_P0
#define DEF_ARRDISP AS_LIST
#define DEF_SOCK    1

string gl_vname; // Underlying name to populate the vrb struture.
int_4 gl_ser;
int_4 gl_sock;
int_4 gl_arr;

#if defined(OC_FORCE_NAMESPACE)
using namespace OC;
#endif


//////////////////////////////////////////////////////////////////////////////
// CLASS: MyMidasServer
//
// DESCRIPTION:
//   MidasServer used to send and receive Vals with T4000 support.
//
//////////////////////////////////////////////////////////////////////////////
class MyMidasServer : public MidasServer {
  public:
    // Constructor
    MyMidasServer (string host, int port,
      Serialization_e serial=Serialization_e(DEF_SER),
      bool dual_socket=(DEF_SOCK==2) ? true : false,
      ArrayDisposition_e dis=ArrayDisposition_e(DEF_ARRDISP)) :
      MidasServer(host, port, serial, dual_socket, dis),
      isXmIn_(false),
      isXmOut_(false)
    { }

    // Destructor
    virtual ~MyMidasServer () { }

    // Displays list of connected clients.
    void displayClients()
    {
      clientFDTable_.prettyPrint(cerr);
    }

    void setHcb(CPHEADER& hcb)
    {
      phcb_ = &hcb;
    }

    void setXmIn(bool v) { isXmIn_ = v; }
    void setXmOut(bool v) { isXmOut_ = v; }

    // Sends data to all connected clients.
    void send(Val& data)
    {
      if (isXmIn_) return;

      // Send Val data to all connected clients.
      for (It ii(clientFDTable_); ii(); ) {
        if (Mc->break_) break;
        sendBlocking_(clientFDTable_[ii.key()][1], data);
      }
    }

  protected:
    CPHEADER *phcb_;
    bool isXmIn_;
    bool isXmOut_;

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
      if (Mu->verbose) {
        m_info("SERVER: Client Connected [" +
          Stringize(read_fd) + ":" + Stringize(write_fd) + "]");
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
      if (Mc->debug & 0x00000020) {
        m_info("SERVER: Client Got Data [" +
          Stringize(read_fd) + ":" + Stringize(write_fd) + "]");
      }

      // Send recieved Val data out to T4000 file/pipe.
      if (isXmIn_) {
        sendT4Val(*phcb_, data, gl_vname);
        if (Mc->debug & 0x00000020) {
          data.prettyPrint(cerr);
        }
      }
    }

    // User Callback for when a client disconnects.  The read_fd
    // distiguishes which client is disconnecting.  The read_fd and
    // write_fd will be closed almost immediately after this method
    // returns (the user does not have to close the descriptors).
    virtual void disconnectClient_ (int read_fd, int write_fd) 
    {
      if (Mu->verbose) {
        m_info("SERVER: Client Disconnected [" +
          Stringize(read_fd) + ":" + Stringize(write_fd) + "]");
      }
    }
  
}; // MyMidasServer

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: openUpServer
//
// DESCRIPTION:
//   Tries to open up a connection for a MidasServer.
//
//////////////////////////////////////////////////////////////////////////////
void openUpServer(MidasServer& ms, Tab& tr, real_8 wait)
{
  // Set the start time.
  real_8 tnow, ft;
  m_now(tnow, ft);
  tnow += ft;

  // Is it time to try and open again.
  if (tnow < real_8(tr["TRYTIME"])) {
    return;
  }

  // Open the Midas Talker/Server.
  try {
    ms.open(wait);
    m_info("Connection Open: [" + ms.host() + "@" + Stringize(ms.port()) + "]");
    tr["UPTIME"] = tnow;
    tr["ISOPEN"] = true;
    if (Mu->verbose) {
      tr.prettyPrint(cerr);
    }
  }
  // Open failed, set to try again later.
  catch (const runtime_error& e) {
    if (bool(tr["ISOPEN"])) {
      m_warning("Problem: " + Stringize(e.what()));
      m_advisory("Connection Failed: [" + ms.host() + "@" + Stringize(ms.port()) +
        "] attempting to reconnect...");

      // Set to false and only display once.
      tr["ISOPEN"] = false;
      if (Mu->verbose) {
        tr.prettyPrint(cerr);
      }
    }

    tr["ISOPEN"] = false;
    tr["TRYTIME"] = tnow + wait;
  }
}

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: openUpTalker
//
// DESCRIPTION:
//   Tries to open up a connection for a MidasTalker.
//
//////////////////////////////////////////////////////////////////////////////
void openUpTalker(MidasTalker& mt, Tab& tr, real_8 wait)
{
  // Set the start time.
  real_8 tnow, ft;
  m_now(tnow, ft);
  tnow += ft;

  // Is it time to try and open again.
  if (tnow < real_8(tr["TRYTIME"])) {
    return;
  }

  // Open the Midas Talker/Server.
  try {
    mt.open();
    m_info("Connection Open: [" + mt.host() + "@" + Stringize(mt.port()) + "]");
    tr["UPTIME"] = tnow;
    tr["ISOPEN"] = true;
    if (Mu->verbose) {
      tr.prettyPrint(cerr);
    }
  }
  // Open failed, set to try again later.
  catch (const runtime_error& e) {
    if (bool(tr["ISOPEN"])) {
      m_warning("Problem: " + Stringize(e.what()));
      m_advisory("Connection Failed: [" + mt.host() + "@" + Stringize(mt.port()) +
        "] attempting to reconnect...");

      // Set to false and only display once.
      tr["ISOPEN"] = false;
      if (Mu->verbose) {
        tr.prettyPrint(cerr);
      }
    }

    tr["ISOPEN"] = false;
    tr["TRYTIME"] = tnow + wait;
  }
}
void mainroutine()
{
  CPHEADER hcb;

  MyMidasServer *ms;
  MidasTalker *mt;
  Tab tConnection = Tab();
  string host = m_upick(3);
  int_4 port  = m_lpick(2);
  bool gotData = false;
  bool exitNow = false;

  real_8 tnow, ft;
  real_8 lastWarnTime = 0.0;

  // Switches
  bool isServer   = (m_get_switch("SERVER") > 0);
  bool isXmIn     = (m_get_switch("XM_IN") > 0);
  bool isXmOut    = (m_get_switch("XM_OUT") > 0);
  bool doDrop     = (m_get_switch("DROP") > 0);
  real_8 wait     = m_get_dswitch_def("WAIT", Mc->pause);
  real_8 warnTime = m_get_dswitch_def("CWARN", 0);
  if (m_get_uswitch("VNAME", gl_vname) <= 0) {
    gl_vname = DEFAULT_VRBKEY;
  }
  gl_ser   = m_get_switch_def("SER", DEF_SER);
  gl_sock  = m_get_switch_def("SOCK", DEF_SOCK);
  gl_arr   = m_get_switch_def("ARRDISP", DEF_ARRDISP);

  if (Mu->verbose) {
    cerr << "Serialization:     " << gl_ser << endl;
    cerr << "Socket:            " << gl_sock << endl;
    cerr << "Array Disposition: " << gl_arr << endl;
  }
  // Translate gl_sock into enumeration for socket duplex
  SocketDuplex_e socket_duplex = DUAL_SOCKET;
  {
    Tab options("{ 1:0, 2:1, 777:777 }");
    int_4 val = options(gl_sock);
    socket_duplex = SocketDuplex_e(val);
  }

  // Initialize the file/pipe.
  if (isXmIn && isXmOut) {
    m_error("Both /XM_IN and /XM_OUT switches set.");
  }
  else if (isXmIn) {
    initT4Val(hcb, m_apick(1), HCBF_OUTPUT);
  }
  else if (isXmOut) {
    initT4Val(hcb, m_apick(1), HCBF_INPUT);
  }
  else {
    m_error("Neither /XM_IN nor /XM_OUT switch set.");
  }

  if (!isServer && host.size() <= 0) {
    m_error("Client needs server hostname.");
  }

  if (doDrop) {
    m_info("/DROP switch detected.");
  }

  if (warnTime > 0) {
    m_info("/CWARN set to " + Stringize(warnTime) + " minutes");
    warnTime *= 60;
  }

  m_sync();

  // Initialize connection status.
  tConnection["TRYTIME"]  = 0.0;
  tConnection["UPTIME"]   = 0.0;
  tConnection["ISOPEN"]   = true;
  tConnection["COUNT"]    = 0;

  // SERVER
  if (isServer) {
    if (Mu->verbose) {
      m_info("Starting Server");
    }
    ms = new MyMidasServer(Mc->hostname, port,
			   Serialization_e(gl_ser), 
			   socket_duplex,
			   ArrayDisposition_e(gl_arr));
    ms->setHcb(hcb);
    ms->setXmIn(isXmIn);
    ms->setXmOut(isXmOut);
    openUpServer(*ms, tConnection, wait);
    
    while (!Mc->break_) {
      // Get the current time.
      if (!gotData) {
        m_pause(Mc->pause);
      }
      gotData = false;
      m_now(tnow, ft);
      tnow += ft;

      // XM_OUT
      if (bool(tConnection["ISOPEN"]) || doDrop) {
        if (isXmOut) {
          // Grab the Val from the file/pipe.
          Val v;
          if (recvT4Val(hcb, v, gl_vname) && bool(tConnection["ISOPEN"])) {
            if (Mc->debug & 0x00000020) {
              v.prettyPrint(cerr);
            }
            // Send the Val to the socket.
            try {
              ms->send(v);
              tConnection["COUNT"] = int_4(tConnection["COUNT"]) + 1;
            } catch (const runtime_error& e) {
              tConnection["TRYTIME"] = tnow + wait;
              tConnection["ISOPEN"] = false;

              if (Mu->verbose) {
                m_warning("Problem: " + Stringize(e.what()));
                m_advisory("Connection Failed: [" + ms->host() + "@" + Stringize(ms->port()) +
                  "] attempting to reconnect...");
                tConnection.prettyPrint(cerr);
              }
            }
            gotData = true;
          }
        }
      }

      // Connect if not connection.
      if (!bool(tConnection["ISOPEN"])) {
        openUpServer(*ms, tConnection, wait);
      }
    }
    // delete ms; // Seems to cause seg-fault on way out
  }
  
  // CLIENT
  else {
    if (Mu->verbose) {
      m_info("Starting Client");
    }
    mt = new MidasTalker(host, port,
			 Serialization_e(gl_ser), 
			 socket_duplex,
			 ArrayDisposition_e(gl_arr));
    openUpTalker(*mt, tConnection, wait);
    
    while (!Mc->break_ && !exitNow) { 
      // Get the current time.
      if (!gotData) {
        m_pause(Mc->pause);
      }
      gotData = false;
      m_now(tnow, ft);
      tnow += ft;

      if (bool(tConnection["ISOPEN"]) || doDrop) {
        try {
          // XM_OUT
          if (isXmOut) {
            // Grab the Val from the file/pipe.
            Val v;
            if (recvT4Val(hcb, v, gl_vname) && bool(tConnection["ISOPEN"])) {
              if (Mc->debug & 0x00000020) {
                v.prettyPrint(cerr);
              }
              // Send the Val to the socket.
              mt->send(v, wait);
              tConnection["COUNT"] = int_4(tConnection["COUNT"]) + 1;
              gotData = true;
            } else if (hcb.pipe == 0) {
	      exitNow = true;  // EOF reached.
	    }
          }
          // XM_IN
          else {
            // Receive the Val from the socket.
            if (bool(tConnection["ISOPEN"])) {
              Val v = mt->recv(wait);

              // Send the Val out the file/pipe.
              if (v != None) {
                sendT4Val(hcb, v, gl_vname);

                if (Mc->debug & 0x00000020) {
                  v.prettyPrint(cerr);
                }
 
                tConnection["COUNT"] = int_4(tConnection["COUNT"]) + 1;
                gotData = true;
              }
            }
          }
        }
        catch (const runtime_error& e) {
          tConnection["TRYTIME"] = tnow + wait;
          tConnection["ISOPEN"] = false;

          if (Mu->verbose) {
            m_warning("Problem: " + Stringize(e.what()));
            m_advisory("Connection Failed: [" + mt->host() + "@" + Stringize(mt->port()) +
              "] attempting to reconnect...");
            tConnection.prettyPrint(cerr);
          }
        }
      }

      // Connect if not connection.
      if (!bool(tConnection["ISOPEN"])) {
        if ((warnTime > 0) && (tnow - lastWarnTime > warnTime)) {
          lastWarnTime = tnow;
          m_warning("Connection Not Open: [" + mt->host() + "@" + Stringize(mt->port()) +
            "] attempting to reconnect...");
        }
        openUpTalker(*mt, tConnection, wait);
      }
    }
    delete mt;
  }

  if (hcb.open) m_close(hcb);

  if (Mu->verbose) {
    m_info("Count: [" + Stringize(tConnection["COUNT"]) + "]");
  }
}
