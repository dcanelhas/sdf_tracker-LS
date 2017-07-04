
// Copyright 1998 Rincon Research Corporation

// ///////////////////////////////////////////// Include Files

#include "m2sockete.h"
#include "udppythonclient.h"
#include "udpifhelp.h"


// ///////////////////////////////////////////// Alphabet Stuff

M2ALPHABET_BEGIN_MAP (UDPServer_Status_e)

  AlphabetMap   ("UP",       UDPServer_UP);
  AlphabetMap   ("DOWN",     UDPServer_DOWN);
  AlphabetMap   ("Idle",     UDPServer_LISTEN);

M2ALPHABET_END_MAP (UDPServer_Status_e)

M2ENUM_UTILS_DEFS  (UDPServer_Status_e)



// ///////////////////////////////////////////// UDPPythonClient Methods

UDPPythonClient::UDPPythonClient () :
  M2ATTR_VAL(Serialization, GeoOpalMsg_BINARY),  // M2k binary be default

  M2ATTR     (OutMsg),
  M2ATTR_VAL (MsgsRcvd,                  0),
  M2ATTR_VAL (DebugMask,                 0x0000),
  M2ATTR_VAL (Status,                    UDPServer_LISTEN),
  M2ATTR_VAL (GUIColour,  UnStringize("{BG=\"orange\",FG=\"black\"}", OpalTable)),
  serverTimeoutTimer_ ("checkServers",   false),
  msgsRcvd_(0),
  debugMask_(0),
  status_(UDPServer_LISTEN)
{
  addWakeUp_(DebugMask,         "DebugMaskChange");
  addWakeUp_(serverTimeoutTimer_);

  serverTimeoutTimer_.disable();
  serverTimeoutTimer_.interval(UDPServerTimeoutInterval);

}



// Extend from Service-Primitive-Component

void UDPPythonClient::startMyself_ () 
{
  //DEPRECATED NOTE/CMNT..BUT GOOD TO REMEMBER
  // MUST go here...DONT take WakeUp interrupts
  // before XMXlateMsg is inited above..UDP Server
  // could already be bdxting to your port w/o
  // the login protocol from a prev failed script
  //
  UDPService::startMyself_();

}                                          // UDPPythonClient::startMyself_




// Extend from Service-Primitive-Component

void UDPPythonClient::startup_ () 
{
  UDPService::startup_();

  serverTimeoutTimer_.enable();

  debugMask_  = DebugMask.value();

}                                          // UDPPythonClient::startup_




//
// Preconditions: 
//
// Postconditions: 
//
// Parameters:
//   None
//
// Extended function from UDPService-
//
// Process incoming UDP datagram info.  Indicates activity on
// UDP socket. If data available, UDP protocal header has
// already been stripped (by UDP suite)...this is pure 
// app-level stuff here.
//
// Called from UDPService::takeWakeUpCall(), an Interrupt
// Service Routine (ISR) that is called when the Service 
// thread awakes from one of the "addWakeUp_()" conditions.
// In this case the waking condition is socket activity
// detected by a WakeOnSocket (socketNotice_) in UDPService.
// In this extention of the UDPService::incomingData(), we
// read in the UDP data and do somethng with it.
//

