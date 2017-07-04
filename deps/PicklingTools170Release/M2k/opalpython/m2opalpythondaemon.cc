//
//

// TODO:  How do we determine when a client is leaving, both nicely
// and catastrophically?  1) unable to write  2) read zero (unlikely
// since we're using select() 3) they send a quit message to the daemon,
// who removes them by calling some nice "remove client" message on this
// component.  Use an Attribute?
//
// TODO:  Lots of overhead in this class; use Cheshire Cat?
//
// TODO (ab 6-MAR-00): Use ThreadSafeContainerT from hot/mrs to manage
// access to the BStreams.


// ///////////////////////////////////////////// Include Files

#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include "m2opalpythondaemon.h"
#include "m2bstreame.h"
#include "m2sockete.h"
#include "m2openvector.h"

#include <sys/socket.h>

// ///////////////////////////////////////////// Enumerated Types

M2ALPHABET_BEGIN_MAP(OpalPythonDaemon_Client_e)
  AlphabetMap("Dual",	OpalPythonDaemon_DUAL);
  AlphabetMap("Single",	OpalPythonDaemon_SINGLE);
M2ALPHABET_END_MAP(OpalPythonDaemon_Client_e)

M2ENUM_UTILS_DEFS(OpalPythonDaemon_Client_e)


M2ALPHABET_BEGIN_MAP(OpalPythonDaemon_AddressType_e)
  AlphabetMap("ByFileDesc",	OpalPythonDaemon_ByFileDesc);
  AlphabetMap("UpByOne",	OpalPythonDaemon_UpByOne);
M2ALPHABET_END_MAP(OpalPythonDaemon_AddressType_e)

M2ENUM_UTILS_DEFS(OpalPythonDaemon_AddressType_e)


M2ALPHABET_BEGIN_MAP(OpalPythonDaemon_Role_e)
  AlphabetMap("Fixed", OpalPythonDaemon_SERVER_AT_FIXED_PORT);
  AcceptableMap("Server At Fixed Port", OpalPythonDaemon_SERVER_AT_FIXED_PORT);

  AlphabetMap("Anywhere", OpalPythonDaemon_SERVER_ANYWHERE);
  AcceptableMap("Server Anywhere", OpalPythonDaemon_SERVER_ANYWHERE);

  AlphabetMap("Zone", OpalPythonDaemon_SERVER_IN_PORT_ZONE);

M2ALPHABET_END_MAP(OpalPythonDaemon_Role_e)

M2ENUM_UTILS_DEFS(OpalPythonDaemon_Role_e)


// ///////////////////////////////////////////// OpalPythonDaemon Methods

OpalPythonDaemon::OpalPythonDaemon () :
  M2ATTR_VAL(Role, OpalPythonDaemon_SERVER_AT_FIXED_PORT),
  M2ATTR_VAL(SingleSocket, false),
  M2ATTR_VAL(PortFailed, false),
  M2ATTR_VAL(Serialization, OpalMsgExt_ADAPTIVE),
  M2ATTR_VAL(AdaptiveDefaultSerialization, OpalMsgExt_BINARY),
  M2ATTR_VAL(Port, 8501),
  M2ATTR_VAL(ClientType, OpalPythonDaemon_DUAL),
  M2ATTR_VAL(AddressType, OpalPythonDaemon_UpByOne),
  M2ATTR_VAL(Advertised, false),
  M2ATTR_VAL(Clients, 0),
  M2ATTR_VAL(InMsg, OpalTable()),
  M2ATTR_VAL(InMsgBypass, OpalTable()),
  M2ATTR_VAL(OutMsg, OpalTable()),
  M2ATTR_VAL(InMsgQueueSize, 4),
  M2ATTR_VAL(CurrentQueued, 0),
  M2ATTR_VAL(ClientVector, Vector(NumericType(int_4), 0)),
  M2ATTR_VAL(ClientAction, OpalTable()),
  M2ATTR_VAL(ClientReadDataTimeLimit, -1.0),
  M2ATTR_VAL(ClientWriteDataTimeLimit, -1.0),
  M2ATTR_VAL(DisconnectClient, 0),
  M2ATTR_VAL(QuerySocketBeforeWrite, false),
  M2ATTR_VAL(DisconnectQueueSize, 500),
  M2ATTR_VAL(PortZoneAttempts, 5000),
  M2ATTR_VAL(InternalDisconnectClient_, 0),
  portListener_(Port),
  inRundown_(false),
  listenQSize_(this, &OpalPythonDaemon::queueSizeChange_),
  listenToBypass_(this, &OpalPythonDaemon::bypassChange_),
  upByOneCounter_(0x7ffffffd)
{
  // Since we're using socket select to tell us that a connection attempt
  // is present, if it fails it's because the other side suddenly went
  // away, or is using the wrong protocol.  Give them five seconds to
  // complete the connection before we drop them.  NB: If the listen
  // backlog is too small, the second part of a DualSocketConnection may
  // not be accepted in time when the accept time limit is two seconds.
  serviceEntrance_.acceptTimeLimit(5.0);

  // Set the queue size here so that addWakeUp will treat
  // it as a queued attribute.
  InMsg.queueSize(InMsgQueueSize.value());

  // 500 has been chosen arbitrarily.  The important thing to know is
  // that the queueSize must be greater than 0, else its possible for
  // this Service's thread to block while trying to enqueue.  Bad news
  // if InMsg is being used and not InMsgBypass.
  InternalDisconnectClient_.queueSize(DisconnectQueueSize.value());

  addWakeUp_(portListener_, "service parameter");

  addWakeUp_(newClient_, "new client");
  addWakeUp_(clientActivity_, "client activity");

  addWakeUp_(InMsg, "input");

  addWakeUp_(DisconnectClient, "disconnect client");

  addWakeUp_(InternalDisconnectClient_, "internal disconnect");

  listenQSize_.listenToChanges(InMsgQueueSize);
  listenQSize_.listenToChanges(DisconnectQueueSize);
  listenToBypass_.listenToChanges(InMsgBypass);
}



