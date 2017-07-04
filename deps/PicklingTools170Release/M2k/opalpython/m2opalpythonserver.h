#ifndef M2OPALPYTHONSERVER_H_

//


// ///////////////////////////////////////////// Include Files

#include "m2componentcrew.h"
#include "m2time.h"
#include "m2opalpythondaemon.h"


// ///////////////////////////////////////////// OpalPythonServer

M2RTTI(OpalPythonServer)
  
class OpalPythonServer : public ComponentCrew {
  
  M2PARENT(ComponentCrew)

    public :

    // ///// Midas 2k Attributes
  
    // Mocked OpalDaemon Attributes.

    RTParameter<OpalTable>            InMsg;

    RTParameter<OpalTable>            InMsgBypass;

    RTReadout<OpalTable>              OutMsg;

    RTReadout<Vector>                 ClientVector;

    RTReadout<OpalTable>              ClientAction;

    RTParameter<string>               DisconnectClient;
    

    // Attributes unique to OpalPythonServer.

    RTBool                                  UniqueClientIds;

    RTBool                                  AllowExternalThreadWrites;
    
    RTReadout<OpalTable>                    UniqueIdToClientIdMapping;

    RTCallbackParameter<string, OpalPythonServer> Uptime;



    // ///// Methods

    // Constructor.

    OpalPythonServer ();

    
    // This method is here only to satisify OpalDaemon's interface.
    // Remember, you should be able to "drop in" OpalPythonServer anywhere
    // OpalDaemon is used as a replacement.

    void disconnectClient (int_4 desc)
    {
      disconnectClient_(Stringize(desc));
    }


  protected:

    // ///// Methods

    // ///// Extend ComponentCrew
    
    virtual void useMyComponents_ ();

    virtual void assembleMyself_ ();

    virtual void startMyself_ ();


    // OpalPythonServer calls the following methods depending on the type of
    // event which occurs. These methods are implemented as empty
    // virtuals in this class.  If you are deriving from this class,
    // you can override these methods and fill them in with your
    // desired behavior.
    //
    // Its important to know that these methods are animated by
    // OpalDaemon's thread.  Thus as little work as possible should be
    // performed in these methods.  If you need to perform a lot of
    // work or call functions which could possibly block, you should
    // use another thread and hand that work off.  By not returning
    // immediately from these methods, you will create a "Denial of
    // Service" for any other clients of this server.


    // A new client (client_id) has connected to us.

    virtual void handleClientConnection_ (const string&) { }


    // A currently connected client (client_id) is sending a request
    // to us (cli_request);

    virtual void handleIncomingClientMessage_ (const string&, const OpalTable&) { }

    
    // A currently connected client (client_id) is disonnecting from
    // us.  If we were told to disconnect it then bool is true.
    
    virtual void handleClientDisconnection_ (const string&, const bool) { }


    // Our network interface is running down.  Hopefully because we
    // told it to.

    virtual void handleDaemonRundown_ () { }



    // Called by our children when they want to disconnect a client
    // (client_id) from our network interface.  This causes
    // handleClientDisconnection_ above to set "user_requested" to
    // true.
    
    bool disconnectClient_ (const string& client_id);


    // Called by our children when they want to send a message (msg)
    // to a currently connected client (client_id).  If the message is
    // considered droppable then that should be set to true.

    bool writeToClient_ (const string& client_id, 
			 const OpalTable& msg,
			 const bool droppable = false);


    // Called by our children when they want to send a message (msg)
    // to all our connected clients.  "droppable" should be set to
    // true if this message is not precious.

    void broadcastToClients_ (const OpalTable& msg,
			      const bool droppable = false);


  private:

    // ///// Data Members

    // The OpalPythonDaemon which provides the network connectivity to
    // clients.
  
    OpalPythonDaemon		daemon_;
      
    // Start time of this server.

    M2Time                      startTime_;

    // Structures used to maintain unique client ids.

    Size                        nextUniqueId_;

    M2Mutex                     mapMutex_;

    OpalTable                   uniqueClientMap_;

    // Listens to the daemon's OutMsg so that we can react to client
    // requests.
    
    AttributeListener<OpalPythonServer>     listenToDaemon_;

    AttributeListener<OpalPythonServer>     listenToAttrs_; 


    // ///// Methods

    // Called by AttributeListeners
    
    void activityFromDaemon_ (const ChangesMessage& msg);

    void attributeChanged_ (const ChangesMessage& msg);


    // Called by RTCallbackParameter

    string getUpTime_ () const;


    // Helper methods.

    void handleClientMessage_ (const OpalTable& cli_msg);
    
    void handleClientAction_ (const OpalTable& cli_act);

    void handleClientVector_ (const Vector& vec);

    string createUniqueId_ (const string& client_id);

    string getUniqueID_ (const string& client_id);

    string getClientID_ (const string& unique_id);

    string removeClientID_ (const string& client_id);


  public:
  
    // ///// Midas 2k Attributes

    // From OpalDaemon

    RTEnum<OpalPythonDaemon_Role_e>&	Role;
    RTBool&                             SingleSocket;
    
    RTReadout<bool>&                    PortFailed;

    RTEnum<OpalMsgExt_Serialization_e>&	Serialization;
    RTEnum<OpalMsgExt_Serialization_e>& AdaptiveDefaultSerialization;

    RTParameter<int_u4>&		Port;

    RTParameter<OpalPythonDaemon_Client_e>&   ClientType;

    RTReadout<bool>&                    Advertised;

    RTReadout<int_4>&			Clients;

    RTParameter<int_u4>&                InMsgQueueSize;

    RTBool&                             QuerySocketBeforeWrite;

    RTParameter<int_u4>&                DisconnectQueueSize;
        
    RTParameter<real_8>&                ClientReadDataTimeLimit;
    
    RTParameter<real_8>&                ClientWriteDataTimeLimit;

    RTParameter<int_4>&                 PortZoneAttempts;

};					// OpalPythonServer

#define M2OPALPYTHONSERVER_H_
#endif // M2OPALPYTHONSERVER_H_

