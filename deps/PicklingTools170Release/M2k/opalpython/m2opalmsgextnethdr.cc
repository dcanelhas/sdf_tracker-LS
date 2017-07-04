//

// ///////////////////////////////////////////// Include Files

#include "m2opalmsgextnethdr.h"
#include "m2opalpython.h"
#include "m2opalprotocol2.h"
#include "m2openconser.h"
#include "m2pickleloader.h" // new implementation of the loader

// The following are for htonl and ntohl
#include <sys/types.h>
#include <netinet/in.h>

#ifdef IRIX_
#include <sys/endian.h>                 // define ntohl
#endif

// ///////////////////////////////////////////// Alphabet Functions

M2ALPHABET_BEGIN_MAP(OpalMsgExt_Serialization_e)
  AlphabetMap("ASCII",	OpalMsgExt_ASCII);
  AlphabetMap("Binary",	OpalMsgExt_BINARY);
  AlphabetMap("PythonNoNumeric",	OpalMsgExt_PYTHON_NO_NUMERIC);
  AlphabetMap("PythonWithNumeric",	OpalMsgExt_PYTHON_WITH_NUMERIC);
  AlphabetMap("PythonWithNumPy",	OpalMsgExt_PYTHON_WITH_NUMPY);
  AlphabetMap("Adaptive",	OpalMsgExt_ADAPTIVE);
  AlphabetMap("Old ASCII", OpalMsgExt_OLD_ASCII);
  AlphabetMap("PythonP2", OpalMsgExt_PYTHON_P2);
  AlphabetMap("PythonP2WithArray", OpalMsgExt_PYTHON_P2_WITH_ARRAY);
  AlphabetMap("PythonP2WithNumeric", OpalMsgExt_PYTHON_P2_WITH_NUMERIC);
  AlphabetMap("PythonP2WithNumPy", OpalMsgExt_PYTHON_P2_WITH_NUMPY);
  AlphabetMap("PythonP2Old", OpalMsgExt_PYTHON_P2_OLD);
  AlphabetMap("PythonP2OldWithNumeric", OpalMsgExt_PYTHON_P2_OLD_WITH_NUMERIC);
  AlphabetMap("OpenContainers", OpalMsgExt_OPENCONTAINERS);
M2ALPHABET_END_MAP(OpalMsgExt_Serialization_e)

M2ENUM_UTILS_DEFS(OpalMsgExt_Serialization_e)



// ///////////////////////////////////////////// OpalMsgExtNetHdr Methods

OpalMsgExtNetHdr::OpalMsgExtNetHdr (OpalMsgExt_Serialization_e serial_kind) :
  serialKind_(serial_kind)
{ }


// HELPER FUNCTIONS TO DEAL WITH very large buffers

template <class T>
void swap(T& a, T& b)
{
  T temp=a;
  a=b;
  b=temp;
}

// Handle 8 byte Network Host Order
inline int_u8 ntohll (int_u8 bytes)
{
  int_u8 check = 1;
  char *check1 = reinterpret_cast<char*>(&check);
  if (*check1==1) {
    // This is little-endian machine, have to convert to big
    char *look = reinterpret_cast<char*>(&bytes);
    swap(look[0], look[7]);
    swap(look[1], look[6]);
    swap(look[2], look[5]);
    swap(look[3], look[4]);
    return bytes;
  } else {
    // Big endian, do nothing
    return bytes;
  }
}

// Get the message length of the message: it may be 4 bytes or 12 bytes
inline size_t getMessageLen (BStream& bin)
{
  size_t true_message_len = 0;

  // Always starts with message length
  int_u4 msglen;
  bin.readExact(&msglen, sizeof(msglen));
  msglen = NetworkToNativeMachineRep(msglen);

  // Is this a standard 4 byte message, or the escape seq 0XFFFFFFFF
  // indicating another 8 bytes is coming?
  if (msglen==int_u4(0xFFFFFFFF)) {
    int_u8 msglen8 = 0;
    bin.readExact(&msglen8, sizeof(msglen8));
    msglen8 = ntohll(msglen8);
    true_message_len = msglen8;
  } else {
    true_message_len = msglen;
  }
  // cerr << "GOT A MESSAGE LENGTH!" << true_message_len << endl;
  return true_message_len; 
}

