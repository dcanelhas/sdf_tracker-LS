#ifndef M2OPALPYTHONDAEMON_H_

//
//

/*

Author:		DTJ, RTS

Last Revised:	8/18/97, ... 2010

*/

// ///////////////////////////////////////////// Include Files

#include "m2service.h"
#include "m2svrattr.h"
#include "m2recattr.h"
#include "m2socketserver.h"
#include "m2table.h"
#include "m2wakeonsocket.h"
#include "m2opalmsgextnethdr.h"
#include "m2mutex.h"
#include "m2flyset.h"

// ///////////////////////////////////////////// Type Definitions

enum OpalPythonDaemon_Client_e {
  OpalPythonDaemon_SINGLE,
  OpalPythonDaemon_DUAL
};
M2ENUM_UTILS_DECLS(OpalPythonDaemon_Client_e)

enum OpalPythonDaemon_AddressType_e {
  OpalPythonDaemon_ByFileDesc,
  OpalPythonDaemon_UpByOne
};
M2ENUM_UTILS_DECLS(OpalPythonDaemon_AddressType_e)

enum OpalPythonDaemon_Role_e {
  // Creates a server at a port which must be free.  If port is
  // unavailable, PortFailed will be posted to.
  OpalPythonDaemon_SERVER_AT_FIXED_PORT,

  // Creates a server at a port chosen by the system.  The actual port
  // chosen will be posted to the "Port" Attribute.
  OpalPythonDaemon_SERVER_ANYWHERE,

  // Creates a server within some "zone" defined from the value of the
  // Port Attribute up to (Port + PortZoneAttempts) ports above it.
  // The actual port chosen will be posted to Port.  If no ports in
  // the "zone" are available, PortFailed will be posted to.
  OpalPythonDaemon_SERVER_IN_PORT_ZONE
};
M2ENUM_UTILS_DECLS(OpalPythonDaemon_Role_e)


// ///////////////////////////////////////////// The OpalPythonDaemon Class

M2RTTI(OpalPythonDaemon)
class OpalPythonDaemon : public Service {

  M2PARENT(Service)

  public:

    // ///// Midas 2k Attributes

    RTEnum<OpalPythonDaemon_Role_e>	      Role;
    RTBool                            SingleSocket;


    RTReadout<bool>                   PortFailed;

      
    RTEnum<OpalMsgExt_Serialization_e>   Serialization;


    RTEnum<OpalMsgExt_Serialization_e>   AdaptiveDefaultSerialization;


    RTParameter<int_u4>	              Port;


    RTEnum<OpalPythonDaemon_Client_e>  ClientType;


    RTEnum<OpalPythonDaemon_AddressType_e>  AddressType;


    RTReadout<bool>                   Advertised;


    RTReadout<int_4>                  Clients;


    RTParameter<OpalTable>            InMsg;
    

    RTParameter<OpalTable>            InMsgBypass;


    RTReadout<OpalTable>              OutMsg;


    RTParameter<int_u4>	              InMsgQueueSize;


    RTReadout<Vector>                 ClientVector;

    
    RTReadout<OpalTable>              ClientAction;


    RTReadout<real_8>                 CurrentQueued;

    
    RTParameter<real_8>               ClientReadDataTimeLimit;


    RTParameter<real_8>               ClientWriteDataTimeLimit;


    RTParameter<int_4>                DisconnectClient;


    RTBool                            QuerySocketBeforeWrite;


    RTParameter<int_u4>               DisconnectQueueSize;


    RTParameter<int_4>                PortZoneAttempts;



    // ///// Methods

    // Constructor

    OpalPythonDaemon ();

    
    // Destructor

    ~OpalPythonDaemon ();


    // Disconnects the given client, if it is connected.  Closes the
    // sockets associated with it.

    void disconnectClient (int_4 desc);


  protected:

    // ///// Midas 2k Attributes

    // This Attribute is protected because its meant only to be called
    // internally from OpalPythonDaemon (or any children).
    
    RTParameter<int_4>                InternalDisconnectClient_;


    // ///// Methods

    // ///// Extend Service

    // Advertises the service at the given Port.

    virtual void preamble_ ();


    // Make sure the wake conditions are properly disabled even if we
    // didn't rundown cleanly.

    virtual void stopMyself_ ();


    // Processes incoming messages from the clients.

    virtual bool takeWakeUpCall_ (WakeCondition& waker);


    // Closes all sockets and the service.

    virtual void rundown_ ();


    // Process a message such as may be dequeued from InMsg.

    void processMessage_ (const OpalTable& ov);


    // Same as above, but message gets sent to those on client_list only.
    
    void processMessage_ (const OpalTable& ov, const FlySet& client_list);


  private:

    // ///// Type Definitions

    enum OpalPythonDaemon_Action_e {
      OpalPythonDaemon_Action_CONNECTION,
      OpalPythonDaemon_Action_DISCONNECTION,
      OpalPythonDaemon_Action_USER_DISCONNECT,
      OpalPythonDaemon_Action_RUNDOWN,
      OpalPythonDaemon_Action_POSTATTRIBUTES,
      OpalPythonDaemon_ACTION_E
    };
    

    // ///// Data Members

