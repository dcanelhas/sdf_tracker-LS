// ///////////////////////////////////////////// Include Files

#include <unistd.h>

#include "udpservice.h"
#include "m2sockete.h"


// ///////////////////////////////////////////// Private Classes

class UDPService_Private {

  public:

    // ///// Data Members

    // Use same socket for reading and writing.

    int_4		socket_fd;

    // ///// Methods

    UDPService_Private () :
      socket_fd(-1)
    { }

    void clearDescriptors ()
    {
      if (-1 != socket_fd) {
	UDPSocketLib::UDP_closeSocket(socket_fd);
	socket_fd = -1;
      }
    }

};



// ///////////////////////////////////////////// UDPService Methods


//
// Preconditions: 
//
// Postconditions: 
//
// Parameters:
//
// Description:
//
//  Contructor
//

UDPService::UDPService () :
  M2ATTR_VAL(Host,       ""),
  M2ATTR_VAL(Port,       0),
  M2ATTR_VAL(RemoteHost, ""),
  M2ATTR_VAL(RemotePort, 9141),
  port_(0),
  host_(""),
  isOpen_(false),
  remoteHost_(""),
  remotePort_(9141)
{
  private_ = new UDPService_Private;

  addWakeUp_(socketNotice_, "socket");
  addWakeUp_(Port,          "PortChange");
  addWakeUp_(RemoteHost,    "RemoteHostChange");
  addWakeUp_(RemotePort,    "RemotePortChange");

  Host.post(ThisHostName());
  SetRemoteHost (ThisHostName());
  remotePort_         = RemotePort.value();
}


//
// Preconditions: 
//
// Postconditions: 
//
// Parameters:
//
// Description:
//
//  Destructor
//

UDPService::~UDPService ()
{
  closeNetworkPort_();
  delete private_;
}


//
// Preconditions: 
//
// Postconditions:
//   Returns:  string containing local hostname or NULL string if failed. 
//
// Parameters:
//
// Description:
//
//  Utility routine for inheriting application to get local hostname 
//  programmatically.  Same value as UDPService::Host attribute.
//  Routine is used in UDPService to set value of Host attribute.
//
//  Note: recommend UDPService::GetHost() function for this purpose
//        in inheriting apps.
//

string UDPService::ThisHostName ()
{
  char hostname[1024];
  if (gethostname(hostname, 1024) < 0) {
    perror("gethostname");
    return string();
  } else
    return string(hostname);
}


//
// Preconditions: 
//
// Postconditions:
//   Returns:  string containing value of UDPService::RemoteHost
//             attribute.
//
// Parameters:
//
// Description:
//
//  Utility routine for inheriting application to get remote hostname 
//  programmatically.  Same value as UDPService::RemoteHost attribute.
//

string UDPService::GetRemoteHost () const
{
  return remoteHost_;
}


//
// Preconditions: 
//
// Postconditions:
//  RemoteHost attribute is set to parameter value.
//
// Parameters:
//  const string& s   IN;   Remote hostname for UDPService::write()
//                          function to send UDP datagrams to.
//
// Description:
//
//  Utility routine for inheriting application to set 
//  UDPService::RemoteHost attribute programmatically.
//

void UDPService::SetRemoteHost (const string& s)
{
  remoteHost_ = s;
  RemoteHost.value(remoteHost_);
}


//
// Preconditions: 
//
// Postconditions:
//  RemotePort attribute is set to parameter value.
//
// Parameters:
//  int_4         n   IN;   Remote port number for UDPService::write()
//                          function to send UDP datagrams to.
//
// Description:
//
//  Utility routine for inheriting application to set 
//  UDPService::RemotePort attribute programmatically.
//

void UDPService::SetRemotePort (int_4 n)
{
  remotePort_ = n;
  RemotePort.value(remotePort_);
}


//
// Preconditions: 
//
// Postconditions:
//  Closes previously opened UDP socket port and re-opens new one
//  defined by parameter.
//
// Parameters:
//  int_4         n   IN;   Local (source) port number for receiving UDP 
//                          datagrams and through which sends are made over
//                          network.
//
// Description:
//
//  Utility routine for inheriting application to set 
//  UDPService::Port attribute programmatically.
//