// ReadExact only reads int_u4 bytes (maybe even int_4?) so we 
// break it up into that sized chunks if we want to read
// very large buffers
inline void BinReadExact (BStream& bin, char* buff, size_t bytes)
{
  size_t bytes_to_read = bytes;
  size_t current = 0;
  while (bytes_to_read) {
    int_4 chunk_size = (1) << 30;  // 
    if (bytes_to_read < size_t(chunk_size)) {
      chunk_size = bytes_to_read;
    }
    bin.readExact(buff+current, chunk_size);
    current += chunk_size;
    bytes_to_read -= size_t(chunk_size);
  }
  // cerr << "READ ALL " << bytes << endl;
}

inline void BinWrite (BStream& bout, char* buff, size_t bytes)
{
  size_t bytes_to_write = bytes;
  size_t current = 0;
  while (bytes_to_write) {
    int_u4 chunk_size = (1) << 30;
    if (bytes_to_write < size_t(chunk_size)) {
      chunk_size = bytes_to_write;
    }
    bout.write(buff+current, chunk_size);
    current += chunk_size;
    bytes_to_write -= size_t(chunk_size);
  }
  // cerr << "WROTE ALL" << bytes << " " << current << endl;
}


OpalTable OpalMsgExtNetHdr::guessing_ (BStream& bin)
{
  size_t msglen = getMessageLen(bin);

  // Guessing requires a bit of peeking into the next few bytes: 
  // Read the next 4 bytes ... they tell us a lot
  char rep[4];
  bin.readExact(rep, sizeof(rep));
  
  string first_four(rep, sizeof(rep));
  if (first_four=="M2BD") { 
    // This is a special key to indicate an extended
    // header for serialization
    serialKind_ = OpalMsgExt_BINARY;
  } else if (first_four=="IEEE" || first_four=="EEEI") {
    // Older m2k serialization still supported, std stuff
    serialKind_ = OpalMsgExt_BINARY;
  } else if (rep[0]=='O' && rep[1]=='C') {
    // OpenContainers binary serialization
    serialKind_ = OpalMsgExt_OPENCONTAINERS;
  } else if (rep[0] == 'C' || rep[0] == 'V') {
    // Older m2k serialization still supported, CRAY or VAX
    serialKind_ = OpalMsgExt_BINARY;
  } else if (rep[0] == '{') {
    // Probably M2k ASCII
    serialKind_ = OpalMsgExt_ASCII;
  } else if (rep[0] == 'P' && rep[1]=='Y') {
    // Probably Python serialization Protocol 
    if (rep[3]=='2') {
      if (rep[2]=='N') {
	serialKind_ = OpalMsgExt_PYTHON_P2_WITH_NUMERIC;
      } else if (rep[2]=='A') {
	serialKind_ = OpalMsgExt_PYTHON_P2_WITH_ARRAY;
      } else if (rep[2]=='0') {
	serialKind_ = OpalMsgExt_PYTHON_P2;
      } else if (rep[2]=='U') {
	serialKind_ = OpalMsgExt_PYTHON_P2_WITH_NUMPY;
      }	else {
	throw MidasException("Unsupported Python header:"+first_four);
      }
      // version # in rep[3]
    } else if (rep[3]=='-') {
      if (rep[2]=='N') {
	serialKind_ = OpalMsgExt_PYTHON_P2_OLD_WITH_NUMERIC;
      } else if (rep[2]=='0') {
	serialKind_ = OpalMsgExt_PYTHON_P2_OLD;
      } else {
	throw MidasException("Unsupported Python header:"+first_four);
      }
    } else if (rep[3]=='0') { 
      if (rep[2]=='N') {
	serialKind_ = OpalMsgExt_PYTHON_WITH_NUMERIC;
      } else if (rep[2]=='0') {
	serialKind_ = OpalMsgExt_PYTHON_NO_NUMERIC;
      } else if (rep[2]=='U') {
	serialKind_ = OpalMsgExt_PYTHON_WITH_NUMPY;
      } else {
	throw MidasException("Unsupported Python header:"+first_four);
      }
    } else {
      throw MidasException("Unsupported Python header:"+first_four);
    }
  } else {
    // Not sure, old ASCII??
    serialKind_ = OpalMsgExt_OLD_ASCII;
  }
  //cerr << "SERIAL:" << serialKind_ << endl;

  // Normally, we would do below, but because we have already read
  // 4 bytes we have to take that into account.  There should be
  // a new method on bstream called "readContinueStreamDataEncoding",
  // but that's baseline and hard to change, so we replicate code
  // here.
  /*if (OpalMsgExt_BINARY == serialKind_) {
    bin.readStreamDataEncoding(sde);
    }
    
    ArrayPotHolder<char> buffer(new char[msglen + 1]);
    bin.readExact((void *)buffer.data(), msglen);
    buffer[msglen] = 0;
  */
  
  int_8 correction = -1;
  ArrayPotHolder<char> buffer(new char[msglen + 1]);  
  StreamDataEncoding sde;
  if (OpalMsgExt_BINARY == serialKind_) {
    readContinueStreamDataEncoding_(bin, sde, rep);
    correction = 0; // The msglen is still same, the extra 4 bytes were for
                    // further headers
  } else if (serialKind_ == OpalMsgExt_OLD_ASCII || 
	     serialKind_==OpalMsgExt_ASCII) {
    correction = sizeof(rep); // Take into account already read 4 bytes :
                              // we need those for data!
    memcpy(buffer.data(), rep, correction);
  } else {
    correction = 0; // Everything else does have a 4 byte header after len
  }
	       
  char* buff = buffer.data();
  BinReadExact(bin, buff+correction, msglen-correction);
  buffer[msglen] = 0;

  // cerr << "About to call interpret_" << endl;
  return interpret_(msglen, buffer, sde);
}



