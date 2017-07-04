
// A MidasTalker is a client which talks to a server using sockets.
// This module exists to allow a simplified interface to talk to a
// Midas system, whether talking to an M2k OpalPythonDaemon or the the
// MidasServer in either C++ or Python.

// Most problems when using the MidasTalker are due to socket problems
// (wrong host port or name, server unexpectedly goes away).  These can
// be caught with try/expect blocks.  See the example below.


// String host = "somehost"; 
// int port=9999;
// MidasTalker mt = new MidasTalker(host, port);
//
// try {
//      mt.open();
//      System.out.println("Opened connection to host:" + mt.host() + 
//                         " port:" + mt.port());
// } catch (IOException e) {
//      System.out.println("Problem: ", e);
//      System.out.println("...couldn't open right away");
// }
// try {
//      Object res = mt.recv(5.0);
//      if (res == null) {
//         System.out.println("...didn't get a receive after 5 seconds ...");
//         // Maybe try to do some other work
//      } else {
//         // Do something with the result
//         System.out.println("Got result", res);
//      }
// } catch (IOException e) {
//      System.out.println("Problem: ", e);
//      System.out.println("Server appears to have gone away? "+
//                         "Attempting to reconnect");
// }

package com.picklingtools.midas;

import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;

public class MidasTalker extends MidasSocket_ {
    
    // The MidasTalker class: a Java class for talking to a
    // MidasServer or an M2k OpalPythonDaemon.
    
    // Host is the name of the machine running the server that we wish
    // to connect to, port is the port number on that machine
    
    // Serialization should be one of:
    //   0: SERIALIZE_P0 (use Python Pickling Protocol0: slow but compatible)
    //   1: SERIALIZE_NONE (send data in strings as-is: good for RAW data),
    //   2: SERIALIZE_P2 (use Python Pickling Protocol2: fast but less compat)
    // We default to SERIALIZE_P0 for backwards compatibility, but
    // strongly urge users to use SERIALIZE_P2 for the speed.
    //
    // CURRENTLY:   SERIALIZE_P2 is the best option


    // dual_socket refers to how you communicate with the server: using one
    // bi-directional socket or two single directional sockets.
    //   0: SINGLE_SOCKET
    //   1: DUAL_SOCKET
    // The dual socket mode is a hack around an old TCP/IP stack problem
    // that has been irrelevant for years, but both X-Midas and M2k have to
    // support it.  We recommend using single_socket mode when you can, but
    // default to dual socket mode because that's what X-Midas and Midas 2k
    // default to. Note: A server either supports single or dual socket
    // mode: NOT BOTH.
    
    // array_disposition ONLY applies if you are using SERIALIZE_P0 or
    // SERIALIZE_P2: array_disposition describes how you expect to send/recv
    // binary arrays: as Python arrays (import array) or Numeric arrays
    // (import Numeric).  By default, the MidasTalker sends the data as
    // lists, but this isn't efficient as you lose the binary/contiguous
    // nature.  This option lets the constructor check to see if your
    // Python implementation can support your choice.  Choices are:
    //  0: AS_NUMERIC         (DEPRECATED: recommended if using XMPY< 4.0.0)
    //  1: AS_LIST            (default)
    //  2: AS_PYTHON_ARRAY    (recommended if using Python 2.5 or greater)
    //  3: AS_NUMERIC_WRAPPER (NOT CURRENTLY SUPPORTED)
    //  4: AS_NUMPY           (recommended!!)
    //
    // CURRENTLY: Only AS_LIST and AS_PYTHON_ARRAY are supported.

    // Note that both the Client (MidasTalker and MidasServer) need
    // to match on all 3 of these parameters or they WILL NOT TALK!
    
    // If using "adaptive" serialization (which by default all Java
    // Servers and Talkers do), then we keep track of what
    // serializations happen for each conversation and try to talk to
    // servers in the same serialization they talked to us.  When there
    // is a doubt as to what to talk, then the parameters below decide
    // that.
    public MidasTalker (String hostname, int port, 
			Serialization serialization,
			SocketDuplex dual_socket, 
			ArrayDisposition array_disposition) {
	super(serialization, array_disposition);
	this.host_          = hostname;
	this.port_          = port;
	this.serialization_ = serialization;
	this.dualSocket_    = dual_socket;

	s_ = new Socket[2];
	s_[0] = null;
	s_[1] = null;
    }
    