OpalPythonDaemon::~OpalPythonDaemon ()
{ 
  preDestruct_();
}



void OpalPythonDaemon::queueSizeChange_ (const ChangesMessage& msg)
{
  int_u4 new_size = msg.value().number();
  
  if (msg.mostRecent(InMsgQueueSize)) {
    InMsg.queueSize(new_size);
  } else if (msg.mostRecent(DisconnectQueueSize)) {
    InternalDisconnectClient_.queueSize(new_size);
  }
}



void OpalPythonDaemon::bypassChange_ (const ChangesMessage& msg)
{
  processMessage_(msg.value());
}



void OpalPythonDaemon::disconnectClient (int_4 desc)
{
  DisconnectClient = desc;
}



void OpalPythonDaemon::preamble_ ()
{
  Service::preamble_();
  postAction_(OpalPythonDaemon_Action_POSTATTRIBUTES, 0);
  newService_();
}



bool OpalPythonDaemon::takeWakeUpCall_ (WakeCondition& waker)
{
  string reason = waker.groupID();
  
  if (log_.isReporting(M2DEBUG_FINE)) 
    log_.debug(M2DEBUG_FINE, "OpalPythonDaemon::takeWakeUpCall_: reason = " +
	       reason);
  
  if (reason == "service parameter") {
    newService_();
  } else if (reason == "new client") {

    BStream* stream = 0;
    
    OpalPythonDaemon_Client_e client_type(ClientType.value());
    
    try {
      switch (client_type) {
      case OpalPythonDaemon_SINGLE:
	stream = serviceEntrance_.acceptSingleClient();
	break;
      case OpalPythonDaemon_DUAL:
	// throw MidasException("Only SINGLE socket is currently supported");
	stream = serviceEntrance_.acceptDualClient(SingleSocket.value());
	break;
      default:
	throw M2UNHANDLEDCASE_EX(client_type);
	M2BOGUS_BREAK;
      }
    } catch (const BStreamException& e) {
      // Exceptions may be thrown if the client goes away before we
      // try to listen, or if there's a timeout on the accept call and
      // a type-SINGLE client connects to a type-DUAL daemon (the
      // required second connection is never made).
      log_.warning("Error accepting client type " + Stringize(client_type) + ": " + e.problem());
      // NB: To test the behavior of this component on unhandled exceptions,
      // re-throw the exception instead of returning here, and use telnet
      // or some other mechanism to sucker the daemon into trying to
      // accept a bogus connection.
      // throw e;
      return true;
    }

    // Try high-bandwidth
    stream->transferWindow(50000);

    // Set up client data time limits
    stream->readDataTimeLimit(ClientReadDataTimeLimit.value());
    stream->writeDataTimeLimit(ClientWriteDataTimeLimit.value());

    int_4 rdesc = stream->readDescriptor();
    M2Mutex* cmuxtex = new M2Mutex();

    {
      // Add the read-descriptor to our BStream lookup table, create a
      // mutex, counter, and boolean to synchronize access to
      // this connection.
      ProtectScope lock(collectionMutex_);
      bstreamTable_[rdesc]  = stream;
      serializations_[rdesc] = OpalMsgExt_ADAPTIVE;
      mutexTable_[rdesc]    = cmuxtex;
      refCountTable_[rdesc] = 0;
      canUseTable_[rdesc]   = true;
 
      // Be able to map fd to up_by_one counter and vice-versa
      int_4 up_by_one_count = upByOneCounter_++;
      if (up_by_one_count < 0) {
	// wrap back around positives if it ever becomes negative
	up_by_one_count = 1; 
	upByOneCounter_ = 1;
      }
      fdToUpByOne_[rdesc] = up_by_one_count;
      upByOneTofd_[up_by_one_count] = rdesc;

    }

    // Tell the WakeOnSocket monitor to listen for this client.
    clientActivity_.addDescriptor(rdesc);

    // Update the readouts of daemon state.
    postAction_(OpalPythonDaemon_Action_CONNECTION, rdesc);

    log_.info("Accepted new client");
    
  } else if (reason == "client activity") {

    // Grab the complete list of active clients, and for each one,
    // read and ship the appropriate OpalTables.  Grab it ONCE, not
    // multiple times, lest it change out from underneath you.

    FlySet adesc(clientActivity_.activeDescriptors());
    for (Index i = 0; i < adesc.length(); i++) {
      serviceClient_(adesc[i]);
    }

  }					// client activity
  else if (reason == "input") {
    Size entries = mainAlarm_.pendingEvents();
    if (log_.isReporting(M2DEBUG_MEDIUM))
	log_.debug(M2DEBUG_MEDIUM, "queue entries = " + Stringize(entries));
    for (Index i = 0; i < entries; i++) {
      if (DequeueOpalValue(*this, InMsg, true)) {
	CurrentQueued.post(entries-i-1);
	processMessage_(InMsg.value());
      }
    }
  }

  else if (reason == "disconnect client") {
    // This is an external disconnect.
    disconnectClient_(DisconnectClient.value(), true);
  } else if (reason == "internal disconnect") {
    Size entries = mainAlarm_.pendingEvents();
    for (Index ii = 0; ii < entries; ii++) {
      if (DequeueOpalValue(*this, InternalDisconnectClient_, true)) {
	disconnectClient_(InternalDisconnectClient_.value());
      }
    }
  }

  else {
    log_.warning("WakeCondition reason \"" + reason + "\" not recognized");
  }
  return true;
}					// OpalPythonDaemon::takeWakeUpCall_



