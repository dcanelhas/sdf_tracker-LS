
// MidasSocket encapsulates most of the Socket work that the
// MidasServer and MidasTalker do.  TODO: Write MidasServer
package com.picklingtools.midas;


import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;

import com.picklingtools.pickle.Pickler;
import com.picklingtools.pickle.Unpickler;


// A Helper class for MidasTalker/MidasServer 
public class MidasSocket_ {

    // Different types of Python serialization: Notice that this is
    // reasonably backwards compatible with previous releases, and 0 and 1
    // still correspond to "Pickling Protocol 0" and "No serialization".
    // Now, the int 2 becomes "Pickling Protocol 2".
    //  public static final int SERIALIZE_P0   = 0;
    //  public static final int SERIALIZE_NONE = 1;
    //  public static final int SERIALIZE_P2   = 2;
    public static enum Serialization {
	SERIALIZE_P0(0),
	    SERIALIZE_NONE(1),
	    SERIALIZE_P2(2);
	
	private int code;
	private Serialization (int code) { this.code = code; }
	public int getCode() { return code; }
	public static Serialization get (int code) {
	    switch(code) {
	    case 0: return SERIALIZE_P0;
	    case 1: return SERIALIZE_NONE;
	    case 2: return SERIALIZE_P2;
	    }
	    return null;
	}
    };

    // Do you use two sockets or 1 socket for full duplex communication?
    // Some very old versions (VMWARE) only supported single duplex sockets,
    // and so full duplex sockets had to be emulated with 2 sockets.
    // The single/dual socket protocols imply extra data sending:
    // we also support "normal sockets" (which just connects a single
    // full duplex socket, like HTTP uses). (Normal socket implies you
    // DO NOT send out a preamble indentifying it:  Single socket
    // implies you send out a preamble of "SNGL"*4, Double socket
    // implies you send out a preamble of "DUAL"*4)
    //    public static final int SINGLE_SOCKET = 0;
    //    public static final int DUAL_SOCKET   = 1;
    //    public static final int NORMAL_SOCKET = 777;
    public static enum SocketDuplex {
	SINGLE_SOCKET(0),
	DUAL_SOCKET(1),
	NORMAL_SOCKET(777);
	
	private int code;
	private SocketDuplex(int code) { this.code = code; }
	public int getCode() { return code; }
	
	public static SocketDuplex get(int code) {
	    switch(code) {
	    case 0 : return SINGLE_SOCKET;
	    case 1 : return DUAL_SOCKET;
	    case 777 : return NORMAL_SOCKET;
	    }
	    return null;
	}
    };


    // Array Disposition:
    // Currently, only AS_LIST and AS_PYTHON_ARRAY are supported.
    // Even then, AS_PYTHON_ARRAY may not work as you cross from
    // Python 2.6 to 2.7 ... (it should, but be wary)
    // public static final int AS_NUMERIC = 0;
    // public static final int AS_LIST = 1;
    // public static final int AS_PYTHON_ARRAY = 2;
    // // public static final int AS_NUMPY = 4;
    public static enum ArrayDisposition {
	AS_NUMERIC(0),
	    AS_LIST(1),
	    AS_PYTHON_ARRAY(2);
	// AS_NUMPY(4)
	
	private int code;
	private ArrayDisposition (int code) { this.code = code; }
	public int getCode() { return code; }
	public static ArrayDisposition get (int code) {
	    switch(code) {
	    case 0: return AS_NUMERIC;
	    case 1: return AS_LIST;
	    case 2: return AS_PYTHON_ARRAY;
	    }
	    return null;
	}
    };
    

    // Centralized error handling place
    protected static String errout(String message) throws IOException {
	// System.out.println(message);
	throw new IOException(message);
	// return message;
    }

    protected Socket helpOpenSocket_(String host, int port, double timeout) throws UnknownHostException, IOException {
	Socket s = new Socket(host, port);
	// s.bind();
	return s;
	// TODO: Better error handling
    }
    
    // Helper blocking read method
    protected static void readExact_ (Socket fd, byte[] buff, int bytes, int reading_into) throws IOException {
	// Sometimes reads get broken up... it's very rare (or
	// impossible?)  to get an empty read on a blocking call.  If we
	// get too many of them, then it probably means the socket went
	// away.
	InputStream in = fd.getInputStream();
	int empty_retries = 1000;
	int offset = 0;
	int bytes_to_read  = bytes;
	while (bytes_to_read != 0) {
	    // raw read: no timeout checking
	    int r = in.read(buff, offset+reading_into, bytes_to_read);
	    if (r<0) errout("read"); // EOF   
	    // Instead of getting a SIGPIPE, when we ignore the SIGPIPE signal,
	    // a number of "empty retries" is equivalent to the SIGPIPE, but then
	    // at least we catch it.
	    if (r==0 && --empty_retries<0) errout("read: too many empty retries");
	    bytes_to_read -= r;
	    offset        += r;
	}
    }

    
    protected static void readExact_ (Socket fd, byte[] buff, int bytes) throws IOException {
	readExact_(fd, buff, bytes, 0);
    }


    // Blocking call to get next Val off of socket.
    protected Object recvBlocking_ (Socket fd) throws IOException {
	Object retval = null;
	
	// Preamble: number of bytes to read (doesn't include 4 byte hdr)
	byte[] first_four = new byte[4];
	readExact_(fd, first_four, 4);
	ByteBuffer bb = ByteBuffer.wrap(first_four);
	int bytes_to_read = bb.getInt();
	if (bytes_to_read==0) {
	    return retval;
	}
	
	// Do we have a header?
	int read_bytes = 0;
	byte[] hdr = new byte[4];
	readExact_(fd, hdr, 4);
	if (hdr[0]==(char)'P' && hdr[1]==(char)'Y' && 
	    (hdr[3]==(char)'0' || hdr[3]==(char)'2')) {
	    read_bytes = 0;
	} else {
	    // Assume it's just some raw data
	    read_bytes = 4;
	}

	byte[] buffer = new byte[bytes_to_read];
	System.arraycopy(hdr, 0, buffer, 0, read_bytes);
	readExact_(fd, buffer, bytes_to_read - read_bytes, read_bytes);

	retval = unpackageData_(buffer);
	return retval;
    }



