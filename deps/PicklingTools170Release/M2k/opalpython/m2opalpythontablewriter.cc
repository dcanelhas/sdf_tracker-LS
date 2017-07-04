//


// ///////////////////////////////////////////// Include Files

#include "m2opalpythontablewriter.h"
#include "m2filenames.h"
#include "m2opalpython.h"
#include "m2openconser.h"
#include "m2ocstringtools.h" // For Image only!

// ///////////////////////////////////////////// OpalPythonTableWriter methods

OpalPythonTableWriter::OpalPythonTableWriter () :
  M2ATTR_VAL(FileName, ""),
  M2ATTR(StreamInput),
  M2ATTR_VAL(QueueSize, 2000),
  M2ATTR_VAL(CloseStream, OpalValue()),
  M2ATTR_VAL(WithNumeric, false),
  M2ATTR_VAL(UsesOCSerialization, false),
  streamOpen_(false)
{
//  StreamInput.queueSize(2000);
  StreamInput.queueSize(QueueSize.value());
  addWakeUp_(StreamInput, "Stream Input");
  addWakeUp_(QueueSize, "Queue Size");
  addWakeUp_(FileName, "File Name");
}   // OpalPythonTableWriter::OpalPythonTableWriter


bool OpalPythonTableWriter::actOnWakeUp_ (const string& waker_id)
{
  if (waker_id == "Queue Size") {
    //LJB_MSG("Changing queue size to " + Stringize(QueueSize.value()));
//    StreamInput.queueSize(QueueSize.value());
//    StreamInput.broadcast();
  }
  else if (waker_id == "File Name") {
    if (streamOpen_)
      stream_.close();
    string fn = FileName.value();
    try {
      bool uses_oc = UsesOCSerialization.value();
      std::ios_base::openmode mode 
	= (uses_oc) ? std::ios::binary | std::ios::out : std::ios::out;
      if (Names->extensionOf(fn).length() > 0) {
	stream_.open(getDataPathFor_(fn).c_str(), mode);
      }
      else {
	stream_.open(getDataPathFor_(fn+".tbl").c_str(), mode);
      }
      streamOpen_ = true;
    }
    catch (const MidasException& e) {
      log_.error("Problem opening file for writing:\n" + e.problem());
    }
  }
  else if (waker_id == "Close Stream") {
    stream_.close();
    streamOpen_ = false;
  }
  else if (waker_id == "Stream Input") {
    Size entries = StreamInput.queueEntries();
    for (Index i = 0; i < entries; i++) {
      if (DequeueOpalValue(*this, StreamInput, false)) {
	//LJB_MSG("Got OpalValue")
	try {
	  //LJB_POST(streamOpen_)
	  if (streamOpen_) {
	    processStreamInput_(StreamInput.value());
	    //LJB_MSG("streamed")
	  }
        }
	catch (MidasException& e) {
	  log_.warning("Error processing StreamInput");
	}
      }
    }
  }
  else {
    log_.warning("Invalid waking condition");
  }
  return true;
}   // OpalPythonTableWriter::actOnWakeUp_


void OpalPythonTableWriter::processStreamInput_ (const OpalTable& ot)
{
  bool uses_oc = UsesOCSerialization.value();
  if (uses_oc) {
    size_t bytes = BytesToSerialize(ot);
    string stringized;
    stringized.append(bytes, '\0');  // Grr.. no way to NOT initialize string
    char* data = (char*)stringized.data();
    char* final = Serialize(ot, data);
    size_t final_length = final-data;
    if (final_length != bytes) {
      string mesg = 
	"We overestimated the number of bytes: this happens sometimes";
      // cerr << mesg << endl;
      log_.info(mesg);
    }

    // Don't add an extra header: we already know the length
    /*    
    // Write the number of bytes, then the string itself
    if (final_length <= 0xFFFFFFFF) {
      // Small enough for int_u4
      int_u4 length = final_length;
      length = NetworkToNativeMachineRep(length);
      stream_.write((char*)&length, sizeof(length));
    } else {
      // Too big: write escape code, then int_u8
      int_u4 length = 0xFFFFFFFF;
      length = NetworkToNativeMachineRep(length);
      stream_.write((char*)&length, sizeof(length));
      int_u8 len8 = final_length;
      len8 = NetworkToNativeMachineRep(len8);
      stream_.write((char*)&len8, sizeof(len8));
    }
    */
    // cerr << PyImageImpl(stringized.length(), stringized.data()) << endl;
    stream_.write(stringized.data(), final_length); // stream_ << stringized; 
    stream_.flush();

  } else {
    bool with_numeric = WithNumeric.value();
    OpalValue ov = ot;
    Array<char> a(1024);
    {
      PythonBufferPickler<OpalValue> pbp(a, with_numeric);
      pbp.dump(ov);
    }
    string stringized(a.data(), a.length());
    stream_ << stringized;
  }
}   // OpalPythonTableWriter::processStreamInput_