void OpalPythonDaemon::stopMyself_ ()
{
  // In case we didn't hit rundown (e.g., blew out of the PPL due to an
  // uncaught exception, not that that's a good thing), disable all the
  // wake conditions.  If we don't do this, the server socket is still
  // open, and the listen backlog will permit clients to think they've
  // successfully connected.  Note, however, that we do not close existing
  // clients, since if we restart we'll be able to continue processing.
  newClient_.disable();
  clientActivity_.disable();
  serviceEntrance_.close();

  // Go through and close all sockets.  This should force any threads
  // blocked in socket reads or writes to pop out.  This is necessary
  // if we want this instance to go away anytime soon.  Otherwise the
  // thread requesting our stoppage could block forever waiting for
  // some thread blocked on a socket to return.
  {
    ProtectScope lock(collectionMutex_);

    IntHashTableIterator<BStream *> next(bstreamTable_);
    while (next()) {
      const int_4 client_id = next.key();

      // By setting the canUseTable_ flag, no other clients will be
      // allowed to write to the socket.  Since rundown_() gets called
      // after stopMyself_(), the cleaning up of the bstreams and the
      // mutexes will be done there.
      canUseTable_[client_id] = false;

      // NOTE (ab 21-JUN-00): Ideally, I would like to call
      // "next.value()->close()" on each BStream, but DEC UNIX seems
      // to have a problem close()'ing the sockets. I think it may be
      // due to the SO_LINGER socket option.  According to Stevens,
      // page 187: "By default, close returns immediately, but if
      // there is any data still remaining in the socket send buffer,
      // the system will try to deliver the data to the peer."
      // 
      // Regardless of the reason, shutdown(2) seems to do the trick.
      M2LOG_DEBUG(M2DEBUG_COARSE, "Closing open socket: " + Stringize(client_id));
      shutdown(next.value()->writeDescriptor(), SHUT_RDWR);
      shutdown(next.value()->readDescriptor(), SHUT_RDWR);
    }
  }

  Service::stopMyself_();
}



void OpalPythonDaemon::rundown_ ()
{
  log_.debug(M2DEBUG_MEDIUM, "Running down daemon");
  
  // Stop listening for changes
  newClient_.disable();
  clientActivity_.disable();

  serviceEntrance_.close();
  Advertised.post(false);

  // Broadcast to all clients that its time to Disconnect.
  OpalTable  tbl;
  tbl.put("COMMAND", "DISCONNECT");
  processMessage_(tbl);

  // Do all the closing/disabling here; I'm afraid of deleting within
  // the iterator.

  // Sweep all the old clients out of the wakeOnSocket
  clientActivity_.clearDescriptors();      

  bool rundown_service = false;
  
  // This is very tricky.  We need to delete all BStreams and
  // M2Mutexs, but there may still be threads using these structures.
    
  Array<int_u4> disconnect_list;

  { 
    ProtectScope lock(collectionMutex_);
    
    inRundown_ = true;

    if (bstreamTable_.entries() == 0) {
      // No connections are being used, safe to rundown now.
      finalRundown_();
      rundown_service = true;
    } else {
      IntHashTable<bool> itertbl = canUseTable_;

      // Go through and mark all clients as unuseable.  Check the ref
      // counts for 0, we can call disconnectClient_() on those right
      // now.  If the ref count > 0, then the writers who are still
      // around should notify us when they are done.
      IntHashTableIterator<bool> next(itertbl);
      while (next()) {
	canUseTable_[next.key()] = false;
      }

      IntHashTableIterator<Index> next_ref(refCountTable_);
      while (next_ref()) {
	if (next_ref.value() == 0) {
	  disconnect_list.append(next_ref.key());
	}
      }
    }
  } // end-ProtectScope

  // Go through and disconnect all the descriptors with a 0 ref count.
  for (Index ii = 0; ii < disconnect_list.length(); ii++) {
    disconnectClient_(disconnect_list[ii]);
  }

  if (rundown_service) {
    // Update the readouts that describe the daemon state
    postAction_(OpalPythonDaemon_Action_RUNDOWN, 0);
    Service::rundown_();
  }
}