OpalTable OpalMsgExtNetHdr::interpret_ (size_t msglen, 
					ArrayPotHolder<char>& buffer,
					StreamDataEncoding& sde)
{ return interpretBuffer(msglen, buffer.data(), sde); }

OpalTable OpalMsgExtNetHdr::interpretBuffer (size_t msglen, 
					     char* buffer,
					     StreamDataEncoding& sde)
{
  OpalTable tbl;
  // Do the actual interpretation
  if (serialKind_ != OpalMsgExt_BINARY && 
      serialKind_ != OpalMsgExt_PYTHON_P2 && 
      serialKind_ != OpalMsgExt_PYTHON_P2_WITH_ARRAY && 
      serialKind_ != OpalMsgExt_PYTHON_P2_WITH_NUMERIC &&
      serialKind_ != OpalMsgExt_PYTHON_P2_WITH_NUMPY &&
      serialKind_ != OpalMsgExt_PYTHON_P2_OLD &&
      serialKind_ != OpalMsgExt_PYTHON_P2_OLD_WITH_NUMERIC &&
      serialKind_ != OpalMsgExt_OPENCONTAINERS) {
    
    // TODO (dtj 18-SEP-98): Need to be able to handle all types of
    // ASCII serialization, which means recognizing all kinds of
    // line termination sequences.  Some examples:
    //
    // UNIX:  LF
    // MacOS: CR
    // PC:    CR/LF
    // VMS:   LF/CR
    //
    // But for now, the only one that's killing me is a CR instead of
    // an LF, which I shall handle here by just replacing CRs with LFs.

    // This scheme turns MacOS (CR) line terminations and PC (CR/LF)
    // terminations into UNIX ones.
    // TODO (dtj 18-SEP-98): We'd like to bail out early if we realize
    // that it's coming from a UNIX machine.

    char* rptr = buffer; // .data();
    char* wptr = rptr;

    for (; *rptr != 0; rptr++, wptr++) {
      if (*rptr == '\n') break;
      if (*rptr == '\r') {
	if (*(rptr + 1) == '\n') {
	  // This is the PC way.  Drop this CR and fall through to use
	  // the LF.
	  rptr++;
	} else {
	  // This is the MacOS way.  Write an LF *instead* of a CR;
	  // go back around.
	  *wptr = '\n';
	  continue;
	}
      }			// If there's a nasty ol' CR
      *wptr = *rptr;
    }

    // If the read and write pointers are different, then we've probably
    // shrunk (e.g. reading PC-format tables).  Truncate the table.
    // If they're the same, there's no reason to touch it.
    if (rptr != wptr) {
      *wptr = 0;
    }

    // Protocol 0 definitely has \n
    if (OpalMsgExt_PYTHON_NO_NUMERIC == serialKind_ ||
	OpalMsgExt_PYTHON_WITH_NUMERIC == serialKind_ ||
	OpalMsgExt_PYTHON_WITH_NUMPY == serialKind_) {
      // New: uses rewritten opalloader
      PickleLoaderImpl<OpalValue> pp(buffer, msglen);

      // Only support Numeric if requested: this will turn them into 
      // Lists
      OpalValue& ov = pp.env();
      EXTRACT_DICT(ot, ov);
      DICT_PUT(ot, "supportsNumeric", 
	       serialKind_==OpalMsgExt_PYTHON_WITH_NUMERIC); // true for Numeric
      
      // TODO: should we do this with NumPy? Probably not?
      
      OpalValue tbl_holder;
      pp.loads(tbl_holder);
      tbl = UnOpalize(tbl_holder, OpalTable);
      
    } else if (OpalMsgExt_PYTHON_OLD_NO_NUMERIC == serialKind_ ||
	       OpalMsgExt_PYTHON_OLD_WITH_NUMERIC == serialKind_) {
      bool supports_numeric = (OpalMsgExt_PYTHON_WITH_NUMERIC == serialKind_);
      PythonBufferDepickler<OpalValue> 
	pdp(msglen, buffer, supports_numeric);
      tbl = pdp.load();
    } else if (OpalMsgExt_ASCII == serialKind_) {
      tbl = UnStringize(buffer, OpalTable);
    } else {
      tbl.deserialize(buffer);
    }
  }
  
  // Binary protocols:  The P2 does have some \n and \cr in there ..
  // could that be problematic?  I think Python handles it for us,
  // so it shouldn't be a problem
  else if (OpalMsgExt_PYTHON_P2 == serialKind_              || 
	   OpalMsgExt_PYTHON_P2_WITH_ARRAY == serialKind_   ||
	   OpalMsgExt_PYTHON_P2_WITH_NUMERIC == serialKind_ ||
	   OpalMsgExt_PYTHON_P2_WITH_NUMPY == serialKind_) {
    // New: uses rewritten opalloader
    PickleLoaderImpl<OpalValue> pp(buffer, msglen);
    
    // Only support Numeric if requested: this will turn them into 
    // Lists
    OpalValue& ov = pp.env();
    EXTRACT_DICT(ot, ov);
    DICT_PUT(ot, "supportsNumeric", 
	     serialKind_==OpalMsgExt_PYTHON_WITH_NUMERIC); // true for Numeric
    
    // TODO: should we do this with NumPy? Probably not?
    
    OpalValue tbl_holder;
    pp.loads(tbl_holder);
    tbl = UnOpalize(tbl_holder, OpalTable);

  } else if (OpalMsgExt_PYTHON_P2_OLD == serialKind_          ||
	     OpalMsgExt_PYTHON_P2_OLD_WITH_NUMERIC == serialKind_) {
    OpalValue ov;
    P2TopLevelLoadOpal(ov, buffer);  // Load handles all weird options
    tbl = ov; // thank you ref counts!
  } else if (OpalMsgExt_OPENCONTAINERS == serialKind_) {
    OpalValue ov;
    // cerr << "About to call Deserialize for OC" << endl;
    Deserialize(ov, buffer);
    // cerr << "All done with call to Deserialize for OC" << endl;
    tbl = ov; // thank you ref counts!
  } else {
    IMemStream imem(buffer, msglen);
    imem.setStreamDataEncoding(sde);
    imem >> tbl;
  }
  // cerr << "Got the table:" << tbl << endl;
  return tbl;
}