void UDPService::SetPort (int_4 n)
{
  port_ = n;
  Port.value(port_);
}

//
// Preconditions: 
//
// Postconditions:
//  Returns value of UDPService::RemotePort Attribute.
//
// Parameters:
//
// Description:
//
//  Utility routine for inheriting application to get 
//  UDPService::RemotePort attribute programmatically.
//

int_4 UDPService::GetRemotePort () const
{
  return remotePort_;
}


//
// Preconditions: 
//
// Postconditions:
//   Returns:  string containing local hostname or NULL string if failed. 
//
// Parameters:
//
// Description:
//
//  Utility routine for inheriting application to get local hostname 
//  programmatically.  Same value as UDPService::Host attribute.
//
//

string UDPService::GetHost () const
{
  return host_;
}


//
// Preconditions: 
//
// Postconditions:
//   Returns:  hostname & port identifier over which current UDP network
//             service is active.  Returned string contains locale hostname
//             and port, delimited by a colon: <hostname:<port>, e.g.
//                                              "shaman:6667"
//
// Parameters:
//
// Description:
//
//  Utility routine for inheriting application to get hostname:port
//  UDP service identifier.  Intended for use in logging, error 
//  reporting, etc.
//
//

string UDPService::Identifier () const
{
  return string(GetHost() + ":" + IntToString(GetPort()));
}


//
// Preconditions: 
//
// Postconditions:
//   Returns:  remote hostname & port identifier to which UDPService::write()
//             will send UDP datagram messages.  Returned string contains remote
//             hostname and port, delimited by a colon: <hostname:<port>, e.g.
//                                                        "shaman:9141"
//
// Parameters:
//
// Description:
//
//  Utility routine for inheriting application to get default destination 
//  hostname:port identifier.  Intended for use in logging, error 
//  reporting, etc.
//
//

string UDPService::RemoteIdentifier () const
{
  return string(GetRemoteHost() + ":" + IntToString(GetRemotePort()));
}


int_4 UDPService::GetPort () const
{
  return port_;
}


// Extend from Service-Primitive-Component

void UDPService::preamble_ ()
{
  Service::preamble_();
  //
  // Not here...
  //
  //openNetworkPort_();
}


// Extend from Service-Primitive-Component

void UDPService::postamble_ ()
{
  socketNotice_.disable();
  closeNetworkPort_();
  Service::postamble_();
}


// Extend from Service-Primitive-Component

void UDPService::assembleMyself_ () 
{
  Service::assembleMyself_();
}                                          // UDPService::AssembleMyself_





// Extend from Service-Primitive-Component

void UDPService::startMyself_ () 
{
  Service::startMyself_();
}                                          // UDPService::startMyself_



// Extend from Service-Primitive-Component

void UDPService::startup_ () 
{

  Service::startup_();

  port_               = Port.value();
  remotePort_         = RemotePort.value();
  remoteHost_         = RemoteHost.value();

  Host.post(ThisHostName());
  host_               = Host.value();
  //Port.value(port_);
  //RemoteHost.value(remoteHost_);
  //RemotePort.value(remotePort_);

  char  hname[64];
  int_4 stat = UDPSocketLib::UDP_findHostByName(remoteHost_.c_str(), 
                                                hname, 
                                                sizeof(hname), 
                                                &remoteAddr_);
  if (stat < 0) {
      log_.error("UDPService:: Hostname lookup FAILED; remoteHost_= <" +
                 remoteHost_ + ">");
    validRemoteHost_ = false;
  } else
    validRemoteHost_ = true;

  remoteAddr_.sin_port = htons((u_short) remotePort_);


  stat = UDPSocketLib::UDP_findHostByName(host_.c_str(), 
                                          hname, 
                                          sizeof(hname), 
                                          &localAddr_);
  if (stat < 0) {
      log_.error("UDPService:: Lookup FAILED for local host; host_= <" +
                 host_ + ">");
    validLocalHost_ = false;
  } else
    validLocalHost_ = true;

  localAddr_.sin_port = htons((u_short) port_);

  openNetworkPort_();

}                                          // UDPService::startup_



// Extend from Service-Primitive-Component

void UDPService::stopMyself_ ()
{
  closeNetworkPort_();
  Service::stopMyself_();
}