    // Helper: convert int to 4 bytes for network send
    protected final byte[] struct_pack(int bytes_to_write)
    {
	byte[] bytes_hdr = new byte[4];
	bytes_hdr[0] = (byte)((bytes_to_write>>24) & 0x00ff);
	bytes_hdr[1] = (byte)((bytes_to_write>>16) & 0x00ff);
	bytes_hdr[2] = (byte)((bytes_to_write>>8) & 0x00ff);
	bytes_hdr[3] = (byte)((bytes_to_write>>0) & 0x00ff);
	//for (int ii=0; ii<bytes_hdr.length; ii++) {
	//    System.out.println(bytes_hdr[ii]);
	//}
	return bytes_hdr;
    }

    // Blocking call to send the val over the socket
    protected void sendBlocking_(Socket fd, Object val) throws IOException {
	byte[] data = packageData_(val);

	int bytes_to_write = data.length;
	byte[] bytes_to_send = struct_pack(bytes_to_write);
	writeExact_(fd, bytes_to_send, bytes_to_send.length);
	if (serialization_ != Serialization.SERIALIZE_NONE) {
	    writeExact_(fd, header_, header_.length);
	}
	writeExact_(fd, data, bytes_to_write);	

    }

    // raw data send out: user should never need to call
    protected static void writeExact_(Socket fd, byte[] buff, int bytes) throws IOException {
	OutputStream out = fd.getOutputStream();
	out.write(buff, 0, bytes);
    }
    

    // When we want to close a socket, some platforms don't seem
    // to respond well to "just" a close: it seems like a 
    // shutdown is the only portable way to make sure the socket dies.
    // Some people may spawn processes and prefer to use close only,
    // so we allow 'plain' close is people specify it, otherwise
    // we force a shutdown.  The default is TRUE.
    protected void closing_ (Socket fd) throws IOException {
	if (forceShutdownOnClose_) {
	    fd.shutdownInput();
	    fd.shutdownOutput();
	    fd.close();
	}
    }


    // Helper function to pack data (Pickled, raw, etc.).
    // and return the unpacked data
    protected byte[] packageData_(Object val) throws IOException {
	byte[] data;
	if (serialization_ == Serialization.SERIALIZE_NONE) {
	    data = (byte[])val;
	} else {
	    Pickler p = new Pickler(false);
	    data = p.dumps(val);
	}
        return data;
    }
    
    // Helper function to unpackage the data for the different
    // types of serialization (Pickling, raw data, etc.). The
    // packaging policy is dictated by the constructor.
    protected Object unpackageData_(byte[] packaged_data) throws IOException {
	Object retval = null;
	if (serialization_ == Serialization.SERIALIZE_NONE) {
	    retval = packaged_data;
	} else {
	    Unpickler u = new Unpickler();
	    Object o = u.loads(packaged_data);
	    retval = o;
	    u.close();
	}
	return retval;
    }
    
    // Create MidasSocket_ that either Python Pickles or sends strings
    // as is.  This also chooses whether or not to use Numeric or plain
    // Python Arrays in serialization. For backwards compat, AS_LIST is 1
    // (being true), AS_NUMERIC is 0 (being false) and the new
    // AS_PYTHON_ARRAY is 2.
    // 
    // Note that the Java version of the Midastalker and MidasServer
    // does NOT currently support ADAPTIVE serialization.  
    public MidasSocket_ (Serialization serialization, 
			 ArrayDisposition array_disposition) { 
	forceShutdownOnClose_ = true;
	serialization_ = serialization;
	arrayDisp_ = array_disposition; 

	if (serialization == Serialization.SERIALIZE_NONE) {
	    header_ = null;
	} else {
	    header_ = new byte[4];
	    header_[0] = 'P';
	    header_[1] = 'Y';	    
	    if (serialization==Serialization.SERIALIZE_P0) {
		System.out.println(
			    "Currently, the Java pickling tools package\n"+
			    "can understand Python Pickling 0, but can only\n"+
			    "speak Pickling 2.  We recommend using \n"+
			    "SERIALIZE_P2 instead so both sides use the same\n"+
			    "serialization.");
		header_[3] = '0';	    
	    } else if (serialization==Serialization.SERIALIZE_P2) {
		header_[3] = '2';	    
	    }
	    //if (arrayDisp_==ArrayDisposition.AS_NUMERIC) {
	    //	header_[2] = 'N';
	    if (arrayDisp_==ArrayDisposition.AS_LIST) {
		header_[2] = '0';
	    //} else if (arrayDisp_==ArrayDisposition.AS_NUMPY) {
	    //  header_[2] = 'U';
	    } else if (arrayDisp_==ArrayDisposition.AS_PYTHON_ARRAY) {
		header_[2] = 'A';
	    } else {
		System.out.println("Unknown array disposition... default to ArrayDisposition.AS_LIST");
		header_[2] = '0';
	    }
	}
    }

    protected boolean forceShutdownOnClose_; // Do we do close only or shutdown/close ?
    protected Serialization serialization_;  // None, P0 or P2?
    protected ArrayDisposition arrayDisp_;   // AS_LIST, AS_NUMPY, AS_NUMERIC
    protected byte[] header_;                // PY?0, PY?2

};