// This is a helper routine used by "guess":
// This is all "cut-and-paste" code from BStream::readStreamDataEncoding.
// Because it is baseline code, we can't really change it, so yes,
// this is brittle and probably should be a method of the BStream
// if we do put this functionality in the baseline.
void OpalMsgExtNetHdr::readContinueStreamDataEncoding_ (BStream& bin,
							StreamDataEncoding& sde,
							char* rep)
{
  // Prior to M2k version 3.0.6.0, the initial four bytes were the
  // string format of the machine rep, and that's all there was.
  // Determine whether we are using an extended-format encoding
  // message, in which case there is more stuff that we have to read.
  // If not, the test will at least record the correct machine
  // representation in the sde.
  int_u1* buf;
  Size len;
  if (sde.isExtendedFormat(rep, 4, buf, len)) {
    // We are using an extended format.  The length of the message
    // may change, but there's a prefix that is known to be the
    // same, and the SDE told us how long it is and where we
    // should put the rest of it.  Do so.
    bin.readExact(buf, len);

    // Now ask the SDE to look at the prefix, and determine how much
    // more information is needed to complete the SDE message.  Read
    // that in, then have the SDE initialize itself from the entire
    // message.
    sde.extendMessageFromPrefix(buf, len);
    bin.readExact(buf, len);
    sde.setFromMessage();
  } else {
    // Not using extended format; presumably, the other side is a version
    // of m2k that is pretty old.  Set the default version number to
    // Original.
    sde.defaultSerialFormatLatest(false);
  }
  return;
}



