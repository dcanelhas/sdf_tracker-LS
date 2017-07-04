#ifndef UDPPYTHONCLIENT_H_

//
//
// Author: C Pendergraph
//         derived from XMSocketMsg by Steve Capie
//         UDP version of that code
//         Added serialization options: RTS
//
//
//  M2k version of X-Midas UDP_CLIENT [P-GEO & XINT].  Completely
//  compatible w/ UDP_SERVER.  Designed to communicate with XINTERIM
//  server and receive /CHANASGN/ msgs from a remote CCServer.
//
// ///////////////////////////////////////////// Include Files

// M2K includes:
#include "m2convertrep.h"
#include "m2wakeontimer.h"
#include "m2recattr.h"
#include "m2svrattr.h"


// Unit includes:
#include "udpservice.h"
#include "geoopalmsgnethdr.h"  // Needed for different serializations!

// ///////////////////////////////////////////// Constants

const real_8   UDPServerTimeoutInterval  (60.0);


// ///////////////////////////////////////////// Enumerated Types

enum UDPServer_Status_e {
  UDPServer_UP,
  UDPServer_DOWN,
  UDPServer_LISTEN
};

M2ENUM_UTILS_DECLS (UDPServer_Status_e)


// ///////////////////////////////////////////// Constants
// None


// ///////////////////////////////////////////// Class UDPPythonClient

M2RTTI(UDPPythonClient)

class UDPPythonClient : public UDPService {

    M2PARENT(UDPService)

  public:

    // ///// Midas 2k Attributes

    RTEnum<GeoOpalMsg_Serialization_e>    Serialization;         // In/OUT

    RTReadout<int_u2>                     MsgsRcvd;              // OUT

    RTReadout<OpalTable>                  GUIColour;             // OUT

    ServerAttribute                       OutMsg;                // guess
    
    RTParameter<int_u4>	                  DebugMask;             // IN

    RTReadout<UDPServer_Status_e>	  Status;                // OUT

    // ///// Methods

    // Constructor

    UDPPythonClient ();


  protected:

    // ///// Methods

    // ///// Extend UDPService & Service

    virtual void startMyself_ ();
    virtual void startup_ ();
    virtual void stopMyself_ ();
    virtual bool takeWakeUpCall_ (WakeCondition& waker);


    // ///// Satisfy UDPService

    virtual void incomingData_ ();


    // ///// Data Members
   WakeOnTimer    serverTimeoutTimer_;

  private:

    // ///// Data Members

    struct sockaddr_in      thisHostAddr_;
    int_u4                  debugMask_;
    int_u4                  msgsRcvd_;
    M2Time                  lastMsgTime_;
    UDPServer_Status_e      status_;

    // ///// Methods


};					// UDPPythonClient



// ///////////////////////////////////////////// Inline Methods


#define UDPPYTHONCLIENT_H_
#endif  // UDPPYTHONCLIENT_H_