//
// Preconditions: 
//
// Postconditions:
//  Opened UDP network socket; load private socket file descriptor.
//
// Parameters:
//
// Description:
//
//  Opens UDP network socket interface on port number specified by Port.
//
//

void UDPService::openNetworkPort_ ()
{
  //
  // Disable interrupts on socket
  //
  socketNotice_.disable();

  closeNetworkPort_();
  
  try {
    socketNotice_.clearDescriptors();
    int_4 try_port = port_;
    if (UDPSocketLib::UDP_openPort(&myServer_, try_port) == 0) {
      if (try_port != port_) {
        port_ = try_port;
        //Port.value(port_);
      }
      private_->socket_fd = UDPSocketLib::descriptorForServer(server_());

      //
      // Enable socket interrupts
      //
      socketNotice_.addDescriptor(UDPSocketLib::descriptorForServer(server_()));
      socketNotice_.enable();

      isOpen_ = true;

        log_.message("UDPService:: Opened UDP port: <" + Identifier() + ">;  "+
                     "Socket fd= <"+Stringize(UDPSocketLib::descriptorForServer(server_()))+">");
    }
  } catch (const SocketLowLevelEx& e) {
    log_.noteException(e);
    log_.warning("UDPService:: Unable to open UDP port: <" + Identifier() + ">");
    closeNetworkPort_();
  }
}					// UDPService::openNetworkPort_


//
// Preconditions: 
//
// Postconditions:
//  Closed previously opened UDP network socket; 
//
// Parameters:
//
// Description:
//
//  Closes UDP network socket interface.
//
//

void UDPService::closeNetworkPort_ ()
{
  if (IsOpen()) {
    //cout<<"actualServerPort="<<UDPSocketLib::actualServerPort(server_())<<endl;
    //  log_.message("UDPService:: Closing UDP port: <" + Identifier() + ">;  "+
    //               "Socket fd= <"+Stringize(UDPSocketLib::descriptorForServer(server_()))+">");
    socketNotice_.disable();
    socketNotice_.clearDescriptors();
    UDPSocketLib::destroyServer(myServer_);
    private_->clearDescriptors();
    isOpen_ = false;
    log_.message("UDPService:: Closed UDP port: <" + Identifier() + ">");
  }
}					// UDPService::closeNetworkPort_



//
// Preconditions: 
//
// Postconditions:
//
// Parameters:
//
// Description:
//
//  Interrupt service routine for processing attribute changes and
//  "socket" changes (UDP datagram arrival) from WakeOnSocket
//  Extended from Service inheritance.
//
//
//

bool UDPService::takeWakeUpCall_ (WakeCondition& waker)
{
  string reason = waker.groupID();

  //
  // Not sure if i want it here...
  //
  //acknowledge_();

  //
  // Port attribute change
  //
  if (reason == "PortChange") {
    int_4 NewPort = Port.value();
    closeNetworkPort_();
    port_ = NewPort;
    log_.message("UDPService:: Changed Port attribute = <" + 
		 Stringize(port_) + ">");
    openNetworkPort_();
    if (port_ != Port.value()) {
      Port.value(port_);
    }
    acknowledge_();
    return true;
  }

  //
  // process socket wake condition
  //   assume UDP datagram received...
  //
  else if (reason == "socket") {          // some socket condition
    incomingData_();
    acknowledge_();
    return true;
  }				

  //
  // process RemoteHost attribute change
  //
  else if (reason == "RemoteHostChange") {
    remoteHost_         = RemoteHost.value();
    log_.message ("UDPService:: Changed RemoteHost attribute = <"+remoteHost_+">");
    char  hname[64];
    int_4 stat = UDPSocketLib::UDP_findHostByName(remoteHost_.c_str(), 
                                                hname, 
                                                sizeof(hname), 
                                                &remoteAddr_);
    if (stat < 0) {
        log_.error("UDPService:: Hostname lookup FAILED; RemoteHost= <" +
                   remoteHost_ + ">");
      validRemoteHost_ = false;
    } else
      validRemoteHost_ = true;

    acknowledge_();

    return true;
  }

  //
  // process RemotePort attribute change
  //
  else if (reason == "RemotePortChange") {
    remotePort_         = RemotePort.value();
    remoteAddr_.sin_port = htons((u_short) remotePort_);
    log_.message("UDPService:: Changed RemotePort attribute = <" + 
		 Stringize(remotePort_) + ">");
    acknowledge_();
    return true;
  } else {

    log_.warning("UDPService:: Unknown WakeCondition \"" + reason + "\"");
  }


  return true;
}



