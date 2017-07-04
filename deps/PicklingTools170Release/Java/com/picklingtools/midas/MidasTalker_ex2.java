
// Longer example showing how to handle misconnects, clients going away,
// etc.

package com.picklingtools.midas;


import java.io.IOException;
import java.net.ConnectException;
import java.net.UnknownHostException;
import java.text.ParseException;

import com.picklingtools.pythonesque.Tab;
import com.picklingtools.midas.MidasSocket_.*;  // get all the nested enums

public class MidasTalker_ex2 {

    // Main loop where you send and receive messages to the MidasTalker mt
    public static void sendloop (MidasTalker mt) throws IOException {
	while (true) {

	    // Create a message (a table) and send it to the server
	    // (Server may be OpalPythonDaemon, OpalDaemon, or 
	    // MidasServer (C++ or Python)
	    try {
		Tab shared = new Tab("{ 'shared': 1 }");
		Tab t=new Tab("{'a':1,'hello':[3,3,4.4,5.5],'nest':{'cc':17}}");
		t.put("proxy1", shared);
		t.put("proxy2", shared);  // shared data structure

		t.prettyPrint(System.out);
		mt.send(t);

		// See if we can receive, wait 5 seconds
		while (true) {
		    Object res = mt.recv(5.0);
		    if (res == null) {
			System.out.println("...retrying to receive after 5 seconds...");
			// Maybe try some other work
			continue;
		    } else {
			// "Do something" with result
			Tab r = (Tab)res;
			System.out.println( "Got result:"+res);
			if (r.get("proxy1") != r.get("proxy2")) {
			    System.out.println("NOT Same"); 
			}
			break;
		    }
		}
	    } catch (ParseException pe) { 
		System.out.println("?? Illegal table?? "+pe);
	    }
	}
	    
    }

    public static void softsleep(double seconds) {
	try {
	    int milliseconds = (int)(seconds * 1000.0);
	    java.lang.Thread.sleep(milliseconds);
	} catch (InterruptedException ex) {
	    ;
	}
    }

    public static void openup(MidasTalker mt) {

	// Open the socket, with a retry in case it's not there right away
	while (true) {
	    try {
		mt.open();
		System.out.println("Opened connection to host:"+mt.host()+" port:"+mt.port());
		break;
	    } catch (UnknownHostException e) {
		System.out.println("Had trouble with name: Did you mistype the hostname?"+e);	    
		System.out.println("...couldn't open right away, backing off 5 seconds");
		softsleep(5.0);
	    } catch (ConnectException e) {
		System.out.println("Couldn't connect: Is server up?"+e);     
		System.out.println("...couldn't open right away, backing off 5 seconds");
		softsleep(5.0);

	    } catch (Exception e) { // generic problem
		System.out.println(e.getClass().getName());
		System.out.println("...couldn't open right away, backing off 5 seconds");
		softsleep(5.0);
		System.out.println(e);
	    }
	}
    }

    public static void main(String [] args) throws IOException {

	// Pass in args
	int len = args.length;
	if (len>5 || len<2 ) {
	    System.out.println("Usage: MidasTalker_ex2 hostname portnumber [ser sock arrd]");
	    System.exit(1);
	}

	String host = args[0];
	int port    = Integer.parseInt(args[1]);
	Serialization ser= len<3 ? Serialization.SERIALIZE_P2 : 
	    Serialization.get(Integer.parseInt(args[2]));
	SocketDuplex sock=len<4 ? SocketDuplex.DUAL_SOCKET : 
	    SocketDuplex.get(Integer.parseInt(args[3]));
	ArrayDisposition arrd=len<5 ? ArrayDisposition.AS_LIST : 
	    ArrayDisposition.get(Integer.parseInt(args[4]));
        	// MidasSocket_.AS_PYTHON_ARRAY is the other option

	System.out.println("Host:"+host+" port:"+port+" ser:"+ser+" sock:"+sock+" arrd:"+arrd);

	// Create, but don't actually connect
	MidasTalker mt = new MidasTalker(host, port,ser, sock, arrd);

	// Keep trying to reconnect and work
	while (true) {
	    openup(mt);
	    try {
		sendloop(mt);
	    } catch (Exception e) {
		System.out.println("Problem:"+e);
		System.out.println("Server appears to have gone away? Attempting to reconnect");
	    }
	}
	
    }

};