void OpalPythonDaemon::finalRundown_ ()
{
  // collectionMutex_ should be locked when this is called.
  
  // Sanity check.  Check our data structures.
  
  IntHashTableIterator<Index> next_idx(refCountTable_);
  while (next_idx()) {
    m2assert(next_idx.value() == 0,
	     "Running down with client " + Stringize(next_idx.key()) +
	     "'s count at " + Stringize(next_idx.value()));
  }
  
  IntHashTableIterator<bool> next_bool(canUseTable_);
  while (next_bool()) {
    m2assert(next_bool.value() == false, 
	     "Client " + Stringize(next_bool.key()) + "'s flag is not set");
  }
  Index mutex_cnt = mutexTable_.entries();

  m2assert(mutex_cnt == 0, "Runningdown with a mutex_cnt of " + Stringize(mutex_cnt));

  // Clear out our data structures.
  refCountTable_.clear();
  canUseTable_.clear();
  mutexTable_.clear();
}



void OpalPythonDaemon::clientWriteExit_ (const int_4 client_id, 
				   const bool error)
{  
  bool disconnect_client = false;

  {
    ProtectScope lock(collectionMutex_);
    Index ref_cnt = --(refCountTable_[client_id]);
    
    if (error) {
      // If there was an error during writing, start the clean up
      // process by marking this client as unuseable.
      canUseTable_[client_id] = false;
      
      if (ref_cnt == 0) {
	// If there is no one else waiting to obtain the write_lock for
	// this client, tell OpalPythonDaemon's main thread to clean it up.
	disconnect_client = true;
      }
    } else {
      // No error during write, but check the canUseTable_.  If someone
      // set this flag while we were writing, then this client is in the
      // process of being shut down.  If we are the last one out the
      // door, its then time to wake up OpalPythonDaemon's main thread to
      // complete the clean up.
      if (!canUseTable_[client_id] && ref_cnt == 0) {
	disconnect_client = true;
      }
    }
  }

  if (disconnect_client) {
    M2LOG_DEBUG(M2DEBUG_COARSE, "Requesting disconnection of client: " + Stringize(client_id));
    InternalDisconnectClient_.value(client_id);
  }
}



bool OpalPythonDaemon::writeToClient_ (const int_4 client_id, 
				       OpalMsgExtNetHdr& hdr,
				       OMemStream& to_send,
				       const bool droppable_msg)
{
  BStream* single_client = 0;
  M2Mutex* write_lock    = 0;
  bool     unlocked      = false;
  int_4    wdesc         = 0;

  // Get vital components needed to write.
  {
    ProtectScope lock(collectionMutex_);
    
    if (!canUseTable_.contains(client_id)) {
      // Client does not exist.
      log_.warning("Attempt to write to non-existent client: " + Stringize(client_id));
      return false;
    }
    if (!canUseTable_[client_id]) {
      // Client is about to be reclaimed.
      M2LOG_DEBUG(M2DEBUG_COARSE, "Write denied, client is being closed: " + Stringize(client_id));
      return false;
    }

    single_client = bstreamTable_[client_id];
    refCountTable_[client_id]++;      
    write_lock = mutexTable_[client_id];
  }

  wdesc = single_client->writeDescriptor();
  
  // Try to lock the mutex, if its already locked check for a
  // droppable message.
  
  if (!write_lock->tryLock()) {
    if (droppable_msg) {
      M2LOG_DEBUG(M2DEBUG_COARSE, "Dropping DROPPABLE message due to congested socket descriptor: "+ Stringize(wdesc));
      clientWriteExit_(client_id, false);
      return false;
    }
    write_lock->lock();
  }

  // At this point, we should be "guaranteed" that the write_lock has
  // been obtained.

  
  // If the QuerySocketBeforeWrite Attribute is set to true, use a
  // (expensive) select() call to check if the write descriptor is
  // ready to receive this message.  If its not ready and the message
  // is droppable, then return immediately.
  if (droppable_msg && QuerySocketBeforeWrite.value()) {
    // TODO (ab 28-JUL-99): maybe a version of this method that can
    // determing if *multiple* descriptors are able to be written rather
    // than a single one at a time.
    if (! SocketLowLevel::readyForIO(wdesc, "w")) {
      write_lock->unlock();
      clientWriteExit_(client_id, false);
      return false;
    }
  }
  
  if (log_.isReporting(M2DEBUG_MEDIUM)) 
    log_.debug(M2DEBUG_MEDIUM, "Sending envelope to single client " + Stringize(client_id));

  bool write_success = true;
  try {
    hdr.sendPreparedMsg(*single_client, to_send);
  } catch (const BStreamException& e) {
    M2LOG_DEBUG(M2INFO, "Incomplete write: Connection closed during write");
    M2LOG_DEBUG(M2DEBUG_COARSE, e.problem());
    write_success = false;
  } catch (const MidasException& e) {
    log_.error("MidasException caught during delivery of a single client message to write_desc: " + Stringize(wdesc));
    log_.noteException(e);
    write_success = false;
  } catch (...) {
    // I don't think that we should see a non-MidasException, but just
    // in case.
    log_.error("Non-Midas exception caught during OpalPythonDaemon::processMessage_() to a single client. "
	       "write_desc: " + Stringize(wdesc) + " client_id: " + Stringize(client_id));
    write_success = false;
  }

  write_lock->unlock();
  clientWriteExit_(client_id, !write_success);
  return write_success;
}



