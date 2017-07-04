//

// ///////////////////////////////////////////// Include Files
#include <iostream.h>
#include <fstream.h>
//#include <strstream.h>
#include <iomanip.h>

#include "udppythonserver.h"
#include "udpifhelp.h"
#include "m2sockete.h"
#include "m2getenv.h"


// ///////////////////////////////////////////// UDPPythonServer Methods

UDPPythonServer::UDPPythonServer () :
  M2ATTR_VAL(Serialization, GeoOpalMsg_BINARY),  // M2k binary by default
  M2ATTR     (InMsg),
  M2ATTR_VAL (MsgsSent,                  0),
  M2ATTR_VAL (Clients,                   ""),
  M2ATTR_VAL (NumberClients,             0),
  M2ATTR_VAL (UDPClientTable, OpalTable()),
  M2ATTR_VAL (DebugMask,                 0x0000),
  msgsSent_(0),
  debugMask_(0)
{
//  InMsg.queueSize(500);
  // Are we running out of queue space?
  InMsg.queueSize(10000);

  addWakeUp_(InMsg,             "InMsgRCVD");
  addWakeUp_(DebugMask,         "DebugMaskChange");
}



// Extend from Service-Primitive-Component

void UDPPythonServer::startMyself_ () 
{

  UDPService::startMyself_();
}                                          // UDPPythonServer::startMyself_




// Extend from Service-Primitive-Component

void UDPPythonServer::startup_ () 
{
  UDPService::startup_();

  clientArray_.clear();
  loadClientTable_();

  debugMask_  = DebugMask.value();

}                                          // UDPPythonServer::startup_




void UDPPythonServer::incomingData_ ()
{ 

  UDPSocketLib::UDP_unblockSocket (UDPSocketLib::descriptorForServer(server_()));

  UDPdatagram UDPdataIN;

  struct sockaddr_in  inAddr;
  int_4               addrlen = sizeof(inAddr);
  int_u4              flags (0);   

  int hbytes = UDPService::readFrom ((unsigned char *)UDPdataIN.buffer, 
                                     MAX_UDP_DATA,
                                     flags,
                                     &inAddr,
                                     addrlen);


  if (hbytes <= 0)
    return;

                                 // find out who this is...
  int_u2   port     = ntohs( inAddr.sin_port );
  string   addrSTR  = inet_ntoa( inAddr.sin_addr );
  string   senderID = addrSTR + ":" + Stringize( port );

  //
  // Debug-
  //
  if (debugMask_ & UDPDBG_NETIN) {
      log_.message("UDP RECV: NumBytes= <"+Stringize(hbytes)+
                   ">;  UNK Message; >  from " + senderID);
    //
    // prints raw UDP msg out in hex (cout)
    //
    if (debugMask_ & UDPDBG_DETAIL) 
      UDPService::debugUDPOutput_((unsigned char *)UDPdataIN.buffer, hbytes);
  }


  log_.warning ("UNK Msg rcvd on UDP server <" + UDPService::Identifier() +
                ">;  MsgID= <"+Stringize(senderID)+">");


  return;

}					// UDPPythonServer::incomingData_ 



void UDPPythonServer::sendMessageUDP_ (const OpalTable& MsgOT)
{

  if ( (clientArray_.length() > 0) && UDPService::IsOpen() ) {
    
    //
    // Binary serialize OpalTable message
    //
    OMemStream UDPdataOUT;



    GeoOpalMsg_Serialization_e ser = Serialization.value();
    GeoOpalMsgNetHdr msg(ser);
    msg.prepareOpalMsg(UDPdataOUT, MsgOT);
    unsigned char* serialized_data = (unsigned char*)UDPdataOUT.data();
    int_4 sendBytes = UDPdataOUT.length();
    /////    if (ser=="M2k") {
    /////
    /////  UDPdataOUT << MsgOT;
    /////  //stringTypeUse-
    /////  //string str = MsgOT.serialize();
    /////  //UDPdataOUT.write(str.c_str(), str.length());
    /////  sendBytes = UDPdataOUT.length();
    /////  serialized_data = (unsigned char*)UDPdataOUT.data();
    /////} else if (ser=="P2") {
    /////  int bytes = P2TopLevelBytesToDumpOpal(MsgOT);
    /////  a.expandTo(bytes);
    /////  char* start_mem = (char*)a.data();
    /////  char* end_mem = P2TopLevelDumpOpal(MsgOT, start_mem);
    /////  int actual_bytes = end_mem - start_mem;
    /////  sendBytes = actual_bytes;
    /////  serialized_data = (unsigned char*)a.data();
    /////}

    OpalValue ov;
    string id   = (MsgOT.tryGet("NAME", ov)) ? ov.stringValue() : string("no_value");
    if (id == "no_value")
      string id   = (MsgOT.tryGet("COMMAND", ov)) ? ov.stringValue() : string("no_value");


    for (Index Idx = 0; Idx < clientArray_.length(); Idx++) {
      //
      // Debug-
      //
      if (debugMask_ & UDPDBG_NETOUT) {
        log_.message ("UDP SEND: NumBytes= <" + Stringize(sendBytes) + ">;  MsgID= <" + 
          id + ">  to   " + clientArray_[Idx].ClientID +
	  " [" + clientArray_[Idx].Host + ":" + Stringize(clientArray_[Idx].Port) + "]");

        //
        // prints raw UDP msg out in hex (cout)
        //
        if (debugMask_ & UDPDBG_DETAIL) 
          //UDPService::debugUDPOutput_((unsigned char *)str.c_str(), str.length());
          //UDPService::debugUDPOutput_((unsigned char *)UDPdataOUT.data(), sendBytes);
	  UDPService::debugUDPOutput_(serialized_data, sendBytes);
      }

      
      //
      // Send OpalTable as datagram message across UDP socket.
      //
      try {
          //int_4 NumBytes = UDPService::writeTo ((unsigned char *)UDPdataOUT.data(), 
          //                                      sendBytes, 
          //                                      clientArray_[Idx].Host, 
          //                                      clientArray_[Idx].Port);
          int_4 NumBytes = UDPService::writeTo (serialized_data, 
                                                sendBytes, 
                                                &clientArray_[Idx].Addr, 
                                                clientArray_[Idx].AddrLen);
      }     // try
      catch (const BStreamException& e) {
        log_.noteException(e);
        log_.error("Error sending X-Midas message across socket!");
        return;
      }

        //
        //Debug- convert back and print...
        //
        //IMemStream imem ((char *)UDPdataOUT.data(), sendBytes);
        //OpalTable tmp;
        //imem >> tmp;
        ////stringTypeUse-
        ////tmp.deserialize((void *)UDPdataOUT.data());
        ////tmp.deserialize(str);
        //tmp.prettyPrint(cout);

    }     // for (Index Idx = 0; Idx < clientArray_.length(); Idx++) {

  }     // if ( (clientArray_.length() > 0) && UDPService::IsOpen() ) {

  msgsSent_++;
  MsgsSent.post(msgsSent_);
  
}					// UDPPythonServer::sendMessageUDP_()