//
// Preconditions: 
//
// Postconditions:
//
// Parameters:
//
// Description:
//
//  Called from Service::takeWakeUpCall() interrupt service routine for 
//  processing WakeOnSocket "socket" changes.  Assume UDP datagram arrival;
//  read buffer into application from TCP/IP (UDP) suite and process 
//  application message/data.
//
//  NOTE: This is a virtual function and is intended to be extended for 
//  application level processing of data.  This default just reads data and 
//  prints hex to screen.
//        
//
//

void UDPService::incomingData_ ()
{ 
  int_u1               buffer[8192];
  int_4                length = sizeof(buffer);
  int_u4               flags (0);   
  struct  sockaddr_in  client_addr;
  int_4                addrlen = sizeof(client_addr);
  int_4                NumBytes;


  try {
    //
    // recv()
    // recvfrom() returns;  n > 0:  # bytes read
    //                      n = 0:  no msg/datagram avail (UNBLOCKED);
    //                              we do NOT use UNBLOCKED reads though..
    //                              or peer has done orderly shutdown (TCP only)
    //                      n < 0:  "something wicked this way comes..."
    //                              sets errno; throws exception
    //
        NumBytes = UDPSocketLib::UDP_recvFrom   (private_->socket_fd,
                                                 (unsigned char *)buffer, 
                                                 length, 
                                                 flags, 
                                                 &client_addr, 
                                                 addrlen);
        if (NumBytes > 0) {
          char  clientHost[64];
          int_4 len = sizeof(clientHost);
          UDPSocketLib::UDP_getHostByAddr (clientHost, len, &client_addr);
          string name (clientHost);
          log_.message("incomingData_::[UDP_recvFrom <clientAddr>]  NumBytes= " + 
                       Stringize(NumBytes) + ";  from <" + name + "(" + 
                       inet_ntoa(client_addr.sin_addr) + ")" + ":"+
                       Stringize(ntohs(client_addr.sin_port)) + ">");
        }

  } catch (const SocketLowLevelEx& e) {
      log_.error ("UDPService::[incomingData_]- Read error; "+e.problem());
    }

  //
  // No data...
  //
  if (NumBytes == 0) {
    log_.message("UDPService::[incomingData_]- No Data");
    //closeNetworkPort_();
  }
  //
  // ERROR- UDP socket read error
  //
  else if (NumBytes < 0) {
    log_.error("UDPService::[incomingData_]- UDP socket read error; closing port...");
    closeNetworkPort_();
  }
  
  debugUDPOutput_ ((unsigned char *)buffer, NumBytes);

}					// UDPService::incomingData_ 





void UDPService::debugUDPOutput_ (const unsigned char *buffer, const int_4 NumBytes)
{
  int_4 cnt (0);

  while (cnt < NumBytes) {
    cout<<"          //"; 
    for (Index i = 0; i < 20; i++) {
      if (cnt < NumBytes) {
        cout<<setw(2)<<setfill('0')<<hex<<(int_2)buffer[cnt];
        if (i != 19) cout<<" ";
        cnt++;
      } else {
        cout<<"**";
        if (i != 19) cout<<" ";
      }
    }
    cout<<"//"<<dec<<endl; 
  }     // while (cnt < NumBytes) {
}					// UDPService::debugUDPOutPut_ 




//
// Preconditions: 
//
// Postconditions:
//
// Parameters:
//   const void*  buf      IN;     buffer of data to send
//   const int_4  bytes    IN;     number bytes in buf to send
//
// Description:
//   Send data in buf over network using UDP from Host:Port to
//   destination defined by attributes RemoteHost:RemotePort
//
//


