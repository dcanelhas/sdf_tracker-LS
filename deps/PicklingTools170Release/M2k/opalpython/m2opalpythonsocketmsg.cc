//

// ///////////////////////////////////////////// Include Files

#include "m2opalpythonsocketmsg.h"
#include "m2bstreame.h"
#include "m2sockete.h"
#include "m2opalmsgextnethdr.h"


// ///////////////////////////////////////////// OpalPythonSocketMsg Methods

OpalPythonSocketMsg::OpalPythonSocketMsg () :
  M2ATTR_VAL(Serialization, OpalMsgExt_PYTHON_NO_NUMERIC),
  M2ATTR_VAL(InMsg, OpalTable()),
  M2ATTR_VAL(OutMsg, OpalTable()),
  M2ATTR(Connect),
  M2ATTR(Disconnect),
  peerClosed_(false),
  listen_(this, &OpalPythonSocketMsg::newInMsg_),
  listenToConnect_(this, &OpalPythonSocketMsg::changeConnectState_)
{ }



void OpalPythonSocketMsg::onNewConnection_ ()
{
  listenToConnect_.listenToChanges(Disconnect);
  listen_.listenToChanges(InMsg);
}



void OpalPythonSocketMsg::beforeDisconnect_ ()
{
  listen_.ignoreAll();
  if (peerClosed_)
    peerClosed_ = false;
  else {
    if (!disconnectWithoutReply_) {
      OpalTable tbl;
      tbl.put("COMMAND", OpalValue("DISCONNECT"));
      sendMessage_(tbl);
    }
  }

  listenToConnect_.listenToChanges(Connect);
}



void OpalPythonSocketMsg::sendMessage_ (const OpalTable& tbl)
{
  if (connection_.isOpen() == false) {
    M2LOG_DEBUG(M2DEBUG_COARSE, "Ignoring message directed to closed connection");
    return;
  }

  bool bad_write = false;

  try {
    OpalMsgExtNetHdr h(Serialization.value());
    h.sendOpalMsg(connection_, tbl);
  } catch (const BStreamException& e) {
    M2LOG_DEBUG(M2INFO, "Incomplete write: Connection closed");
    M2LOG_DEBUG(M2DEBUG_COARSE, e.problem());
    bad_write = true;
  } catch (const MidasException& e) {
    log_.error("MidasException caught during delivery of a message");
    log_.noteException(e);
    bad_write = true;
  } 

  if (bad_write) {
    peerClosed_ = true;
    disconnectQuietly_();
  }
}					// OpalPythonSocketMsg::sendMessage_



void OpalPythonSocketMsg::incomingData_ ()
{
  bool bad_read = false;

  OpalTable tbl;
  try {
    OpalMsgExtNetHdr h(Serialization.value());
    tbl = h.getOpalMsg(connection_);
  } catch (const UnexpectedEndOfStreamEx& e) {
    log_.warning("Connection closed");
    bad_read = true;
  } catch (const InexactStreamReadEx& e) {
    log_.warning("Read unexpected number of bytes from peer");
    bad_read = true;
  } catch (const BStreamException& e) {
    M2LOG_DEBUG(M2INFO, "Connection closed during read");
    bad_read = true;
  } catch (const MidasException& e) {
    log_.error("Encountered a MidasException while reading from peer");
    log_.noteException(e);
    bad_read = true;
  }
  
  if (bad_read) {
    peerClosed_ = true;
    disconnectQuietly_();
    return;
  }

  if (tbl.entries() != 0) {
    // Check for the special DISCONNECT message
    SEVal<OpalValue> cmd = tbl.find("COMMAND");
    if (cmd.isValid()  &&  cmd.value().stringValue() == "DISCONNECT") {
      disconnectQuietly_();
    } else {
      getMessage_(tbl);
    }
  } else {
    log_.warning("Empty message received; ignoring message");
  }

}					// OpalPythonSocketMsg::readMessage_



void OpalPythonSocketMsg::getMessage_ (const OpalTable& tbl)
{
  OutMsg.post(tbl);
}



void OpalPythonSocketMsg::newInMsg_ (const ChangesMessage& msg)
{
  const ProtectScope lock(inMsgLock_);
  sendMessage_(msg.value());
}



void OpalPythonSocketMsg::changeConnectState_ (const ChangesMessage& msg)
{
  if (msg.mostRecent(Connect)) {
    if (Connected.value() == true)
      return;
    
    newConnection_();
  }

  else if (msg.mostRecent(Disconnect)) {
    if (Connected.value() == false)
      return;

    disconnect_();
  }

}


