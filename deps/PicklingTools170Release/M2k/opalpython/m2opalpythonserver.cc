//

// ///////////////////////////////////////////// Include Files

#include <fstream>
#include "m2opale.h"
#include "m2opalpythonserver.h"
#include "m2potholder.h"


// ///////////////////////////////////////////// OpalPythonServer Methods

OpalPythonServer::OpalPythonServer () :
  M2ATTR_VAL(InMsg, OpalTable()),
  M2ATTR_VAL(InMsgBypass, OpalTable()),
  M2ATTR_VAL(OutMsg, OpalTable()),
  M2ATTR_VAL(ClientVector, Vector(NumericType(int_4), 0)),
  M2ATTR_VAL(ClientAction, OpalTable()),
  M2ATTR_VAL(DisconnectClient, ""),
  M2ATTR_VAL(UniqueClientIds, true),
  M2ATTR_VAL(AllowExternalThreadWrites, false),
  M2ATTR_VAL(UniqueIdToClientIdMapping, OpalTable()),
  M2ATTR_VAL3(Uptime, string(), &OpalPythonServer::getUpTime_, NULL),
  startTime_(M2Time()),
  nextUniqueId_(0),
  listenToDaemon_(this, &OpalPythonServer::activityFromDaemon_),
  listenToAttrs_(this, &OpalPythonServer::attributeChanged_),
  M2ATTR_REF(Role, daemon_.Role),
  M2ATTR_REF(SingleSocket, daemon_.SingleSocket),
  M2ATTR_REF(PortFailed, daemon_.PortFailed),
  M2ATTR_REF(Serialization, daemon_.Serialization),
  M2ATTR_REF(AdaptiveDefaultSerialization, daemon_.AdaptiveDefaultSerialization),
  M2ATTR_REF(Port, daemon_.Port),
  M2ATTR_REF(ClientType, daemon_.ClientType),
  M2ATTR_REF(Advertised, daemon_.Advertised),
  M2ATTR_REF(Clients, daemon_.Clients),
  M2ATTR_REF(InMsgQueueSize, daemon_.InMsgQueueSize),
  M2ATTR_REF(QuerySocketBeforeWrite, daemon_.QuerySocketBeforeWrite),
  M2ATTR_REF(DisconnectQueueSize, daemon_.DisconnectQueueSize),
  M2ATTR_REF(ClientReadDataTimeLimit, daemon_.ClientReadDataTimeLimit),
  M2ATTR_REF(ClientWriteDataTimeLimit, daemon_.ClientWriteDataTimeLimit),
  M2ATTR_REF(PortZoneAttempts, daemon_.PortZoneAttempts)
{
  // Should this be BINARY?  No, because the default adaptive serialization is
  // binary which has the same effect.
  Serialization = OpalMsgExt_ADAPTIVE; 
}



void OpalPythonServer::useMyComponents_ ()
{
  ComponentCrew::useMyComponents_();

  use(daemon_);
}



void OpalPythonServer::assembleMyself_ ()
{
  ComponentCrew::assembleMyself_();
  
  listenToDaemon_.listenToChanges(daemon_.OutMsg);
  listenToDaemon_.listenToChanges(daemon_.ClientAction);
  listenToDaemon_.listenToChanges(daemon_.ClientVector);

  listenToAttrs_.listenToChanges(InMsg);
  listenToAttrs_.listenToChanges(InMsgBypass);
  listenToAttrs_.listenToChanges(DisconnectClient);
}


void OpalPythonServer::startMyself_ ()
{
  ComponentCrew::startMyself_();

  while (Advertised.value() == false && PortFailed.value() == false) {
    M2LOG_DEBUG(M2INFO, "Waiting for OpalPythonDaemon to Advertise");
    Delay(.5);
  }
}


bool OpalPythonServer::disconnectClient_ (const string& client_id)
{
  string id = client_id;

  if (UniqueClientIds.isTrue()) {
    if ((id = getClientID_(client_id)) == "") {
      log_.info("Ignoring disconnect request for non-existent client: " 
		+ client_id);
      return false;
    }
  }

  daemon_.disconnectClient(UnStringize(id, int_u4));
  return true;
}



bool OpalPythonServer::writeToClient_ (const string& client_id,
				 const OpalTable& msg,
				 const bool droppable)
{
  string id = client_id;

  if (UniqueClientIds.isTrue()) {
    if ((id = getClientID_(client_id)) == "") {
      log_.info("Ignoring write to non-existent client: " + client_id);
      return false;
    }
  }

  OpalTable out_msg;
  out_msg.put("ADDRESS", id);
  out_msg.put("CONTENTS", msg);

  // We're not really broadcasting this message, broadcastToClients_() 
  broadcastToClients_(out_msg, droppable);
  return true;
}



void OpalPythonServer::broadcastToClients_ (const OpalTable& msg, 
				      const bool droppable)
{
  OpalTable envelope(msg);
  if (droppable) {
    envelope.put("DROPPABLE", Opalize(true));
  }

  if (AllowExternalThreadWrites.isTrue()) {
    daemon_.InMsgBypass.value(envelope);
  } else {
    daemon_.InMsg.value(envelope);
  }
}

  

void OpalPythonServer::activityFromDaemon_ (const ChangesMessage& msgin)
{
  // This method is animated by OpalDaemon's thread, either through
  // its OutMsg, or its ClientAction.
  if (!active()) 
    return;

  if (msgin.mostRecent(daemon_.OutMsg)) { 
      handleClientMessage_(msgin.value());
  } else if (msgin.mostRecent(daemon_.ClientAction)) {
      handleClientAction_(msgin.value());
  } else if (msgin.mostRecent(daemon_.ClientVector)) {
    handleClientVector_(msgin.value().vector());
  }
}