int_4 UDPService::write (const void*        buf, 
                         const int_4        bytes)
{ 
  int_u4               flags (0);   
  int_4                addrlen = sizeof(remoteAddr_);
  int_4                NumBytes = -1;

  if (validRemoteHost_) {
    try {
      //
      // sendTo()   returns;  n > 0:  # bytes read
      //                      n = 0:  no data written; EWOULDBLOCK or EAGAIN
      //                              from NONBLOCK socket setting, maybe..
      //                      n < 0:  "something wicked this way comes..."
      //                              sets errno
      //
      NumBytes = UDPSocketLib::UDP_sendTo (private_->socket_fd,
                                           (unsigned char *)buf, 
                                           bytes, 
                                           flags, 
                                           &remoteAddr_, 
                                           addrlen);

    } catch (const SocketLowLevelEx& e) {
        log_.error ("UDPService::[write]- Failed UDP write to <"+ RemoteIdentifier() + ">");
        log_.error ("UDPService::[write]- " + e.problem());
      }

  } else {
    log_.warning("UDPService::[write]- Invalid RemoteHost; Unable to write to <" + 
                 RemoteIdentifier() + ">");
  }

  return NumBytes;

}					// UDPService::write



//
// Preconditions: 
//
// Postconditions:
//
// Parameters:
//   const void*         buf              IN;  buffer of data to send
//   const int_4         bytes            IN;  number bytes in buf to send
//   struct sockaddr_in* clientAddr,      IN;  destination address   
//   const int_4         clientAddrLength IN;  length of dest address struct
//
// Description:
//   Send data in buf over network using UDP from Host:Port to
//   destination defined by clientAddr
//
//

int_4 UDPService::writeTo (const void*         buf, 
                           const int_4         bytes,
                           struct sockaddr_in* clientAddr, 
                           const int_4         clientAddrLength)
{ 
  int_u4               flags (0);   
  int_4                NumBytes = -1;

    try {
      //
      // sendTo()   returns;  n > 0:  # bytes read
      //                      n = 0:  no data written; EWOULDBLOCK or EAGAIN
      //                              from NONBLOCK socket setting, maybe..
      //                      n < 0:  "something wicked this way comes..."
      //                              sets errno
      //
      NumBytes = UDPSocketLib::UDP_sendTo (private_->socket_fd,
                                           (unsigned char *)buf, 
                                           bytes, 
                                           flags, 
                                           clientAddr, 
                                           clientAddrLength);

    } catch (const SocketLowLevelEx& e) {
        char  clientHost[64];
        int_4 len = sizeof(clientHost);
        UDPSocketLib::UDP_getHostByAddr (clientHost, len, clientAddr);
        string name (clientHost);
        log_.error ("UDPService::[writeTo]- Failed UDP write to <" + 
                    name + " (" +
                    inet_ntoa(clientAddr->sin_addr) + "):"+
                    Stringize(ntohs(clientAddr->sin_port)) + ">");
        log_.error ("UDPService::[writeTo]- " + e.problem());
      }

    return NumBytes;

}					// UDPService::writeTo



//
// Preconditions: 
//
// Postconditions:
//
// Parameters:
//   const void*         buf        IN;  buffer of data to send
//   const int_4         bytes      IN;  number bytes in buf to send
//   const string        destHost   IN;  destination hostname
//   const int_4         destPort   IN;  destination port number
//
// Description:
//   Send data in buf over network using UDP from Host:Port to
//   destination defined parameters, destHost:destPort
//
//

int_4 UDPService::writeTo (const void*         buf, 
                           const int_4         bytes,
                           const string        destHost,
                           const int_4         destPort)
{ 
  int_u4               flags (0);   
  int_4                NumBytes = -1;

    try {
      //
      // sendTo()   returns;  n > 0:  # bytes read
      //                      n = 0:  no data written; EWOULDBLOCK or EAGAIN
      //                              from NONBLOCK socket setting, maybe..
      //                      n < 0:  "something wicked this way comes..."
      //                              sets errno
      //
      NumBytes = UDPSocketLib::UDP_sendTo (private_->socket_fd,
                                           (unsigned char *)buf, 
                                           bytes, 
                                           flags, 
                                           destHost, 
                                           destPort);

    } catch (const SocketLowLevelEx& e) {
        log_.error ("UDPService::[writeTo]- Failed UDP write to <" + 
                    destHost + ":" + Stringize(destPort) + ">");
        log_.error ("UDPService::[writeTo]- " + e.problem());
      }

    return NumBytes;

}					// UDPService::writeTo



