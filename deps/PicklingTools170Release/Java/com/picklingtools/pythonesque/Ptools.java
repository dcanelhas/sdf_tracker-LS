
package com.picklingtools.pythonesque;

import java.io.*;
import java.text.ParseException;

import com.picklingtools.pythonesque.Arr;
import com.picklingtools.pythonesque.Complex16Array;
import com.picklingtools.pythonesque.Complex8Array;
import com.picklingtools.pythonesque.Tab;

// A collection of useful static functions for Tab, Arr, etc.
public class Ptools {

    // Write out the given Tab, Arr, etc. as a 
    // Python dict, list, etc. to a text file
    public static void WritePythonTextFile (String filename, Object v) 
	throws FileNotFoundException, UnsupportedEncodingException
    {
	File f = new File(filename);
	PrintStream  ps = new PrintStream(f, "utf-8");
	Ptools.prettyPrint(ps, v);
    }

    // Read the given Tab, Arr, etc. from a  
    // Python ASCII text file
    public static Object ReadPythonTextFile (String filename) 
	throws FileNotFoundException, UnsupportedEncodingException, 
	       IOException, ParseException
    {
	FileReader fr = new FileReader(filename);
	PythonStreamReader psr = new PythonStreamReader(fr, true);
	Object o = psr.expectAnything();
	return o;
	
    }


    // Pretty print an object "like Python", if that makes sense.
    public static void prettyPrint(PrintStream os, Object obj) {
	if (obj==null) { 
	    os.print("None");  
	    return; 
	}
	@SuppressWarnings("rawtypes")
	Class c = obj.getClass();
	String ct = c.getName();
	if (ct.length()>0 && ct.charAt(0)=='[') {
	    // Array!
	    printArray(os, obj);
	    return;
	} else if (obj instanceof Tab) {
	    Tab t = (Tab)obj;
	    t.prettyPrint(os);
	    return;
	} else if (obj instanceof Arr) {
	    Arr a = (Arr)obj;
	    a.prettyPrint(os);
	    return;
	} else if (obj instanceof Float) {
	    Float temp = (Float)obj;
	    float fobj = temp;
	    if (fobj==Float.POSITIVE_INFINITY) {
		os.print("inf");
	    } else if (fobj==Float.NEGATIVE_INFINITY) {
		os.print("-inf");
	    } else if (temp.isNaN()) {
		os.print("nan");
	    } else {
		os.print(fobj);
	    }
	} else if (obj instanceof Double) {
	    Double temp = (Double)obj;
	    double dobj = temp;
	    if (dobj==Double.POSITIVE_INFINITY) {
		os.print("inf");
	    } else if (dobj==Double.NEGATIVE_INFINITY) {
		os.print("-inf");
	    } else if (temp.isNaN()) {
		os.print("nan");
	    } else {
		os.print(dobj);
	    }
	} else if (obj instanceof Boolean) {
	    Boolean b = (Boolean)obj;
	    if (b) { 
		os.print("False"); 
	    } else {
		os.print("True");
	    }
	} else if (obj instanceof String) { 
	    String s = (String)obj;
	    String ss = StringTools.PyImage(s);
	    os.print(ss);
	} else {
	    os.print(obj);
	}
    }