void UDPPythonClient::incomingData_ ()
{ 
  UDPdatagram UDPdataIN;

  struct sockaddr_in  inAddr;
  int_4               addrlen = sizeof(inAddr);

  UDPSocketLib::UDP_unblockSocket (UDPSocketLib::descriptorForServer(server_()));

  int hbytes = UDPService::readFrom ((unsigned char *)UDPdataIN.buffer, 
                                     MAX_UDP_DATA,
                                     0,
                                     &inAddr,
                                     addrlen);
  if (hbytes <= 0)
    return;
                                 //
                                 // create unique ID from inet addr 
                                 //
  int_u2   port     = ntohs( inAddr.sin_port );
  string   addrSTR  = inet_ntoa( inAddr.sin_addr );
  string   senderID = addrSTR + ":" + Stringize( port );


  //
  // Debug-
  //
  if (debugMask_ & UDPDBG_NETIN) {
      log_.message("UDP RECV: NumBytes= <"+Stringize(hbytes)+
                   ">;   from: " + senderID);
    //
    // prints raw UDP msg out in hex (cout)
    //
    if (debugMask_ & UDPDBG_DETAIL) 
      UDPService::debugUDPOutput_((unsigned char *)UDPdataIN.buffer, hbytes);
  }

  //
  // CLIENT-
  //
    M2Time CurrTime;
    lastMsgTime_ = CurrTime;

    // Interpret buffer
    GeoOpalMsg_Serialization_e ser = Serialization.value();
    GeoOpalMsgNetHdr msg(ser);
    StreamDataEncoding sde; 
    OpalTable MsgOT = msg.interpretBuffer(hbytes, (char*)UDPdataIN.buffer, sde);

    ///////////////RTS Left over, trying geoopalmsg, since it does everything for us
    //if (serialize_=="M2k") {
    //  IMemStream imem ((char *)UDPdataIN.buffer, hbytes);
    //  imem >> MsgOT;
    //} else if (serialize_=="P2") {
    //  OpalValue ov;
    //  P2TopLevelLoadOpal(ov, (char*)UDPdataIN.buffer);
    //  MsgOT = ov;
    //}

    ////stringTypeUse-
    ////tmp.deserialize((void *)UDPdataOUT.data());
    ////tmp.deserialize(str);
    //MsgOT.prettyPrint(cout);

    OutMsg.post(MsgOT);

    msgsRcvd_++;
    MsgsRcvd.post(msgsRcvd_);  

    if (status_ != UDPServer_UP) {
      GUIColour.post(UnStringize("{BG=\"green\",FG=\"black\"}", OpalTable));
      log_.warning ("UDP server(s) UP;  Message rcvd from: " + senderID);
      status_ = UDPServer_UP;
      Status.post(UDPServer_UP);
    }

  return;


}					// UDPPythonClient::incomingData_ 



//
// Preconditions: 
//
// Postconditions: 
//
// Parameters:
//   None
//
// Extended function from UDPService-Service-
// An Interrupt Service Routine (ISR) that "wakes" Service
// when one of the "addWakeUp_() conditions occurs and
// needs attention.  These conditions all have a string label
// for identification.  External M2k OpalTable messages
// attributes (like ReceiverAttribute) should have queues
// associated so nothing gets overlooked or postponed.
//

bool UDPPythonClient::takeWakeUpCall_ (WakeCondition& waker)
{
  string reason = waker.groupID();

  if (reason == "DebugMaskChange") {
    acknowledge_();
    ostrstream eout;
    eout<<"UDPPythonClient:: Changed attribute; DebugMask = <0x";
    eout<<setw(4)<<setfill('0')<<hex<<DebugMask.value()<<">"<<ends;
    //string     txt_string = eout.str();
    //delete[] eout.str();
    string txt_string = OstrstreamToString(eout);
    log_.message (txt_string);
    debugMask_ = DebugMask.value();
    return true;
  }

  else if (reason == "checkServers") {
    acknowledge_();
    M2Time     currtime;
    M2Duration diff = currtime - lastMsgTime_;
    if (diff > M2Duration(ServerTimeoutInterval)) {
      Status.post(UDPServer_DOWN);
      if (status_ != UDPServer_DOWN)
        log_.warning ("UDP server(s) DOWN;  Last message rcvd: " + Stringize(lastMsgTime_));
      status_ = UDPServer_DOWN;// Need this to be able to return to "UP"
      GUIColour.post(UnStringize("{BG=\"red\",FG=\"black\"}", OpalTable));

    }
    return true;
  }

  else if (reason == "PortChange") {
    Status.post(UDPServer_DOWN);
    status_ = UDPServer_DOWN; // Need this to be able to return to "UP"
    GUIColour.post(UnStringize("{BG=\"orange\",FG=\"black\"}", OpalTable));
                             //
                             // UDPService has stuff todo too..
    UDPService::takeWakeUpCall_(waker);
    //acknowledge_();
    return true;
  }

  else
    return UDPService::takeWakeUpCall_(waker);

}					// UDPPythonClient::takeWakeUpCall_ 



//
// Preconditions: 
//
// Postconditions: 
//
// Parameters:
//   None
//
// Extended function from UDPService-Service-
// Excecuted everytime component is stopped.  We want to do
// app-level disconnect cleanly each time.  Stops server from
// writing unnecessarily into the ether.
//

void UDPPythonClient::stopMyself_ ()
{

  UDPService::stopMyself_();

}					// UDPPythonClient::stopMyself_