OpalTable OpalMsgExtNetHdr::getOpalMsg (BStream& bin)
{
  // cerr << "CALLING getOpalMsg" << endl;

  // If we try to guess, then we change the serialKind.
  if (serialKind_ == OpalMsgExt_ADAPTIVE) {
    return guessing_(bin);
    // Although we could get by with the guessing branch to 
    // do all the work, I want to keep the original "easy"
    // code (below) for showing how we think (the "guess" code
    // is messy, and its always good to have this to refer to)
  } 

  /// RTS // Always starts with message length
  /// RTS int_u4 msglen;
  /// RTS bin.readExact(&msglen, sizeof(msglen));
  /// RTS msglen = NetworkToNativeMachineRep(msglen);

  size_t msglen = getMessageLen(bin);

  // Binary?  Do extra work
  StreamDataEncoding sde;
  if (OpalMsgExt_BINARY == serialKind_) {
    bin.readStreamDataEncoding(sde);
  } else if (OpalMsgExt_PYTHON_NO_NUMERIC == serialKind_) {
    char hdr[4];
    bin.readExact(&hdr[0], 4);
    // Check for PY00
  } else if (OpalMsgExt_PYTHON_WITH_NUMERIC == serialKind_) {
    char hdr[4];
    bin.readExact(&hdr[0], 4);
    // Check for PYN0
  } else if (OpalMsgExt_PYTHON_P2 == serialKind_) {
    char hdr[4];
    bin.readExact(&hdr[0], 4);
    // Check for PY02
  } else if (OpalMsgExt_PYTHON_P2_WITH_ARRAY == serialKind_) {
    char hdr[4];
    bin.readExact(&hdr[0], 4);
    // Check for PYA2
  } else if (OpalMsgExt_PYTHON_P2_WITH_NUMERIC == serialKind_) {
    char hdr[4];
    bin.readExact(&hdr[0], 4);
    // Check for PYN2
  } else if (OpalMsgExt_PYTHON_WITH_NUMPY == serialKind_) {
    char hdr[4];
    bin.readExact(&hdr[0], 4);
    // Check for PYU0
  } else if (OpalMsgExt_PYTHON_P2_WITH_NUMPY == serialKind_) {
    char hdr[4];
    bin.readExact(&hdr[0], 4);
    // Check for PYU2
  } else if (OpalMsgExt_PYTHON_P2_OLD_WITH_NUMERIC == serialKind_) {
    char hdr[4];
    bin.readExact(&hdr[0], 4);
    // Check for PYN2
  } else if (OpalMsgExt_PYTHON_P2_OLD == serialKind_) {
    char hdr[4];
    bin.readExact(&hdr[0], 4);
    // Check for PYN2
  } else if (OpalMsgExt_OPENCONTAINERS == serialKind_) {
    char hdr[4];
    bin.readExact(&hdr[0], 4);
    // Check for OC00
  }

  // Allocate and read buffer
  ArrayPotHolder<char> buffer(new char[msglen + 1]);
  BinReadExact(bin, buffer.data(), msglen);
  buffer[msglen] = 0;

  return interpret_(msglen, buffer, sde);
}