/*
void OpalPythonDaemon::processMessage_ (const OpalTable& in_tbl)
{
  OpalTable tbl(in_tbl);

  int_4	   client_id     = -1;
  const bool droppable_msg =  (tbl.contains("DROPPABLE") ?
			       UnOpalize(tbl.get("DROPPABLE"), bool) :
			       false);

  // In a thread-safe manner, check if our client is still around, get
  // the bstream and increment its refrence count.
  
  if (tbl.contains("ADDRESS")  &&  tbl.contains("CONTENTS")) {
    client_id = tbl.get("ADDRESS").number();
    tbl = tbl.get("CONTENTS");
   }

  // Regardless of the client, the message must be serialized.
  // Do serialization once so that with multiple clients, we do
  // not have to pay for it again and again.

  OMemStream to_send;
  OpalMsgExtNetHdr hdr(Serialization.value());
  hdr.prepareOpalMsg(to_send, tbl);
  
  if (client_id != -1) {
    // Single client.
    (void)writeToClient_(client_id, hdr, to_send, droppable_msg);
  } else {
    // Broadcast to all clients.
    M2LOG_DEBUG(M2DEBUG_MEDIUM, "Broadcasting message to " + Stringize(bstreamTable_.entries()) + " clients");
    
    // Make a copy for safe iteration.
    IntHashTable<BStream*> iter_table;
    { 
      ProtectScope lock(collectionMutex_);
      iter_table = bstreamTable_;
    }

    IntHashTableIterator<BStream*> next(iter_table);
    while (next()) {
      (void)writeToClient_(next.key(), hdr, to_send, droppable_msg);
    }
    
  }
}					// OpalPythonDaemon::processMessage
*/


void OpalPythonDaemon::processMessage_ (const OpalTable& in_tbl)
{
  OpalTable tbl(in_tbl);

  // Figure out client, and see if in in wrapper
  int_4	   client_id     = -1;
  if (tbl.contains("ADDRESS")  &&  tbl.contains("CONTENTS")) {
    // Convert addr to appropriate file descriptor
    int_4 addr = tbl.get("ADDRESS").number();
    OpalPythonDaemon_AddressType_e addr_type = AddressType.value();
    if (addr_type == OpalPythonDaemon_UpByOne) {
      int_4 fd = upByOneTofd_[addr];
      client_id = fd;
    } else if (addr_type == OpalPythonDaemon_ByFileDesc) {
      client_id = addr;
    } else {
      log_.error("Unknown Address Type:" + Stringize(addr_type));
      throw MidasException("Unknown Address Type:" + Stringize(addr_type));
    }
    // Contents
    tbl = tbl.get("CONTENTS");
   }

  // Broadcast or single?
  if (client_id != -1) {
    // Single client
    FlySet single_client;
    single_client.insertFly(client_id);
    processMessage_(tbl, single_client);

  } else {
    // Broadcast to all clients.
    M2LOG_DEBUG(M2DEBUG_MEDIUM, "Broadcasting message to " + Stringize(bstreamTable_.entries()) + " clients");

    // Build a list of all client_ids, but still have to watch threads
    FlySet all_clients(bstreamTable_.entries());
    {
      ProtectScope ps(collectionMutex_);

      IntHashTableIterator<BStream*> it(bstreamTable_);
      while (it()) {
	all_clients.insertFly(it.key());
      }
    }
    processMessage_(tbl, all_clients);
  }
}					// OpalPythonDaemon::processMessage




/*
void OpalPythonDaemon::processMessage_ (const OpalTable& in_tbl, const FlySet& client_list)
{
  const OpalTable tbl = ((in_tbl.contains("ADDRESS") && in_tbl.contains("CONTENTS")) ?
			 OpalTable(in_tbl.get("CONTENTS")) :
			 in_tbl);
  
  const bool droppable_msg = (tbl.contains("DROPPABLE") ?
			      UnOpalize(tbl.get("DROPPABLE"), bool) :
			      false);
  
   // There is a lot of repeated code in here and processMessage_
   // above. Some time later on, figure out the best way to
   // consolidate.
   OMemStream to_send;
   OpalMsgExtNetHdr hdr(Serialization.value());
   hdr.prepareOpalMsg(to_send, tbl);
   
   // Make a copy for safe iteration.
   IntHashTable<BStream*> iter_table;
   { 
     ProtectScope lock(collectionMutex_);
     iter_table = bstreamTable_;
   }
   
   IntHashTableIterator<BStream*> next(iter_table);
   while (next()) {
     const int client_id = next.key();
     if (client_list.containsFly(client_id)) {
       (void)writeToClient_(client_id, hdr, to_send, droppable_msg);
     }
   }
}
*/