//
// Preconditions: 
//
// Postconditions:
//
// Parameters:
//   void*         buf        IN;  buffer of data to send
//   const int_4   bytes      IN;  size of data buffer "buf"
//
// Description:
//   Get data in from UDP network interface SW and return in "buf"
//   array.  Blind read of data received at opened Host:Port.  No
//   sender or "source" info returned.  Commonly used as just a
//   UDP port reader in apps that do not intend to send data, or
//   one that only send datagrams to a "well-known" port defined by
//   RemoteHost:RemotePort.
//
//   returns;  n > 0:  # bytes read
//             n = 0:  no msg avail (e.g. NONBLOCKING socket).
//             n < 0:  "something wicked this way comes..."
//                     sets errno; throws exception
//
//
//

int_4 UDPService::read (void*        buf, 
                        const int_4  bytes)
{ 
  int_u4               flags (0);   
  struct  sockaddr_in  client_addr;
  int_4                addrlen = sizeof(client_addr);
  int_4                NumBytes = -1;

  try {
    //
    // recvfrom() returns;  n > 0:  # bytes read
    //                      n = 0:  no msg avail or peer has done 
    //                              orderly shutdown (TCP only)
    //                      n < 0:  "something wicked this way comes..."
    //                              sets errno
    //
    NumBytes = UDPSocketLib::UDP_recvFrom   (private_->socket_fd,
                                             (unsigned char *)buf, 
                                             bytes, 
                                             flags, 
                                             &client_addr, 
                                             addrlen);

  } catch (const SocketLowLevelEx& e) {
      log_.error ("UDPService::[read]- Read error; "+e.problem());
    }

  // ERROR- UDP socket read error
  if (NumBytes < 0) {
    log_.error("UDPService::[read] - Read error on UDP socket; Close port= <"+Stringize(port_)+"");
    closeNetworkPort_();
    //throw SocketLowLevelEx(identifier(), bytes);
  }


  return NumBytes;

}					// UDPService::read 




//
// Preconditions: 
//
// Postconditions:
//
// Parameters:
//   void*         buf        IN;  buffer of data to send
//   const int_4   bytes      IN;  size of data buffer "buf"
//   const int_u4  flags      IN;  read flags consistent with
//                                 recvfrom() network IF call;
//                                 most commonly used would be MSG_PEEK
//
// Description:
//   Get data in from UDP network interface SW and return in "buf"
//   array.  Blind read of data received at opened Host:Port.  No
//   sender or "source" info returned.  Commonly used as just a
//   UDP port reader in apps that do not intend to send data, or
//   one that only send datagrams to a "well-known" port defined by
//   RemoteHost:RemotePort.
//
//   This is an overloaded variant to allow flag settings.
//
//   returns;  n > 0:  # bytes read
//             n = 0:  no msg avail (e.g. NONBLOCKING socket).
//             n < 0:  "something wicked this way comes..."
//                     sets errno; throws exception
//
//

int_4 UDPService::read (void*        buf, 
                        const int_4  bytes, 
                        const int_u4 flags)
{ 
  struct  sockaddr_in  client_addr;
  int_4                addrlen = sizeof(client_addr);
  int_4                NumBytes = -1;

  try {
    //
    // recvfrom() returns;  n > 0:  # bytes read
    //                      n = 0:  no msg avail or peer has done 
    //                              orderly shutdown (TCP only)
    //                      n < 0:  "something wicked this way comes..."
    //                              sets errno
    //
    NumBytes = UDPSocketLib::UDP_recvFrom   (private_->socket_fd,
                                             (unsigned char *)buf, 
                                             bytes, 
                                             flags, 
                                             &client_addr, 
                                             addrlen);

  } catch (const SocketLowLevelEx& e) {
      log_.error ("UDPService::[read]- Read error; "+e.problem());
    }

  // ERROR- UDP socket read error
  if (NumBytes < 0) {
    log_.error("UDPService::[read] - Read error on UDP socket; Close port= <"+Stringize(port_)+"");
    closeNetworkPort_();
    //throw SocketLowLevelEx(identifier(), bytes);
  }


  return NumBytes;

}					// UDPService::read 