void OpalMsgExtNetHdr::sendOpalMsg (BStream& bout, const OpalTable& tbl)
{
  OMemStream tmp;
  prepareOpalMsg(tmp, tbl);
  sendPreparedMsg(bout, tmp);
}



void OpalMsgExtNetHdr::prepareOpalMsg (OMemStream& mout, const OpalTable& tbl)
{
  if (serialKind_!=OpalMsgExt_ADAPTIVE && serialKind_ != OpalMsgExt_BINARY) {
    string str;
    if (serialKind_ == OpalMsgExt_PYTHON_WITH_NUMERIC ||
	serialKind_ == OpalMsgExt_PYTHON_NO_NUMERIC ||
	serialKind_ == OpalMsgExt_PYTHON_WITH_NUMPY ||
	serialKind_ == OpalMsgExt_PYTHON_OLD_WITH_NUMERIC ||
	serialKind_ == OpalMsgExt_PYTHON_OLD_NO_NUMERIC) {

      ArrayDisposition_e ad = AS_LIST;
      if (serialKind_ == OpalMsgExt_PYTHON_WITH_NUMERIC || 
	  serialKind_ == OpalMsgExt_PYTHON_OLD_WITH_NUMERIC) {
	ad = AS_NUMERIC;
      } else if (serialKind_ == OpalMsgExt_PYTHON_WITH_NUMPY) {
	ad = AS_NUMPY;
      }
      Array<char> a(1024);
      {
	//cerr << "************************ HERE0" << tbl << " Dis:"<< ad << endl;
	PythonBufferPickler<OpalValue> pbp(a, ad);
	pbp.dump(tbl);
      }
      //cout << "len:" << a.length() << " " << a << endl;
      mout.write(a.data(), a.length());
      return;
    } else if (serialKind_ == OpalMsgExt_PYTHON_P2 ||
	       serialKind_ == OpalMsgExt_PYTHON_P2_WITH_ARRAY ||
	       serialKind_ == OpalMsgExt_PYTHON_P2_WITH_NUMERIC ||
	       serialKind_ == OpalMsgExt_PYTHON_P2_WITH_NUMPY ||
	       serialKind_ == OpalMsgExt_PYTHON_P2_OLD ||
	       serialKind_ == OpalMsgExt_PYTHON_P2_OLD_WITH_NUMERIC) {
      // Figure out disposition
      ArrayDisposition_e dis = AS_LIST;
      if (serialKind_==OpalMsgExt_PYTHON_P2_WITH_ARRAY) {
	dis = AS_PYTHON_ARRAY;
      } else if (serialKind_==OpalMsgExt_PYTHON_P2_OLD_WITH_NUMERIC ||
		 serialKind_==OpalMsgExt_PYTHON_P2_WITH_NUMERIC) {
	dis = AS_NUMERIC;
      } else if (serialKind_==OpalMsgExt_PYTHON_P2_WITH_NUMPY) {
	dis = AS_NUMPY;
      }

      // Old Python talking to us?
      PicklingIssues_e issues = ABOVE_PYTHON_2_2;
      if (serialKind_==OpalMsgExt_PYTHON_P2_OLD_WITH_NUMERIC ||
	  serialKind_==OpalMsgExt_PYTHON_P2_OLD) {
	issues = AS_PYTHON_2_2;
      }

      //cerr << "************************ HERE" << tbl << " Dis:"<< dis << " Issues:" << issues << endl;
      size_t bytes = P2TopLevelBytesToDumpOpal(tbl, dis, issues);  
      Array<char> a(bytes);
      a.expandTo(bytes);
      char* start_mem = (char*)a.data();
      char* end_mem = P2TopLevelDumpOpal(tbl, start_mem, dis, issues);
      size_t actual_bytes = end_mem-start_mem;
      //cout << "bytes:" << actual_bytes << " abytes" << a.length() << a << endl;
      mout.write(a.data(), actual_bytes);
      return;

    } else if (serialKind_ == OpalMsgExt_ASCII) { 
      str = Stringize(tbl);
    } else if (serialKind_ == OpalMsgExt_OPENCONTAINERS) { 
      
      // OpenContainers: BytesToSerialize, then Serialize
      size_t bytes = BytesToSerialize(tbl);
      // cerr << "In OC Serialize, bytes=" << bytes << endl;
      Array<char> a(bytes);
      a.expandTo(bytes);
      char* start_mem = (char*)a.data();
      // cerr << "About to serialize" << endl;
      char* end_mem = Serialize(tbl,start_mem);
      // cerr << "serialized" << endl;
      size_t actual_bytes = end_mem - start_mem;
      // cerr << "sizeof(Size) == " << sizeof(Size) << endl;
      mout.write(a.data(), actual_bytes);
      return;

    } else {
      str = tbl.serialize();
    }
    mout.write(str.c_str(), str.length());
  } else { // Adaptive defaults to BINARY if never set
    mout << tbl;
  }
}