void OpalPythonServer::attributeChanged_ (const ChangesMessage& chmsg)
{
  if (chmsg.mostRecent(InMsg) || chmsg.mostRecent(InMsgBypass)) {
    OpalTable msg(chmsg.value());
    OpalValue addr, contents;
    if (msg.tryGet("ADDRESS", addr) && msg.tryGet("CONTENTS", contents)) {
      // Need to look for DROPPABLE, so writeToClient_ will know
      // whether to place it in the envelope it will prepare for
      // OpalDaemon.
      bool droppable = false;
      OpalValue dropv;
      if (msg.tryGet("DROPPABLE", dropv)) {
	droppable = UnOpalize(dropv, bool);
      }
      (void)writeToClient_(addr.stringValue(), contents, droppable);
    } else {
      // We don't need to worry about looking for DROPPABLE here, if
      // it was put into the envelope before it got to us, it will
      // remain there.  We're not breaking up and reconstructing the
      // message like we're doing above.
      broadcastToClients_(msg);
    }
  } else if (chmsg.mostRecent(DisconnectClient)) {
    (void)disconnectClient_(chmsg.value().stringValue());
  } else {
    throw M2UNHANDLEDCASE_EX(chmsg.mostRecent().identifier());
  }
}



string OpalPythonServer::getUpTime_ () const
{
  return Stringize(M2Time() - startTime_);
}



void OpalPythonServer::handleClientMessage_ (const OpalTable& original_envelope)
{  
  if (original_envelope.length() == 0) {
    return;
  }

  OpalTable envelope(original_envelope);

  OpalTable msg = envelope.get("CONTENTS");
  string client_id = envelope.get("RETURN ADDRESS").stringValue();
  
  if (UniqueClientIds.isTrue()) {
    client_id = getUniqueID_(client_id);
    envelope.put("RETURN ADDRESS", client_id);
  }

  handleIncomingClientMessage_(client_id, msg);
  OutMsg.post(envelope);
}                      // OpalPythonServer::commandFromDaemon_



void OpalPythonServer::handleClientAction_ (const OpalTable& action_tbl) 
{
  if (action_tbl.length() == 0) {
    return;
  }

  OpalTable act_tbl(action_tbl);

  const string action = act_tbl.get("ACTION").stringValue();
  string client_id = act_tbl.get("CLIENT ID").stringValue();

  if (action == "DISCONNECT") {
    bool user_requested = false;

    OpalValue reason;
    if (act_tbl.tryGet("REASON", reason)) {
      user_requested = (reason.stringValue() == "USER REQUESTED");
    }
 
    if (UniqueClientIds.isTrue()) {
      string unique_id = removeClientID_(client_id);
      act_tbl.put("CLIENT ID", UnStringize(unique_id, int_4));
      client_id = unique_id;
    }
    M2LOG_DEBUG(M2DEBUG_FINE, "Recieved notification of client disconnection: " + client_id);
    handleClientDisconnection_(client_id, user_requested);
  }
    
  else if (action == "CONNECT") {
    if (UniqueClientIds.isTrue()) {
      client_id = createUniqueId_(client_id);
      act_tbl.put("CLIENT ID", UnStringize(client_id, int_4));
    }
    M2LOG_DEBUG(M2DEBUG_FINE, "Recieved notification of new client connection: " + client_id);
    handleClientConnection_(client_id);
  } else if (action == "RUNDOWN") {
    M2LOG_DEBUG(M2DEBUG_FINE, "Recieved notification of RUNDOWN by OpalDaemon");
    handleDaemonRundown_();
  } else {
    log_.error("Unknown action notification from daemon_: " + action);
    return;
  }

  ClientAction.post(act_tbl);
}



void OpalPythonServer::handleClientVector_ (const Vector& in_vec)
{
  if (UniqueClientIds.isFalse()) {
    ClientVector.post(in_vec);
    return;
  }

  Vector out_vec(NumericType(int_4), 0);

  const int_4* vec_buf;
  Size elements;
  
  OpenVectorForRead<int_4> ov(in_vec, vec_buf, elements);
  for (Index ii = 0; ii < elements; ii++) {
    out_vec.expandBy(1, UnStringize(getUniqueID_(Stringize(vec_buf[ii])), int_4));
  }
  
  ClientVector.post(out_vec);
}



// The following methods perform the generation, mapping and managing
// of unique id's for each client.

string OpalPythonServer::createUniqueId_ (const string& client_id)
{
  string unique_id; 
  { 
    ProtectScope lock(mapMutex_);
    unique_id = Stringize(++nextUniqueId_);
    uniqueClientMap_.put(unique_id, client_id);
  }
  UniqueIdToClientIdMapping.post(uniqueClientMap_);
  return unique_id;
}



string OpalPythonServer::getUniqueID_ (const string& client_id) 
{
  string unique_id;
  ProtectScope lock(mapMutex_);
  if (!uniqueClientMap_.containsValue(unique_id, client_id)) {
    throw MidasException("Client id not in mapping: " + client_id);
  }
  return unique_id;
}



string OpalPythonServer::getClientID_ (const string& unique_id)
{
  string client_id;
  ProtectScope lock(mapMutex_);
  try {
    client_id = uniqueClientMap_.get(unique_id).stringValue();
  } catch (const MidasException&) { }
  return client_id;
}



string OpalPythonServer::removeClientID_ (const string& client_id)
{
  string unique_id;
  {
    ProtectScope lock(mapMutex_);
    if (!uniqueClientMap_.containsValue(unique_id, client_id)) { 
      throw MidasException("Client id not in mapping: " + client_id);
    }
    uniqueClientMap_.remove(unique_id);
  }
  UniqueIdToClientIdMapping.post(uniqueClientMap_);
  return unique_id;
}