    // Clean up and close sockets
    public void cleanUp () {
	// When we clean up, make sure we clean up the conversation
	// parameters
	// readWriteDisassociate_(s_[0], s_[1]);
	
	// It's important that we immediately assign into the s[0] and
	// s[1] so we can track that resource in case some exceptional
	// activity takes place.
	Socket fd0 = s_[0]; 
	s_[0] = null;
	try {
	    if (fd0!=null) {
		closing_(fd0);
	    }
	} catch (IOException e) {
	    ;
	}
	
	Socket fd1 = s_[1];
	s_[1] = null;
	try {
	    if (fd0!=fd1 && fd1!=null) {
		closing_(fd1);
	    }
	} catch (IOException e) {
	    ;
	}
    }

    // Alias for cleanUp
    void close () { cleanUp(); }


    // With the host and the port set, open up the socket and connect.
    // During the MidasSocket/talker open negotations, some bytes are
    // sent back and forth:  if things aren't there right away, we allow
    // a timeout so you can recover from a failed open
    public void open (double timeout) throws UnknownHostException, IOException {
	cleanUp();
	s_[0] = helpOpenSocket_(host_, port_, timeout);

	// Single-socket or dual-socket or normal socket?
	if (dualSocket_ == SocketDuplex.NORMAL_SOCKET) {
	    s_[1] = s_[0];
	    // Nothing, just don't do crazy socket stuff
	} else if (dualSocket_ == SocketDuplex.SINGLE_SOCKET) {
	    s_[1] = s_[0]; // single socket, read/write same Socket
	    String m1 = nextBytes_(s_[0], timeout, 16);
	    clientExpectingSingleSocket_(m1);

	} else if (dualSocket_ == SocketDuplex.DUAL_SOCKET) {
	    s_[1] = helpOpenSocket_(host_, port_, timeout);

	    // Distinguish reader and writer
	    String m1 = nextBytes_(s_[0], timeout, 16);
	    clientExpectingDualSocket_(m1, "");
	    String m2 = nextBytes_(s_[1], timeout, 16);
	    if (m1.equals("SENDSENDSENDSEND") && m2.equals("RECVRECVRECVRECV")) {
		; 
	    } else if (m1.equals("RECVRECVRECVRECV") && m2.equals("SENDSENDSENDSEND")) {
		Socket temp = s_[0]; s_[0] = s_[1]; s_[1] = temp;
	    } else {
		clientExpectingDualSocket_(m1, "");
		// errout("Unknown header messages from sockets:'"+m1+"' '"+m2+"'");
	    }
	} else {
	    errout("Unknown socket protocol? Check your dual_socket param");
	}
	// readWriteAssocate_(s_[0], s_[1]);
    }

    // Blocking open until can open: no timeout
    public void open () throws UnknownHostException, IOException {
	open(0.0);
    }

    
    // Try to receive the result from the socket: If message is not
    // available, wait for timeout (m.n) seconds for something to be
    // available.  If no timeout is specified (the default), this is a
    // blocking call.  If timeout expires before the data is available,
    // then null is returned from the call, otherwise the val is
    // returned from the call.  A socket error can be thrown if the
    // socket goes away.
    public Object recv (double timeout) throws IOException { 
	Object result = null;
	try {
	    int milliseconds = (int)(timeout * 1000.0);
	    s_[0].setSoTimeout(milliseconds);
	    result = recvBlocking(); 
	} catch (SocketTimeoutException e) {
	    result = null;
	};
	return result;
    }
    public Object recv () throws IOException { 
	return recv(0.0);
    }

    // Blocking call to recv
    public Object recvBlocking () throws IOException 
    { return recvBlocking_(s_[0]); }

    // Try to send a value over the socket: TODO: If the message can't be sent
    // immediately, wait for timeout (m.n) seconds for the the socket to
    // be available.  If the timeout is None (the default), then this is
    // a blocking call.  If the timeout expires before we can send, then
    // None is returned from the call and nothing is queued, otherwise
    // true is returned.
    public boolean send (Object mesg, double timeout_in_seconds) throws IOException {
	sendBlocking_(s_[1], mesg);
	return true;
    }

    public boolean send (Object mesg) throws IOException {
	send(mesg, 0.0);
	return true;
    }
    