bool UDPPythonServer::takeWakeUpCall_ (WakeCondition& waker)
{
  string reason = waker.groupID();


  acknowledge_();

  if (reason == "InMsgRCVD") {
    Size entries = mainAlarm_.pendingEvents();
    if (entries > 500) {
     cerr << "pendingentries built up over 500 entries!!!" << endl;
    }
    for (Index i = 0; i < entries; i++) {
      if (DequeueOpalValue(*this, InMsg, true))
	sendMessageUDP_(InMsg.value());  	
    }
    return true;
  }

  else if (reason == "PortChange") {

    log_.message ("UDPPythonServer::Changed well-known-port for Server...");

                             //
                             // UDPService has stuff todo too..
    UDPService::takeWakeUpCall_(waker);
    return true;
  }

  else if (reason == "DebugMaskChange") {
    ostrstream eout;
    eout<<"UDPPythonServer::Changed attribute; DebugMask= <0x";
    eout<<setw(4)<<setfill('0')<<hex<<DebugMask.value()<<">"<<ends;
    //string     txt_string = eout.str();
    //delete[] eout.str();
    string txt_string = OstrstreamToString(eout);
    log_.message (txt_string);
    debugMask_ = DebugMask.value();
    return true;
  }


  else
    return UDPService::takeWakeUpCall_(waker);

}					// UDPPythonServer::takeWakeUpCall_ 




void UDPPythonServer::stopMyself_ ()
{

  UDPService::stopMyself_();

}					// UDPPythonServer::stopMyself_




void UDPPythonServer::loadClientTable_ () 
{
  OpalTable client_table = UDPClientTable.value();
  if (client_table.length() == 0) { 
    log_.error("No client table given to udpserver.");
  }

  OpalValue         ov;
  OpalTableIterator clientIterator(client_table);
  // Loop thru client_table entries...
  while (clientIterator.next()) {
    
    OpalTable clientOT = UnOpalize(clientIterator.value(),OpalTable);
    string id = clientOT.tryGet("ID", ov) ? 
      ov.stringValue() : string("no_value");
    string host = clientOT.tryGet("Host", ov) ? 
      ov.stringValue() : UDPService::ThisHostName();
    int_4 port = clientOT.tryGet("Port", ov) ? int_4(ov.number()) : 0 ;
    
    if (port != 0) {
      if (!clientArray_.contains(id)) {
	char                hname[1024];
	struct sockaddr_in  destAddr;
	int_4 stat = UDPSocketLib::UDP_findHostByName (host.c_str(), 
						       hname,
						       sizeof(hname),
						       &destAddr);
	if (stat == 0) { 
	  clientArray_.append(id);
	  Index Idx = clientArray_.index(id);
	  clientArray_[Idx].Port    = port;
	  clientArray_[Idx].Host    = host;
	  destAddr.sin_port = htons((u_short) clientArray_[Idx].Port);
	  clientArray_[Idx].Addr    = destAddr;
	  clientArray_[Idx].AddrLen = sizeof(clientArray_[Idx].Addr);
	} else { 
	  log_.error("UDP_findHostByName failed for host: " + host + ". "
		     "Ignoring.");
	}
      }
    }
  }
  NumberClients.post (clientArray_.length());
  
  // Post Client List to Attribute
  string ClientList;
  for (Index i=0; i < clientArray_.length(); i++) {
    if (i != 0)
      ClientList = ClientList + " " + clientArray_[i].ClientID;
    else {
      ClientList = clientArray_[i].ClientID;
      UDPService::SetRemoteHost (clientArray_[i].Host);
      UDPService::SetRemotePort (clientArray_[i].Port);
    }
  }
  Clients.post(ClientList);
} // UDPPythonServer::loadClientTable_
