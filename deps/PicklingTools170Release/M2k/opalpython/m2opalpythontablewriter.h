#ifndef M2OPALPYTHONTABLEWRITER_H_

//


// ///////////////////////////////////////////// Include Files

#include "m2service.h"
#include "m2recattr.h"
#include "m2menvironment.h"
#include <fstream>


// ///////////////////////////////////////////// The OpalPythonTableWriter Class

M2RTTI(OpalPythonTableWriter)

class OpalPythonTableWriter : public Service {

    M2PARENT(Service)

  public:

    // ///// Midas 2k Attributes

    RTParameter<string>    FileName;

    ReceiverAttribute      StreamInput;

    RTParameter<Size>      QueueSize;

    RTParameter<OpalValue> CloseStream;

    RTBool                 WithNumeric;

    RTBool                 UsesOCSerialization;

    // ///// Methods

    OpalPythonTableWriter ();


  protected:

    // ///// Data Members

    std::ofstream stream_;

    bool streamOpen_;


    // ///// Methods

    bool actOnWakeUp_ (const string& waker_id);

    void processStreamInput_ (const OpalTable& ot);

    inline string getDataPathFor_ (const string& file_name, bool append_file_name = true);


};   // OpalPythonTableWriter



// ///////////////////////////////////////////// inline functions

inline string OpalPythonTableWriter::getDataPathFor_ (const string& file_name,
						bool append_file_name)
{
  return M2Env().writePath().getDirectory(file_name, append_file_name);
}   // OpalPythonTableWriter::getDataPathFor_ 


#define M2OPALPYTHONTABLEWRITER_H_
#endif // M2OPALPYTHONTABLEWRITER_H_

