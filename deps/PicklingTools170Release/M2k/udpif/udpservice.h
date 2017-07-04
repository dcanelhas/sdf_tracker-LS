#ifndef UDPSERVICE_H_

//
//
//
// Author    C. Pendergraph 1/2000
//           Mirrored functionality of M2K DualSocketPeer.
//
// Function:   M2K Service for UDP network communication.  May be instanced
//             by itself for testing, but functionally should be used as
//             a parent class, like M2K's Service Class, with applications 
//             making appropriate UDPService::write() or UDPService::writeTo() 
//             calls and extending UDPService::incomingData() to meet their 
//             own needs.
//
//             Example test code can be found in udptst.cc
//
//             M2Shell example use:
//              
//             unitpath + <udpstuff>
//             use UDPService  udp Port = 6667
//
//             ...then use something like udp_socket_client.c to send
//                datagrams to Host:6667;  the UDPService instance, "udp"
//                will act as a receiving server with "well-known" port
//                6667 and print out source host:port, number bytes 
//                received and the UDP datagram in hex.
//
// Note:       RemoteHost & RemotePort attributes only need to be set if
//             inheriting app does 1-to-1 communication with a predefined
//             client/server using UDPService::write() function calls.
//             1-to-N servers would use UDPService::readFrom() & 
//             UDPService::writeTo() calls and keep track of clients within
//             to app.
//             
// Revisions:  
//


// ///////////////////////////////////////////// Include Files

#include "udpsockets.h"
#include "m2service.h"
#include "m2wakeonsocket.h"
#include "m2remotedeployer.h"


// ///////////////////////////////////////////// Type Definitions


// ///////////////////////////////////////////// UDPService Class

class UDPService_Private;


M2RTTI(UDPService)

class UDPService : public Service {

    M2PARENT(Service)

  public:

    // ///// Midas 2k Attributes
    
    //
    // Host - Read only attribute; local hostname.
    //   Type: <string>
    //   DEFAULTs to local hostname UDPService is running on.
    //
    RTReadout<string>                     Host;          // OUTPUT

    //
    // Port - local "source" port number for sending/receiving UDP datagrams;
    //        whether operating as a "server" or "client" this port number
    //        is the UDP socket connection to the network.
    //        A zero (0) forces library/OS to assign one.
    //   Type: <int_4>
    //   DEFAULT = <0>   let OS/library assign one;
    //
    RTParameter<int_u4>                   Port;          // INPUT + OUTPUT

    //
    // RemoteHost - Destination hostname to send UDP datagram 
    // messages to.  Required only for use with UDPService::write()
    // function, when user communication is one-to-one.
    //   Type: <string>
    //   DEFAULT = <localhost>  local host (Host) is default none specified
    //
    RTParameter<string>                   RemoteHost;

    //
    // RemotePort - Destination port number to send UDP datagram
    //              messages to.  Required only for use with 
    //              UDPService::write() function, when user communication
    //              is one-to-one.
    //   Type: <int_4>
    //   DEFAULT = <9141>
    //
    RTParameter<int_u4>                   RemotePort;



    // ///// Methods

    // Constructor

    UDPService ();

    // Destructor

    ~UDPService ();


    // ///// Methods

    // ///// Static Methods
    
    // Get the current host name.
    
    static string ThisHostName ();


    // Get and set the remote hostname & port to send datagrams to. 
    // RemoteHost & RemotePort are only used by the UDPService::write()
    // function.  This is really a "convenience" function for single
    // one-to-one communications so the app can send datagrams w/o
    // having to maintain the destination machine-port address & just
    // use the simple UDPService::read() & UDPService::write() functions.

    string GetRemoteHost () const;
    void   SetRemoteHost (const string& s);

    int_4  GetRemotePort () const;
    void   SetRemotePort (int_4 new_port);

    // Get local hostname UDPService is running on

    string GetHost () const;

    // Get and set the port number to post the service on.

    int_4 GetPort       () const;
    void  SetPort       (int_4 new_port);

    // Overloaded function for the simple, blind read; returns the datagram
    // at the Port.  Does not care where it came from (no client addr returned).
    // This is good for simple data-sucker that just does UDP reads.
    // Valid values for "flags" parameter are consistent with those
    // specifed for your OS Sockets Library (<sys/socket.h>), usually:
    //    MSG_PEEK
    //    MSG_OOB
    //    MSG_WAITALL
    //
    // NOTE: should be used ONLY in extended incomingData() interrupt service 
    //       function.

    int_4 read          (void* buf, const int_4 bytes);
    int_4 read          (void* buf, const int_4 bytes, const int_u4 flags);