    // Where clients originally connect -- the entity advertising the port.

    SocketServer serviceEntrance_;


    // Listens for new clients coming in.

    WakeOnSocket	newClient_;


    // Listens to the existing clients.

    WakeOnSocket	clientActivity_;


    // The WakeOnAttribute listening for changes in the Port attribute.
    
    WakeOnAttribute	portListener_;


    // Guards the collection consisting of bstreamTable_, mutexTable_,
    // refCountTable_, and canUseTable_.  Every thread wishing to
    // write to a client MUST first acquire this lock.

    M2Mutex                   collectionMutex_;

    // A mapping from descriptor to stream, as well as the collection
    // of the BStreams.  Has deletion responsibility.

    IntHashTable<BStream*>    bstreamTable_;


    // Mapping of write_descriptors to mutex.  The mutexes protect
    // write access to the socket descriptors.

    IntHashTable<M2Mutex*>    mutexTable_;


    // Reference count of how many threads have declared their
    // intention to write to a given socket descriptor.

    IntHashTable<Index>       refCountTable_;


    // Flag per connection to indicate health of socket.

    IntHashTable<bool>        canUseTable_;


    // For "guess" streams, remember the type of serialization
    // the client used so we talk back to him in the same
    // kind of serialization .. look up serialization by client_id

    IntHashTable<OpalMsgExt_Serialization_e>  serializations_;


    // Gets set when the daemon has been told to quit.

    bool                      inRundown_;

    // Listens to our InMsgQueueSize attribute so that an external
    // thread has the ability to "unstick" the queue by making it
    // larger.

    AttributeListener<OpalPythonDaemon>	listenQSize_;


    // Listens to InMsgBypass and allows external threads in to write
    // to their sockets.

    AttributeListener<OpalPythonDaemon>       listenToBypass_;

    // Internally, everything is stored by file descriptors.
    // Externally, for RETURN ADDRESS type fields, it's either
    // by file descriptor (which can cause problems if the file 
    // gets closed and the file descriptor is reused) or by
    // up by one (which may roll over. BUT much less likely to get reused)
    // We have two tables to convert between them.
    IntHashTable<int_4>  fdToUpByOne_;   // convert from FD to UpByOne addr
    IntHashTable<int_4>  upByOneTofd_;   // convert from UpByOne # to fd


    // The up by one counter, we start it at 1 because 0 and -1 can be special

    int_4 upByOneCounter_; 


    // ///// Methods


    // Called when InMsgBypass changes.

    void bypassChange_ (const ChangesMessage& msg);


    // Called when InMsgQueueSize changes.

    void queueSizeChange_ (const ChangesMessage& msg);


    // Completes the rundown process after any pending threads
    // complete their write operations.  If rundown_ is called on this
    // Service, we cannot immediately rundown because there may be
    // pending writers waiting for access to certain descriptors.

    void finalRundown_ ();


    // Perform cleanup after writeToClient_ finishes.
    //
    // Preconditions: writeToClient_ has completed with or without an
    // error and the write desc specific mutex has been unlocked.
    //
    // Postconditions: This client's reference count has been
    // decremented.  If an error occured during the write, then its
    // canUseTable_ flag has been set to false.  If its canUseTable_
    // flag is false and this client's reference count is zero then
    // a disconnect request is made via InternalDisconnectClient_.

    void clientWriteExit_ (const int_4 client_id, 
			   const bool error);


    // Write a message to a client.
    //
    // Preconditions: 1) client_id contains the write descriptor of
    // the client who will recieve this message. 2) hdr contains an
    // OpalMsgNetHdr serialized correctly and prepareOpalMsg has been
    // called on it. 3) to_send contains an OMemStream in which this
    // message has been prepared. 4) droppable_msg is true if this
    // message contained a "DROPPABLE" = true key/value pair.
    //
    // Postconditions: clientWriteExit_ is called.

    bool writeToClient_ (const int_4 client_id,
			 OpalMsgExtNetHdr& hdr,
			 OMemStream& to_send,
			 const bool droppable_msg);

    
    // Returns true if the message should be dropped due to a socket
    // descriptor that is not ready to recieve data.  The amount of
    // room in the kernel's socket buffer for this method to return
    // true is most likely 2k.
    //
    // Preconditions: fd is a valid, open socket descriptor.
    //
    // Postconditions: false is returned if the socket is not "ready".

    bool descriptorWritable_ (const int_4 fd);
    

    // Service the client at the given descriptor.  Involves reading
    // a serialized OpalTable from the socket, putting it in an envelope,
    // and sending it through OutMsg.

    void serviceClient_ (int_4 desc);


    // Called when the user requests a new service by changing the
    // Port attribute.

    void newService_ ();


    // Executed only by the Service's thread.  Disconnects a client
    // and reclaims his BStream and M2Mutex pointers.

    void disconnectClient_ (int_4 desc, bool user_requested = false);
    

    // Called when a connect, disconnect or during rundown_ as a
    // helper to post to ClientAction and ClientVector
    
    void postAction_ (OpalPythonDaemon_Action_e action, int_4 desc);
    
};					// OpalPythonDaemon



#define M2OPALPYTHONDAEMON_H_
#endif // M2OPALPYTHONDAEMON_H_