//
// Preconditions: 
//
// Postconditions:
//
// Parameters:
//   void*               buf               IN;  buffer of data to send
//   const int_4         bytes             IN;  size of data buffer "buf"
//   const int_u4        flags             IN;  read flags consistent with
//                                              recvfrom() network IF call;
//                                              most commonly used would be MSG_PEEK
//   struct sockaddr_in* clientAddr        OUT; network source address of sender
//   const int_4         clientAddrLength  OUT; length of source address struct
//
// Description:
//   Get data in from UDP network interface SW and return in "buf"
//   array.  Returns sender address info to application.
//
//   This is an overloaded variant to return network "ready" sockaddr_in
//   structure so application can respond to sender, if desired.
//
//   returns;  n > 0:  # bytes read
//             n = 0:  no msg avail (e.g. NONBLOCKING socket).
//             n < 0:  "something wicked this way comes..."
//                     sets errno; throws exception
//
//

int_4 UDPService::readFrom (void*               buf, 
                            const int_4         bytes,
                            const int_u4        flags,   
                            struct sockaddr_in* clientAddr, 
                            const int_4         clientAddrLength)
{ 
  int_4                NumBytes = -1;

  try {
    //
    // recvfrom() returns;  n > 0:  # bytes read
    //                      n = 0:  no msg avail or peer has done 
    //                              orderly shutdown (TCP only)
    //                      n < 0:  "something wicked this way comes..."
    //                              sets errno
    //
    NumBytes = UDPSocketLib::UDP_recvFrom   (private_->socket_fd,
                                             (unsigned char *)buf, 
                                             bytes, 
                                             flags, 
                                             clientAddr, 
                                             clientAddrLength);

  } catch (const SocketLowLevelEx& e) {
      log_.error ("UDPService::[readFrom]- Read error; "+e.problem());
    }

  // ERROR- UDP socket read error
  if (NumBytes < 0) {
    log_.error("UDPService::[readFrom] - Read error on UDP socket; Close port= <"+Stringize(port_)+"");
    closeNetworkPort_();
    //throw SocketLowLevelEx(identifier(), bytes);
  }


  return NumBytes;

}					// UDPService::readFrom



//
// Preconditions: 
//
// Postconditions:
//
// Parameters:
//   void*               buf               IN;  buffer of data to send
//   const int_4         bytes             IN;  size of data buffer "buf"
//   const int_u4        flags             IN;  read flags consistent with
//                                              recvfrom() network IF call;
//                                              most commonly used would be MSG_PEEK
//   string              &srcHost          OUT; Hostname of sender
//   int_4               &srcPort          OUT; Port number of sender
//
// Description:
//   Get data in from UDP network interface SW and return in "buf"
//   array.  Returns sender hostname and port info to application.
//
//   This is an overloaded variant to return srcHost:srcPort so 
//   application can respond to sender, if desired.
//
//   returns;  n > 0:  # bytes read
//             n = 0:  no msg avail (e.g. NONBLOCKING socket).
//             n < 0:  "something wicked this way comes..."
//                     sets errno; throws exception
//
//

int_4 UDPService::readFrom (void*               buf, 
                            const int_4         bytes,
                            const int_u4        flags,   
                            string              &srcHost,
                            int_4               &srcPort)
{ 
  int_4                NumBytes = -1;

  try {
    //
    // recvfrom() returns;  n > 0:  # bytes read
    //                      n = 0:  no msg avail or peer has done 
    //                              orderly shutdown (TCP only)
    //                      n < 0:  "something wicked this way comes..."
    //                              sets errno
    //
    NumBytes = UDPSocketLib::UDP_recvFrom   (private_->socket_fd,
                                             (unsigned char *)buf, 
                                             bytes, 
                                             flags, 
                                             srcHost, 
                                             srcPort);

  } catch (const SocketLowLevelEx& e) {
      log_.error ("UDPService::[readFrom]- Read error; "+e.problem());
    }

  // ERROR- UDP socket read error
  if (NumBytes < 0) {
    log_.error("UDPService::[readFrom] - Read error on UDP socket; Close port= <"+Stringize(port_)+"");
    closeNetworkPort_();
    //throw SocketLowLevelEx(identifier(), bytes);
  }


  return NumBytes;

}					// UDPService::readFrom