// Write either 4 bytes (normal messgae) or 12-bytes:
// ESCAPE 0xFFFFFFFF + 8 bytes of real_len
inline void sendLength (BStream& bout, const OMemStream& m)
{
  size_t len = m.length();
  int_u4 msglen = htonl(m.length());
  if (len < 0xFFFFFFFF) {
    // Normal message len: 4 bytes
    bout.write(&msglen, sizeof(msglen));
  } else {
    // Abnormally large: need 4 bytes (esp), then 8 bytes for real length
    msglen = htonl(int_u4(0xFFFFFFFF)); // ESCape for bigger bytes coming
    bout.write(&msglen, sizeof(msglen));
    int_u8 msglen8 = len;
    int_u8 msglenNBO8 = NetworkToNativeMachineRep(msglen8);
    bout.write(& msglenNBO8, sizeof(msglenNBO8));
  } 
}


void OpalMsgExtNetHdr::sendPreparedMsg (BStream& bout, const OMemStream& m)
{
  //int_4 msglen = htonl(m.length());
  //bout.write(&msglen, sizeof(msglen));
  sendLength(bout, m);

  if (serialKind_ == OpalMsgExt_BINARY) {
    StreamDataEncoding sde;
    OpalValue::setStreamDataEncodingVersions(sde);
    bout.writeStreamDataEncoding(sde);
  } else if (serialKind_ == OpalMsgExt_PYTHON_NO_NUMERIC) {
    char hdr[4] = { 'P', 'Y', '0', '0' };
    bout.write(&hdr[0], 4);
  } else if (serialKind_ == OpalMsgExt_PYTHON_WITH_NUMERIC) {
    char hdr[4] = { 'P', 'Y', 'N', '0' };
    bout.write(&hdr[0], 4);
  } else if (serialKind_ == OpalMsgExt_PYTHON_WITH_NUMPY) {
    char hdr[4] = { 'P', 'Y', 'U', '0' };
    bout.write(&hdr[0], 4);
  } else if (serialKind_ == OpalMsgExt_PYTHON_P2) {
    char hdr[4] = { 'P', 'Y', '0', '2' };
    bout.write(&hdr[0], 4);
  } else if (serialKind_ == OpalMsgExt_PYTHON_P2_WITH_ARRAY) {
    char hdr[4] = { 'P', 'Y', 'A', '2' };
    bout.write(&hdr[0], 4);
  } else if (serialKind_ == OpalMsgExt_PYTHON_P2_WITH_NUMERIC) {
    char hdr[4] = { 'P', 'Y', 'N', '2' };
    bout.write(&hdr[0], 4);
  } else if (serialKind_ == OpalMsgExt_PYTHON_P2_WITH_NUMPY) {
    char hdr[4] = { 'P', 'Y', 'U', '2' };
    bout.write(&hdr[0], 4);
  } else if (serialKind_ == OpalMsgExt_PYTHON_P2_OLD) {
    char hdr[4] = { 'P', 'Y', '0', '-' };
    bout.write(&hdr[0], 4);
  } else if (serialKind_ == OpalMsgExt_PYTHON_P2_OLD_WITH_NUMERIC) {
    char hdr[4] = { 'P', 'Y', 'N', '2' };
    bout.write(&hdr[0], 4);
  } else if (serialKind_ == OpalMsgExt_OPENCONTAINERS) {
    char hdr[4] = { 'O', 'C', '0', '0' };
    bout.write(&hdr[0], 4);
  }

  // bout.write(m.data(), m.length());
  BinWrite(bout, (char*)m.data(), m.length());
}





