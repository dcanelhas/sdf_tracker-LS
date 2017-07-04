
package com.picklingtools.midas;

// A simple example showing how to use the MidasTalker with all
// sorts of data in Java.

import java.io.IOException;
import java.text.ParseException;

import com.picklingtools.pythonesque.Arr;
import com.picklingtools.pythonesque.Tab;
import com.picklingtools.midas.MidasSocket_.*;

public class MidasTalker_ex {
    public static void main(String [] args) throws IOException {

	// Pass in args
	int len = args.length;
	if (len>5 || len<2) {
	    System.out.println("Usage: MidasTalker_ex hostname portnumber [ser sock arrd]");
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
	try {
	    // Create and open a new MidasTalker
	    MidasTalker mt = new MidasTalker(host, port,
					     ser, sock, arrd);
	    mt.open();
	    	    
	    // Python dict
	    Tab map=new Tab("{'A':1, 'timedelta': 0.7834578923, 'mode':'tdoa',"+
			    "'empty':None, 'valid':True, 'invalid': False, "+
			    "'byte':64, 'char':64 }");
	    
	    int ia[] = { 215789578, 222, 333 };
	    map.put("arr_int", ia);
	    
	    short iss[] = { 32767, 222, 333 };
	    map.put("arr_short", iss);

	    long lss[] = { 1111111111, 222, 333 };
	    map.put("arr_long", lss);
	    
	    // C++ can't handle byte data (as it uses unicode in encoding)
	    //byte bss[] = { 127, -127, 0 };
	    //map.put("arr_byte", bss);
	    
	    char css[] = { 'A', '!', '0' };
	    map.put("arr_char", css);

	    float fss[] = { 1.1f, 2.2f, 3.3f };
	    map.put("arr_float", fss);

	    double dss[] = { 1.1, 2.2, 3.3 };
	    map.put("arr_double", dss);
	    
	    
	    // Python list
	    Arr a=new Arr("[1, 2.1, 'three']");
	    
	    // Nested dicts and lists
	    map.put("nest", new Tab("{'stuff':1, 'junk':1.2}"));
	    map.put("a", a);
	    
	    mt.send(map);
	    try {
		//Thread.sleep(10000);
	    } catch (Exception e) { }

	    Object k = mt.recv();
	    System.out.println("RECEIVED:"+k);
	    
	    // Long way to get out a nested key
	    Tab dict = (Tab)k;
	    if (dict.containsKey("CONTENTS")) {
		dict = (Tab)dict.get("CONTENTS");
	    }
	    Tab nest1 = (Tab)dict.get("nest");
	    System.out.println(nest1);
	    
	    int i = (Integer)nest1.get("stuff");
	    System.out.println(i);
	    

	    // .. better way: use get! avoids excessive user coersions
	    System.out.println(dict.get("nest", "stuff"));

	    // Keep receiving
	    while (true) {
		Object o = mt.recv(5.0);
		if (o==null) {
		    System.out.println("Nothing to receive");
		} else if (o instanceof Tab) {
		    Tab t = (Tab)o;
		    t.prettyPrint(System.out);
		} else {
		    break;
		}
	    }
	    mt.close();

	} 
	catch (IOException e) {
	    System.out.println(e);
	}
	catch (ParseException e) {
	    System.out.println(e);
	}
	
    }

};