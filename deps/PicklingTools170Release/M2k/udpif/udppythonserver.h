#ifndef UDPPYTHONSERVER_H_

//
//
// Author: C Pendergraph
//         largely derived from XMSocketMsg by Steve Capie
//         UDP version of that code; wrote my own XMMSG
//         (XMXlateMsg) because i had to support conversions
//         both ways

// ///////////////////////////////////////////// Include Files

#include "m2convertrep.h"
#include "m2wakeontimer.h"
#include "m2recattr.h"
#include "udpservice.h"
#include "geoopalmsgnethdr.h" // Needed to distinguish different serializations

// ///////////////////////////////////////////// UDPPythonServer

// ///////////////////////////////////////////// ClientInfoShort Class

class ClientInfoShort {
  public:
    string              ClientID;
    string              Host;
    int_4               Port;
    struct sockaddr_in  Addr;
    int_4               AddrLen;


    // Constructors

    ClientInfoShort () {ClientID           = "";
                        Host               = "";
                        Port               = 0;
                        AddrLen            = 0;
    }

    ClientInfoShort (const string ID) {ClientID = ID;}

    // ///// Operators

    int operator == (const ClientInfoShort& other) const {
      return ClientID == other.ClientID; }

    int operator == (const string ID) const {
      return ClientID == ID; }

  protected:
  private:
};




// ///////////////////////////////////////////// UDPPythonServer Class

M2RTTI(UDPPythonServer)

class UDPPythonServer : public UDPService {

    M2PARENT(UDPService)

  public:

    // ///// Midas 2k Attributes

    RTEnum<GeoOpalMsg_Serialization_e>    Serialization; 

    RTReadout<string>                     Clients;               // OUT

    RTReadout<int_u2>                     NumberClients;         // OUT

    RTReadout<int_u2>                     MsgsSent;              // OUT

    ReceiverAttribute                     InMsg;                 // guess    

    RTParameter<OpalTable>                UDPClientTable;        // IN

    RTParameter<int_u4>	                  DebugMask;             // IN

    // ///// Methods

    // Constructor

    UDPPythonServer ();


  protected:

    // ///// Methods

    // ///// Extend UDPService & m2::Service

    virtual void startMyself_ ();
    virtual void startup_ ();
    virtual void stopMyself_ ();
    virtual bool takeWakeUpCall_ (WakeCondition& waker);


    // ///// Satisfy UDPService

    virtual void incomingData_ ();


    // ///// Data Members

  private:

    // ///// Data Members

                                    // list of clients w/ net addresses
    Array<ClientInfoShort>   clientArray_;
                                    // number of clients
    int_u4                   msgsSent_;

    struct sockaddr_in       thisHostAddr_;
    int_u4                   debugMask_;


    // ///// Methods

    // Method called when my InMsg is changed.  Attempts to convert
    // the in-coming OpalTable to an binary stream message, then attempts to
    // write this message to our socket.  An exception can be thrown
    // if either attempt fails.

    void sendMessageUDP_            (const OpalTable& MsgOT);

    void loadClientTable_           ();
};					// UDPPythonServer



#define UDPPYTHONSERVER_H_
#endif // UDPPYTHONSERVER_H_