    // Simple write to destination defined by RemoteHost:RemotePort attributes

    int_4 write         (const void* buf, const int_4 bytes);

    // Send datagram to destination defined by app; Given the destination
    // Host:Port, app could call UDPSocketLib::UDP_findHostByName() to get 
    // client address OR if UDP service is used as a Server, just store & use 
    // the client address returned in readFrom()

    int_4 writeTo       (const void*         buf, 
                         const int_4         bytes,
                         struct sockaddr_in* clientAddr, 
                         const int_4         clientAddrLength);

    // Send datagram to destination defined by app; uses destination
    // destHost:destPort only.  Client Host:Port is "known" OR if UDP service is
    // used as a Server, just store & use the client Host:Port from one of the 
    // overloaded UDPService::readFrom() functions.

    int_4 writeTo       (const void*         buf, 
                         const int_4         bytes,
                         const string        destHost,
                         const int_4         destPort);

    // Overloaded function for reading UDP datagrams arriving at Host:Port and
    // returning either INET ready source addresses of sender OR Host:Port
    // of sender.
    //
    // NOTE: should be used ONLY in extended incomingData() interrupt service 
    //       function.

    int_4 readFrom      (void*               buf, 
                         const int_4         bytes,
                         const int_u4        flags,   
                         struct sockaddr_in* clientAddr, 
                         const int_4         clientAddrLength);

    int_4 readFrom      (void*               buf, 
                         const int_4         bytes,
                         const int_u4        flags,   
                         string              &destHost,
                         int_4               &destPort);

    // Prints, using cout, raw hex output of UDP datagrams received.

    void debugUDPOutput_ (const unsigned char *buffer, const int_4 NumBytes);
    
    // TODO (csp  2-MAY-00): remove
    // this may not work.... NA for now "once a great notion"... will remove

    string              UDPServiceStatus () const;


    // The single string identifier for a UDP socket is written
    // as the host name, followed by a colon, then the port number.
    // e.g. "<hostname>:<port>",  like  "shaman:9141"
    // Primarily used for logging/announcements..

    // ///// for Host:Port
    virtual string Identifier       () const;

    // ///// for RemoteHost:RemotePort
    virtual string RemoteIdentifier () const;


    // Returns true if UDP socket is open; false if not.

    inline bool IsOpen () const;



  protected:

    // ///// Methods

    // The handle to the underlying "server".
    
    server_type* server_ () const
    { return myServer_; }


    // Called to create a new UDP socket service.  Closes from the existing
    // one.  Could be extended to include app level protocol stuff.  Be sure 
    // to include call to this function from any extension.

    virtual void openNetworkPort_ ();


    // Called to close UDP network socket service.
    // Could be extended to include app level protocol stuff.  Be sure 
    // to include call to this function from any extension.

    virtual void closeNetworkPort_ ();


    // Interrupt service routine called whenever data is present at
    // UDP socket (done thru WakeOnSocket class).  This function SHOULD BE
    // EXTENDED for specific applications.  In its native state, it just
    // reads datagram message that arrived and a debug dump out to screen.

    //virtual void incomingData_ () = 0;
    virtual void incomingData_ ();

    
    // ///// Extend from- Component [M2K]
    // Startup/Initialization routine.

    virtual void stopMyself_ ();

    // ///// Extend from- Component [M2K]
    // Startup/Initialization routine.

    void assembleMyself_ ();


    // ///// Extend from- Component [M2K]
    // Startup/Initialization routine.

    virtual void startMyself_ ();
    virtual void startup_ ();




    // ///// Extend Service

    // The preamble and postamble start and stop the network 
    // connection respectively.

    virtual void preamble_ ();
    virtual void postamble_ ();
    virtual bool takeWakeUpCall_ (WakeCondition& waker);

    // ///// Data Members

    struct  sockaddr_in  localAddr_;
    bool                 validLocalHost_;


    // The WakeOnSocket listens for data arrival at the UDP socket.

    WakeOnSocket	socketNotice_;




  private:

    // ///// Data Members

    string		 host_;
    int_u4               port_;
    string		 remoteHost_;
    int_u4               remotePort_;
    bool	         isOpen_;


    struct  sockaddr_in  remoteAddr_;
    bool                 validRemoteHost_;


    // The server itself.

    server_type*	myServer_;


    UDPService_Private* private_;    
    
    // ///// Methods
    

};					// UDPService



// ///////////////////////////////////////////// Inline Methods

inline bool UDPService::IsOpen () const
{
  return isOpen_;
}



#define UDPSERVICE_H_
#endif // UDPSERVICE_H_


