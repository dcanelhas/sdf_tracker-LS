#ifndef M2OPALPYTHONSOCKETMSG_H_

//
//

/*

  Author:		DTJ

  Last Reviewed:	9/29/1998

  Purpose:  Full duplex OpalTables over the network.

 */

// ///////////////////////////////////////////// Include Files

#include "m2dualsocketpeer.h"
#include "m2opalmsgextnethdr.h"


// ///////////////////////////////////////////// The OpalPythonSocketMsg Class

M2RTTI(OpalPythonSocketMsg)
class OpalPythonSocketMsg : public DualSocketPeer {

  M2PARENT(DualSocketPeer)

  public:

    // ///// Midas 2k Attributes

    RTEnum< OpalMsgExt_Serialization_e > Serialization;
    
    
    RTParameter<OpalTable>	InMsg;
    RTReadout<OpalTable>	OutMsg;

  
    PushBtnAttr                Connect;
    PushBtnAttr                Disconnect;


    // ///// Methods

    // Constructor.

    OpalPythonSocketMsg ();


  protected:

    // ///// Methods

    // Send the given message out over the output connection.

    void sendMessage_ (const OpalTable& tbl);


    // Receive the given message from the server.  This is called
    // after an OpalTable has been deserialized over the reading
    // end of the connection.  Default action is to post to OutMsg.

    virtual void getMessage_ (const OpalTable& tbl);


    // ///// Satisfy DualSocketClient

    virtual void incomingData_ ();
    virtual void onNewConnection_ ();
    virtual void beforeDisconnect_ ();


  private:

    // ///// Data Members

    // Whether or not we suspect our peer has closed (message length
    // not read successfully).

    bool	peerClosed_;

    // Only one thread can write to our InMsg at a time, lest we risk
    // two threads writing different messages to the same socket
    // simultaneously.

    M2Mutex     inMsgLock_;


    // Listens to InMsg.

    AttributeListener<OpalPythonSocketMsg>	listen_;


    // Listens to Connect and Disconnect

    AttributeListener<OpalPythonSocketMsg>	listenToConnect_;


    // ///// Methods

    // Called whenever a new InMsg is received.

    void newInMsg_ (const ChangesMessage& msg);


    // Called whenever Connect or Disconnect is pressed

    void changeConnectState_ (const ChangesMessage& msg);


};					// OpalPythonSocketMsg



#define M2OPALPYTHONSOCKETMSG_H_
#endif // M2OPALPYTHONSOCKETMSG_H_


