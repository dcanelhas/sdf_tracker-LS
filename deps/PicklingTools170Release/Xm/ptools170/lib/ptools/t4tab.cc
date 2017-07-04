#include <t4tab.h>

PTOOLS_BEGIN_NAMESPACE
//
///////////////////////////////////////////////////////////////////////////////
// FUNCTION: initT4Tab
//
// DECRIPTION:
//   This function initializes and opens a T4000 file/pipe.
//
///////////////////////////////////////////////////////////////////////////////
void initT4Tab (CPHEADER& hcb, string name, int_4 hcbfmode)
{
  initT4Val(hcb, name, hcbfmode);
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: sendT4Tab
//
// DECRIPTION:
//   This function sends a Tab through an already open T4000 file/pipe.
//   This is done by using a VRBSTRUCT simply as a pointer to an external
//   buffer which is then sent down the pipe.  The receiving end must know
//   that what it receives is not a normal VRBSTRUCT and handle the buffer
//   appropriately.  In this case the buffer contains a serialized Tab
//   object, so the receiver must deserialize it to gain access to the Tab.
//   recvT4Tab was written for this purpose.
//
// NOTES:
//   In order to use these functions properly, initT4Tab should be used
//   initialize the file/pipe and sendT4Tab and recvT4Tab should be used to
//   add and grab data to or from the file/pipe.
//
///////////////////////////////////////////////////////////////////////////////
void sendT4Tab (CPHEADER& hcb, const Tab& t, string vrb_key, Serialization_e ser)
{
  sendT4Val(hcb, t, vrb_key, ser);
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: recvT4Tab
//
// DECRIPTION:
//   This function receives a Tab through an already open T4000 file/pipe.
//   This is done by using the T4000 VRBSTRUCT.
//
// NOTES:
//   In order to use these functions properly, initT4Tab should be used
//   initialize the file/pipe and sendT4Tab and recvT4Tab should be used to
//   add and grab data to or from the file/pipe.
//   The recvT4Tab function is non-blocking, so the returned Tab must be
//   emptiness-tested before use.
//
///////////////////////////////////////////////////////////////////////////////
bool recvT4Tab (CPHEADER& hcb, Tab& t, string vrb_key, Serialization_e ser)
{
  Val v;
  if (recvT4Val(hcb, v, vrb_key), ser) {
    if (v.tag == 't') {
      t = v;
      return true;
    }
  }
  else {
    return false;
  }
  
  return false;
}

PTOOLS_END_NAMESPACE