void OpalPythonDaemon::processMessage_ (const OpalTable& in_tbl, 
					const FlySet& client_list)
{
  OpalMsgExt_Serialization_e base_serialization = Serialization.value();

  const OpalTable tbl = ((in_tbl.contains("ADDRESS") && in_tbl.contains("CONTENTS")) ?
			 OpalTable(in_tbl.get("CONTENTS")) :
			 in_tbl);
  
  const bool droppable_msg = (tbl.contains("DROPPABLE") ?
			      UnOpalize(tbl.get("DROPPABLE"), bool) :
			      false);
  
  // Make copies for safe iteration.
  IntHashTable<BStream*> iter_table;
  IntHashTable<OpalMsgExt_Serialization_e> serializations;
  { 
    ProtectScope lock(collectionMutex_);
    iter_table     = bstreamTable_;
    serializations = serializations_;
  }
  
  // Set-up tables so that once we have serialized using a particular format,
  // it is saved for reuse (to avoid serializaion again)
  const int len = OpalMsgExt_LENGTH;
  Array<OMemStream*> to_sends(len);
  Array<OpalMsgExtNetHdr> to_hdrs(len);
  for (int ii=0; ii<len; ii++) {
    to_sends.append(0);  // no op= on OMemStream, so have to clean up manual
    to_hdrs.append(OpalMsgExtNetHdr(OpalMsgExt_BINARY)); 
    // Why binary?  because that's what the default of the OpalSocketMsg is ..
  }
  
  IntHashTableIterator<BStream*> next(iter_table);
  while (next()) {
    
    int client_id = next.key();
    if (!client_list.containsFly(client_id)) continue; // no client/no serial!

    // Are we doing forced serialization of guessing per client?
    OpalMsgExt_Serialization_e per_client_serialization = base_serialization; 
    if (base_serialization == OpalMsgExt_ADAPTIVE) { 
      per_client_serialization = serializations[client_id];
      if (per_client_serialization == OpalMsgExt_ADAPTIVE) {
	// Still adaptive?  Have to use AdaptiveDefaultSerialization
	OpalMsgExt_Serialization_e adaptive_default = 
	  AdaptiveDefaultSerialization.value();
	if (adaptive_default==OpalMsgExt_ADAPTIVE) 
	  adaptive_default = OpalMsgExt_BINARY;
	{
	  ProtectScope lock(collectionMutex_);
	  serializations_[client_id] = adaptive_default;
	}
	per_client_serialization=serializations[client_id]=adaptive_default;
	log_.debug(M2DEBUG_COARSE,
"If your client is appears to be getting junk, it's possible you need to set\n"
"the AdapativeDefaultSerialization attribute on the OpalPythonDaemon to \n"
"something else.  The problem:\n"
"Unknown how to talk to the Client (as we are talking to the\n"
"client before it talked to us), so we use **"+Stringize(adaptive_default)+"**\n"
" (the value on Attribute AdaptiveDefaultSerialization or binary if weird)\n"
"To get around this problem, you have two choices: \n"
"(1) send a empty message to the OpalPythonDaemon to initiate the \n"
"    conversation (then the server will know how to talk to the client)\n"
"(2) set the AdaptiveDefaultSerialization to something else\n");
      }
    }
    
    // Serialize for the type of serialization request, if not already done
    int ser_type = int(per_client_serialization);
    if (!to_sends[ser_type]) {
      to_sends[ser_type] = new OMemStream;
      to_hdrs[ser_type]  = OpalMsgExtNetHdr(per_client_serialization);
      to_hdrs[ser_type].prepareOpalMsg(*to_sends[ser_type], tbl);
    }
    
    (void)writeToClient_(client_id, to_hdrs[ser_type], *to_sends[ser_type],
			 droppable_msg);
  }
  
  // Clean up OMemStreams
  for (int jj=0; jj<len; jj++) {
    delete to_sends[jj];
  }
  
}