    public static void printArray(PrintStream os, Object obj) {
	if (obj instanceof byte[]) {
	    byte[] a = (byte[])obj;
	    os.print("array('b',[");
	    for (int ii=0; ii<a.length; ii++) {
		os.print(a[ii]);
		if (ii!=a.length-1) {
		    os.print(",");
		} else {
		    os.print("])");
		}
	    }
	} else if (obj instanceof char[]) {
	    char[] a = (char[])obj;
	    os.print("array('c', [");
	    for (int ii=0; ii<a.length; ii++) {
		os.print(a[ii]);
		if (ii!=a.length-1) {
		    os.print(",");
		} else {
		    os.print("])");
		}
	    }
	} else if (obj instanceof short[]) {
	    short[] a = (short[])obj;
	    os.print("array('h',[");
	    for (int ii=0; ii<a.length; ii++) {
		os.print(a[ii]);
		if (ii!=a.length-1) {
		    os.print(",");
		} else {
		    os.print("])");
		}
	    }
	} else if (obj instanceof int[]) {
	    int[] a = (int[])obj;
	    os.print("array('i',[");
	    for (int ii=0; ii<a.length; ii++) {
		os.print(a[ii]);
		if (ii!=a.length-1) {
		    os.print(",");
		} else {
		    os.print("])");
		}
	    }
	} else if (obj instanceof long[]) {
	    long[] a = (long[])obj;
	    os.print("array('l',[");
	    for (int ii=0; ii<a.length; ii++) {
		os.print(a[ii]);
		if (ii!=a.length-1) {
		    os.print(",");
		} else {
		    os.print("])");
		}
	    }
	} else if (obj instanceof float[]) {
	    float[] a = (float[])obj;
	    os.print("array('f',[");
	    for (int ii=0; ii<a.length; ii++) {
		os.print(a[ii]);
		if (ii!=a.length-1) {
		    os.print(",");
		} else {
		    os.print("])");
		}
	    }
	} else if (obj instanceof double[]) {
	    double[] a = (double[])obj;
	    os.print("array('d',[");
	    for (int ii=0; ii<a.length; ii++) {
		os.print(a[ii]);
		if (ii!=a.length-1) {
		    os.print(",");
		} else {
		    os.print("])");
		}
	    }

	} else if (obj instanceof Complex8Array) {
	    Complex8Array c8a = (Complex8Array)obj;
	    float[] a = (float[])c8a.data_;
	    os.print("array('f',[");
	    for (int ii=0; ii<a.length; ii++) {
		os.print(a[ii]);
		if (ii!=a.length-1) {
		    os.print(",");
		} else {
		    os.print("])");
		}
	    }
	} else if (obj instanceof Complex16Array) {
	    Complex16Array c16a = (Complex16Array)obj;
	    double[] a = (double[])c16a.data_;
	    os.print("array('d',[");
	    for (int ii=0; ii<a.length; ii++) {
		os.print(a[ii]);
		if (ii!=a.length-1) {
		    os.print(",");
		} else {
		    os.print("])");
		}
	    }
	} else {
	    os.print(obj);
	}
    }

    // Helper function for Tab and Arr
    static void indentOut_ (PrintStream os, int amt) {
	StringBuffer s = new StringBuffer(amt);
	for (int ii=0; ii<amt; ii++) {
	    s.append(' ');
	}
	os.print(s.toString());
    }


    // Helper: backend used for cascading lookups for both Arr and Tab
    static protected Object getBackend_(Object lookup, Object keys[]) 
	    throws ClassCastException {
    	for (Object obj : keys) {
    	    if (lookup instanceof Tab) {
    		Tab nested = (Tab) lookup;
    		lookup = nested.get(obj);
    	    } else if (lookup instanceof Arr) {
    		Arr nested = (Arr) lookup;
		int index = -1;
		if (obj instanceof Integer) {
		    index = (Integer)obj;
		} else if (obj instanceof String) {
		    String s = (String)obj;
		    index = Integer.parseInt(s);
		} else {
		    throw new ClassCastException();
		}
    		lookup = nested.get(index);
    	    } else {
    		throw new ClassCastException();
    	    }
	    
    	}
    	return lookup;
    }


    // Helper: Does all the backend work for the cascading inserts: here, it can
    // be shared between Tab and Arr
    static protected Object putBackend_(Object lookup, Object keys[]) 
	throws ClassCastException {
	// Assertion: at least 2 entries in keys
	for (int ii=0; ii<keys.length-2; ii++) {
	    Object obj = keys[ii];
	    
	    if (!(obj instanceof Integer || obj instanceof String)) {
		throw new ClassCastException("Can only have Integers and Strings for keys to Tabs and Arrs");
	    }
    	    if (lookup instanceof Tab) {
    		Tab nested = (Tab) lookup;
    		lookup = nested.get(obj);
    	    } else if (lookup instanceof Arr) {
    		Arr nested = (Arr) lookup;
		int index = -1;
		if (obj instanceof Integer) {
		    index = (Integer)obj;
		} else if (obj instanceof String) {
		    String s = (String)obj;
		    index = Integer.parseInt(s);
		} else {
		    throw new ClassCastException();
		}
    		lookup = nested.get(index);
    	    } else {
    		throw new ClassCastException();
    	    }
    	}
	// Assertion: lookup contains container, try to insert
	// into it
	Object key   = keys[keys.length-2];
	Object value = keys[keys.length-1];
	if (lookup instanceof Tab) {
	    Tab t = (Tab)lookup;
	    return t.put(key, value);
	} else if (lookup instanceof Arr) {
	    Arr a = (Arr)lookup;
	    int index = -1;
	    if (key instanceof Integer) {
		index = (Integer)key;
	    } else if (key instanceof String) {
		String s = (String)key;
		index = Integer.parseInt(s);
	    } else {
		throw new ClassCastException();
	    }
	    return a.set(index, value);
	}
	return null;
    }

    

}; 