    // Blocking call to send Val over socket.
    void sendBlocking (Object s) throws IOException { sendBlocking_(s_[1], s); }
    
    
    public String host () { return host_; }
    public int    port () { return port_; }

    // Helper routine: Read exactly the next bytes from the fd and plop
    // them into a String
    protected String nextBytes_ (Socket fd, double timeout_in_seconds, int bytes) throws IOException {
	// TODO: handle timeout

	// Some data, grab it
	byte[] buff = new byte[bytes];
	MidasSocket_.readExact_(fd, buff, bytes);
	String retval;
	try {
	    retval = new String(buff, "UTF8"); // TODO: Revisit?
	} catch (UnsupportedEncodingException e) {
	    retval = "";
	}
	return retval;
    }

    // Check and see that we are expecting DUAL_SOCKET correctly:
    // throw an exception with a good error message if they don't match,
    // otherwise return ok
    protected void clientExpectingDualSocket_ (String m1, String m2) throws UnknownHostException, IOException {
	String mesg;
	if (!m1.equals("SENDSENDSENDSEND") && !m1.equals("RECVRECVRECVRECV")) {
	    if (m1.equals("SNGLSNGLSNGLSNGL")) {
		mesg = 
		    "Your client is configured as DUAL_SOCKET, but your server\n"+
		    "is configured as SINGLE_SOCKET.  You need to set them both to\n"+
		    "match, then restart both sides (if you change just the client,\n"+
		    "you may only need to restart the client).\n";
	    } else {
		mesg = 
		    "Something is wrong: the client is set-up as DUAL_SOCKET\n" +
		    "but we are getting data which suggests your host is set-up as\n" +
		    "NORMAL_SOCKET, or maybe even a port that doesn't use the\n" +
		    "Midastalker/Server/OpalPythonDaemon protocols!\n" +
		    "Recheck your client and server socket type: \n" +
		    "   (DUAL_SOCKET, SINGLE_SOCKET, NORMAL_SOCKET)\n" +
		    "and make sure both client and server match (also check port #).\n";
	    }
	    errout(mesg);
	}
	// The preamble is one of the two expected, what if both are same?
	if (m1.equals(m2)) {
	    mesg = 
		"Congratulations!  You have found the DUAL_SOCKET race condition!\n" +
		"It's an innate problem that occurs fairly rarely.  Your sockets\n" +
		"are messed up and you need to reset both sides.  If this error\n" +
		"message occurs constantly, you may need to go to SINGLE_SOCKET\n" +
		"or NORMAL_SOCKET on both sides (if you interface with old systems,\n" +
		"SINGLE_SOCKET is your only choice)\n";
	    errout(mesg);
	}
	// If make it here, connection is okay
    }

    // Check and see that we are expecting SINGLE_SOCKET correctly:
    // throw an exception with a good error message if they don't match,
    // otherwise return ok
    protected void clientExpectingSingleSocket_ (String m1) throws UnknownHostException, IOException {
	String mesg;
	if (m1.equals("SENDSENDSENDSEND") || m1.equals("RECVRECVRECVRECV")) {
	    mesg = 
		"Your server seems to be in DUAL_SOCKET mode, and your client\n" +
		"is in SINGLE_SOCKET mode.  You need to reconfigure both client\n" +
		"and server to match and RESTART both sides (you may need to only\n" +
		"restart your client side if you set this client to DUAL_SOCKET)\n"; 
	    errout(mesg);
	} else if (!m1.equals("SNGLSNGLSNGLSNGL")) {
	    mesg = 
		"Something is wrong: the client is set-up as SINGLE_SOCKET\n" +
		"but we are getting data which suggests your host is set-up as\n" +
		"NORMAL_SOCKET, or maybe even a port that doesn't use the\n" +
		"Midastalker/Server/OpalPythonDaemon protocols!\n" +
		"Recheck your client and server socket type: \n" +
		"   (DUAL_SOCKET, SINGLE_SOCKET, NORMAL_SOCKET)\n" +
		"and make sure both client and server match (also check port #).\n";
	    errout(mesg);
	} else {
	    // Okay
	}
    }

    // Data members
    private String          host_;
    private int             port_;
    private SocketDuplex    dualSocket_;
    private Socket          s_[];
}; 