void OpalPythonDaemon::serviceClient_ (int_4 client_id)
{
  BStream* client = 0;
  bool     cleanup = false;

  {
    ProtectScope lock(collectionMutex_);
    
    if (!canUseTable_.contains(client_id) || !canUseTable_[client_id]) {
      // This client is somehow slated for removal (perhaps a writing
      // thread encountered an error), extract this message (if
      // possible) and then shut him down.
      cleanup = true;
    }

    if (!bstreamTable_.contains(client_id)) {
      M2LOG_DEBUG(M2DEBUG_COARSE, "Ignoring stale message from disconnected client: " + Stringize(client_id));
      return;
    } else {
      client = bstreamTable_[client_id];
    }
  }

  M2LOG_DEBUG(M2DEBUG_MEDIUM, "Servicing client " + Stringize(client_id));

  try {
    OpalMsgExt_Serialization_e base_serialization = Serialization.value();
    OpalMsgExtNetHdr h(base_serialization);
    OpalTable tbl = h.getOpalMsg(*client); // Mutates serialization w/guessing

    // If we are in guess mode, record what kind of serialization we
    // saw from the client.  I guess right now we do this for every
    // request?  TODO: Smarter? Or do we like that behaviour?
    if (base_serialization==OpalMsgExt_ADAPTIVE) {
      ProtectScope ps(collectionMutex_);
      if (h.serialization() == OpalMsgExt_ADAPTIVE) {
	log_.warning("Should have been able to guess a serialization, defaulting to Binary");
	serializations_[client_id] = OpalMsgExt_BINARY;
      } else {
	serializations_[client_id] = h.serialization();
      }
    }

    // Check to see if COMMAND = DISCONNECT is in here; that's a special
    // key and means that we need to disconnect the client quietly and
    // get out.
    
    if (tbl.contains("COMMAND")  &&
	tbl.get("COMMAND").stringValue() == "DISCONNECT") {
      log_.info("Client " + Stringize(client_id) + " requesting disconnection");
      cleanup = true;
    } else {
      // Convert RETURN ADDRESS to either file desc or upbyone number
      int_4 post_id = client_id;
      if (AddressType.value() == OpalPythonDaemon_UpByOne) {
	int_4 up_by_one = fdToUpByOne_[client_id];
	post_id = up_by_one;
      }

      // We have the contents.  But now we need to put the message in
      // an envelope.
      OpalTable envelope;
      envelope.put("RETURN ADDRESS", post_id);
      envelope.put("CONTENTS", tbl);
      OutMsg.post(envelope);
    }
  }
  // The following order matters.  UnexpectedEndOfStreamEx derives
  // from InexactStreamReadEx.
  catch (const UnexpectedEndOfStreamEx &e) {
    M2LOG_DEBUG(M2INFO, "Connection closed unexpectedly during read");
    cleanup = true;
  } catch (const InexactStreamReadEx& e) {
    log_.warning("Read unexpected number of bytes from client");
    cleanup = true;
  } catch (const BStreamException& e) {
    M2LOG_DEBUG(M2INFO, "Connection closded during read");
    cleanup = true;
  } catch (const MidasException& e) {
    log_.error("Encountered a MidasException while reading a message from client " + Stringize(client_id));
    log_.noteException(e);
    cleanup = true;
  } catch (const std::exception& e) {
    log_.error("C++ error:"+ Stringize(e.what()));
    cleanup = true;
  } catch (...) {
    log_.error("Non-midas exception caught during OpalPythonDaemon::serviceClient_(" + Stringize(client_id) + ")");
    cleanup = true;
  }
  
  bool disc_client = false;

  if (cleanup) {
    ProtectScope lock(collectionMutex_); 
    // This is a little tricky.  If the ref_cnt is zero, then we can
    // disconnect the client right now.  If its not zero there must be
    // writers waiting and by setting the canUseTable_'s flag the last
    // writer should notify us when they are all done.
    canUseTable_[client_id] = false;

    if (refCountTable_[client_id] == 0)
      disc_client = true;
  }


  // We can call disconnectClient_ directly (w/o having to post to
  // InternalDisconnectClient_), because this method is animated only
  // by this Service's thread and he's the only one who wakes up to
  // InternalDisconnectClient_.
  if (disc_client) {
    disconnectClient_(client_id);
  }
  
}					// OpalPythonDaemon::serviceClient_



void OpalPythonDaemon::newService_ ()
{
  newClient_.disable();

  try {
    serviceEntrance_.close();

    const OpalPythonDaemon_Role_e assigned_role = Role.value();

    switch (assigned_role) {
    case (OpalPythonDaemon_SERVER_AT_FIXED_PORT):
      serviceEntrance_.advertise(identifier(), Port.value());
      break;
      
    case (OpalPythonDaemon_SERVER_ANYWHERE):
      // TODO (ab 7-APR-00): Create a mix-in class to encompass this
      // functionality as it is used both here and in DualSocketPeer.
      serviceEntrance_.serviceName(identifier());
      serviceEntrance_.openAnyServer();
      portListener_.disable();
      Port.value(serviceEntrance_.port());
      portListener_.enable();
      break;

    case (OpalPythonDaemon_SERVER_IN_PORT_ZONE):
      serviceEntrance_.openNextAvailableServer(Port.value(), PortZoneAttempts.value(), 0);
      portListener_.disable();
      Port.value(serviceEntrance_.port());
      portListener_.enable();
      break;

    default:
      throw M2UNHANDLEDCASE_EX(assigned_role);
      M2BOGUS_BREAK;
    }

    newClient_.clearDescriptors();
    newClient_.addDescriptor(serviceEntrance_.descriptor());

    newClient_.enable();

    Advertised.post(true);
    PortFailed.post(false);
    log_.info("Advertised service " + serviceEntrance_.identifier());
    
    ProtectScope lock(collectionMutex_);
    inRundown_ = false;
  
  } catch (const SocketException& E) {
    log_.noteException(E);
    log_.error("Could not create service at port " +  Port.stringValue() + "; waiting for another");
    PortFailed.post(true);
  }
}					// OpalPythonDaemon::newService_



