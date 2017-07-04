#ifndef M2OPALMSGEXTNETHDR_H_

// Follows the pattern of OpalMsgNetHdr, but this adds extra different
// serializations (PythonPickling 0 and 2, OpenContainers). and makes
// it so we can do ADAPTIVE serialization (each client can have its
// own serialization).

// ///////////////////////////////////////////// Include Files

#include "m2types.h"
#include "m2convertrep.h"
#include "m2bstream.h"
#include "m2opaltable.h"
#include "m2enumutils.h"


// ///////////////////////////////////////////// Type Definitions

enum OpalMsgExt_Serialization_e {

  OpalMsgExt_ASCII = 0,
  OpalMsgExt_BINARY = 1,
  OpalMsgExt_PYTHON_NO_NUMERIC = 2,
  OpalMsgExt_PYTHON_WITH_NUMERIC = 3,

  OpalMsgExt_ADAPTIVE= 4, // Means try to figure out what serialization type of a clint by looking at the message

  OpalMsgExt_OLD_ASCII = 5,

  OpalMsgExt_PYTHON_P2 = 6,
  OpalMsgExt_PYTHON_P2_WITH_ARRAY = 7,
  OpalMsgExt_PYTHON_P2_WITH_NUMERIC = 8,


  // Rarely, we will need to talk to version of Python 2.2 that uses
  // Pickling protocol 2, but unfortunately it doesn't "quite" work
  // like later Pythons.  This is important because a lot of people
  // still use Python 2.2.
  OpalMsgExt_PYTHON_P2_OLD = 9,
  OpalMsgExt_PYTHON_P2_OLD_WITH_NUMERIC = 10,

  // OpenContainers serialization
  OpalMsgExt_OPENCONTAINERS = 11,

  // Old loader for P0
  OpalMsgExt_PYTHON_OLD_NO_NUMERIC = 12,
  OpalMsgExt_PYTHON_OLD_WITH_NUMERIC = 13,

  // Added NumPy support
  OpalMsgExt_PYTHON_WITH_NUMPY = 14,
  OpalMsgExt_PYTHON_P2_WITH_NUMPY = 15,

  OpalMsgExt_LENGTH = 16     // Length


};

M2ENUM_UTILS_DECLS(OpalMsgExt_Serialization_e)

// ///////////////////////////////////////////// OpalMsgNetHdr

// All communications of OpalClient and OpalDaemon must be
// preceded by 8 bytes:  the first 4 bytes tell the length
// of the message following in network byte order.
// The next 4 bytes are 4 characters which describe the machine
// representation of the sender of the message.

class OpalMsgExtNetHdr {

  public:

    // ///// Methods

    // Constructor.

    OpalMsgExtNetHdr (OpalMsgExt_Serialization_e serial_kind);


    // Gets an OpalTable from the stream.  Ends up calling
    // readFromStream, as the header reads itself first.
    // The type of header read will also depend on the
    // serialization mode.

    OpalTable getOpalMsg (BStream& bin);


    // Sends the given OpalTable over the given stream, preceded
    // by this header.

    void sendOpalMsg (BStream& bout, const OpalTable& tbl);


    // Prepares a message for sending by writing it to the
    // OMemStream.  A good idea for when you will be sending
    // the message to multiple recipients.

    void prepareOpalMsg (OMemStream& mout, const OpalTable& tbl);


    // Assumes that the OMemStream m is the one modified by
    // prepareOpalMsg.

    void sendPreparedMsg (BStream& bout, const OMemStream& m);


    // Inspector for the serialization to use ... this may change
    // is the user specifies the "any" type

    OpalMsgExt_Serialization_e serialization () const { return serialKind_; }


    // Read the buffer, based on current serialization and return a Table

    OpalTable interpretBuffer (size_t msglen, char* buffer, 
			       StreamDataEncoding& sde);

  private:

    // ///// Data Members

    // What kind of serialization to use.

    OpalMsgExt_Serialization_e	serialKind_;


    // ///// Methods

    // Try to "guess" the serialization being used: This has the
    // effect of actually changing what the serialKind is!.  This
    // returns the read OpalTable from the guessed stream.

    OpalTable guessing_ (BStream& bin);


    // Read the buffer, based on current serialization and return a Table
    OpalTable interpret_ (size_t msglen, ArrayPotHolder<char>& buffer,
			  StreamDataEncoding& sde);

    // Should be code from BStream ... see .cc file for details

    void readContinueStreamDataEncoding_ (BStream& bin,
					  StreamDataEncoding& sde, 
					  char rep[4]);


};

#define M2OPALMSGEXTNETHDR_H_
#endif // M2OPALMSGEXTNETHDR_H_