void OpalPythonDaemon::disconnectClient_ (int_4 client_id, bool user_requested)
{
  M2Mutex* mutex = 0;
  BStream* stream = 0;
  
  M2LOG_DEBUG(M2DEBUG_COARSE, "Attempting disconnection of client: " + Stringize(client_id));

  {
    ProtectScope lock(collectionMutex_);

    if (!bstreamTable_.contains(client_id)) {
      M2LOG_DEBUG(M2DEBUG_COARSE, "No such client " + Stringize(client_id) + "; disconnect is a no-op");  
      return;
    }

    // If this is user_requested, then we want to proceed ONLY if this
    // client's ref_cnt is zero.  If not, we need to set the
    // canUseTable_ flag and the last writer will notify us on the way out.
    
    canUseTable_[client_id] = false;

    if (refCountTable_[client_id] != 0) {
      M2LOG_DEBUG(M2DEBUG_COARSE, "Client marked for closure: " + client_id);
      // Can't cleanup just yet, we'll get the nod when its safe.
      return;
    }

    M2LOG_DEBUG(M2DEBUG_COARSE, "Disconnecting client " + Stringize(client_id));

    // Remove the mutex from the table.
    mutex = mutexTable_[client_id];
    mutexTable_.remove(client_id);

    // Forget how to look up the client.
    stream = bstreamTable_[client_id];
    bstreamTable_.remove(client_id);

    // Remove serialization for that client
    serializations_.remove(client_id);

    // Remove from fd/UpByOne conversion tables
    if (fdToUpByOne_.contains(client_id)) {
      int_4 up_by_one = fdToUpByOne_[client_id];
      fdToUpByOne_.remove(client_id);
      upByOneTofd_.remove(up_by_one);
    }


    // Hurry and release the collectionMutex_ so we don't hold up
    // traffic.
  }

  // Stop listening to activity on this descriptor BEFORE we close it, so
  // the socket monitor doesn't get upset about being asked to operate on
  // a bad descriptor.  (The close operation below will wake it with a
  // EBADF, and it'll find the corresponding request-to-close in its
  // remove list.)
  clientActivity_.removeDescriptor(client_id);
  
  // Delete the mutex.
  delete mutex;
  
  // Close the stream and delete it.
  stream->close(true);
  delete stream;
  
  log_.info("Disconnected client " + Stringize(client_id));

  if (user_requested) {
    postAction_(OpalPythonDaemon_Action_USER_DISCONNECT, client_id);
  } else {
    postAction_(OpalPythonDaemon_Action_DISCONNECTION, client_id);
  }

  bool rundown_service = false;

  { 
    ProtectScope lock(collectionMutex_);
    Index stream_cnt = bstreamTable_.entries();

    if (inRundown_ && stream_cnt == 0) {
      // Check if this is the last connection.
      finalRundown_();
      rundown_service = true;
    }
  }


  if (rundown_service) {
    // Update the readouts that describe the daemon state.
    postAction_(OpalPythonDaemon_Action_RUNDOWN, 0);
    Service::rundown_();
  }

}



void OpalPythonDaemon::postAction_ (OpalPythonDaemon_Action_e action, int_4 desc)
{
  OpalTable action_table;

  Vector vec = ClientVector.value();

  M2LOG_DEBUG(M2DEBUG_COARSE, "postAction_: Action " + Stringize(action) + " on descriptor " + Stringize(desc));
  switch (action) {
    
  case (OpalPythonDaemon_Action_CONNECTION):
    {
      action_table.put("ACTION", "CONNECT");
      action_table.put("CLIENT ID", desc);
      vec.expandBy(1, desc);      
    }
  break;
  
  case (OpalPythonDaemon_Action_USER_DISCONNECT):
  case (OpalPythonDaemon_Action_DISCONNECTION):
    {
      action_table.put("ACTION", "DISCONNECT");
      action_table.put("CLIENT ID", desc);

      if (action == OpalPythonDaemon_Action_USER_DISCONNECT)
	action_table.put("REASON", "USER REQUESTED");
      
      const int_4* vec_buf;
      Size         elements;
      Index        ii;
      
      OpenVectorForRead<int_4> ov(vec, vec_buf, elements);
      
      // find the element to remove
      for (ii = 0; ii < elements; ii++) {
	if (desc == vec_buf[ii])
	  break;
      }
      
      if (ii == 0) 
	vec = vec.saferange(1);
      else if (ii == (elements-1))
	vec = vec.saferange(0, ii-1);
      else
	vec = concatenate(vec.saferange(0, ii-1), vec.saferange(ii+1, elements));      
      
    }
    break;
    
  case (OpalPythonDaemon_Action_RUNDOWN):
    {
      action_table.put("ACTION", "RUNDOWN");
      vec.expandTo(0);
    }
    break;
    
  case OpalPythonDaemon_Action_POSTATTRIBUTES:
    // All we're doing is updating the attributes
    break;

  default:
    break;    
  }
  
  Clients.post(bstreamTable_.entries());
  ClientAction.post(action_table);
  ClientVector.post(vec);
}                                       // OpalPythonDaemon::postAction_


