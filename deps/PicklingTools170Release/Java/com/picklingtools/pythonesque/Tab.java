

package com.picklingtools.pythonesque;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.PrintStream;
import java.text.ParseException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;

// An abstract class which implements a more Python-esque dictionary.
// Java is a very different language from Python for implementing 
// a dynamic, heterogeneous, recursive dictionaries.  The new
// features of Java (unboxing for primitive types, overloading)
// make this a little easier to handle.  A Tab is just an "extension"
// of HashMap<String, Object>, so a Tab "is-a" HashMap and supports all
// HashMap operations, but it also adds some extra interface to 
// support dynamic dicts better.   The examples below works just like
// a HashMap normally, i.e., this is all standard Java:
//
//   HashMap<String, Object> t = new HashMap<String, Object>();
//   t.put("a", 1);              // with boxing, puts 1 as Integer into table
//   t.put("somefloat", 2.2);    // with boxing, puts 2.2 as Double
//   HashMap<String, Object> nested = new HashMap<String, Object>();
//   nested.put("oo", "str"); 
//   t.put("nest", nested);      // nested empty HashMap<String, Object>
//  
// Getting stuff out uses the standard get routines, and coupled with
// unboxing makes it easier to get tables out.  
// It would be nice if below worked:
// 
//   int i = (int)t.get("a");  // SYNTAX ERROR: Too much unboxing to do!
//
// So we have to content ourselves with casting to the object Integer
// and let Java unbox the rest for us:
//
//   int i = (Integer)t.get("a");  // WORKS! Unboxes Integer to int for us
// 
// All of the above, the HashMap handles by itself and is standard Java.
// The Tab gives us some extra features on top of the HashMap:
//
// (0) Less typing.  
//
//       HashMap<String, Object> m = new HashMap<String, Object>();      
//       Tab t = new Tab(); 
// 
// (1) The constructor supports creating a literal from a string, i.e,
//
//       Tab t = new Tab("{'a':1, 'somefloat':2.2, 'nest':{'oo':'str'} }");
//         // Constructs same table as above
//
//     This is exactly the syntax of dictionary literals in Python, so these 
//     can be cut-and-pasted between Python and Java.
//   
// (2) Supports print and prettyPrint that looks EXACTLY like Python 
//     dictionaries so you can cut and paste Python dicts between Python 
//     and Java
//
//       Tab t = new Tab("{'a':1, 'somefloat':2.2, 'nest':{'oo':'str'} }");
//
//       // Shows nested structure well
//       t.prettyPrint(System.out);
//         --> results in
//         { 
//            'a': 1,
//            'somefloat': 2.2,
//            'nest' : {
//               'oo': 'str'
//            }
//         }
//
//       // Show all on one line 
//       System.out.println(t);     // prints on 1 line w/out nesting, newlines
//        --> results in
//         {'a':1,'somefloat':2.2,'nest':{'oo':'str'}}
//
// (3) Support for cascading lookup
//
//       Tab t = new Tab("{'a':1, 'somefloat':2.2, 'nest':{'oo':'str'} }");
//       String s = (String)t.get("nest", "oo");
//
// (4) Support for cascading insertion
// 
//      Tab t = new Tab("{'nest':{ 'nest2':{} }");
//      t.put("nest", "nest2", "key", "value");
//  
// These features, coupled with Java's unboxing/boxing, overloading, 
// and casting facilities make the dynamic, heterogeneous dictionary a 
// little easier to manipulate in Java:
//
//  Tab t = new Tab("{'a':1, 'b':2.2, 'c':'three'}");  // dynamic values in Tab 
//  int a = (Integer)t.get("a");                       
//  t.put("nest", new Tab("{'z': 7}");
//  Object t = t.get("nest", "z");               // cascading lookup
//  t.put("nest", "nest2", 7);                   // cascading insert

@SuppressWarnings("serial")
public class Tab extends HashMap<Object, Object> {

    // Parse the Python string
    public Tab(String s) throws ParseException, IOException {
	super();
	PythonStringReader psr = new PythonStringReader(s);
	psr.expectTab(this);
    }

    public Tab(int len) {
	super(len);
    }

    public Tab() {
	super();
    }

    // Print the Tab as a Python dictionary, exposing nested structure
    public void prettyPrint (PrintStream os) { 
	prettyPrintHelper_(os, 0, true, 4); 
    }
    public void prettyPrint (PrintStream os, int starting_indent) {
	prettyPrintHelper_(os, starting_indent, true, 4);
    }
    public void prettyPrint (PrintStream os, int starting_indent, 
			     int indent_additive) {
	prettyPrintHelper_(os, starting_indent, true, indent_additive);
    }

    // Print compact version with no indent
    public void print (PrintStream os) {
	prettyPrintHelper_(os, 0, false, 0); 
    }

    // Overloaded methods to make it easier to get from nested subtables. 
    // For example:
    //   Tab t = new Tab("{'nest':{'a':1}}");
    //   int i = (Integer)t.get("nest", "a"); 
    //      --> i gets 1
    public Object get(Object key1, Object... keys) throws ClassCastException {
	Object lookup = this.get(key1);  // First key handled by HashMap
	return Ptools.getBackend_(lookup, keys);
    }

    // Overloaded methods to make it easier to put into nested subtables. 
    // For example:
    //   Tab t = new Tab("{'nest':{'a':1}}");
    //   t.put("nest", "a", 100);
    //      ---> results in {'nest':{'a':100}}
    public Object put(Object key, Object value) {
	if (!(key instanceof Integer || key instanceof String)) {
	    throw new ClassCastException("Can only have Integers and Strings for keys to Tabs and Arrs");
	}
        // Don't want infinite loop
	//HashMap<Object, Object> tt = (HashMap<Object, Object>)this;
	return super.put(key, value);
    }

    public Object put(Object key1, Object... keys) throws ClassCastException {
	if (keys.length==0) throw new ClassCastException();
	if (keys.length==1) return this.put(key1, keys[0]);
	if (!(key1 instanceof Integer || key1 instanceof String)) {
	    throw new ClassCastException("Can only have Integers and Strings for keys to Tabs and Arrs");
	}
	
	Object lookup = this.get(key1);  // First key handled by HashMap
	return Ptools.putBackend_(lookup, keys);
    }


    // Prints list on 1-line without extra lines and nesting.
    // Use prettyPrint if you want to expose nested structure
    public String toString () { return toString(false); }
    public String toString (boolean pretty) {
	ByteArrayOutputStream os = new ByteArrayOutputStream();
	PrintStream ps = new PrintStream(os);
	if (pretty) {
	    this.prettyPrint(ps);
	} else {
	    this.print(ps);
	}
	return new String(os.toString());
    }

    // Handle recursive printing
    protected void prettyPrintHelper_(PrintStream os, int indent, 
				      boolean pretty, int indent_additive) {
	// Base case, empty table
	if (size()==0) {
	    if (pretty) os.print("{ }");
	    else os.print("{}");
	    return;
	} 

	// Recursive case
	os.print("{");
	if (pretty) os.println();

	// Iterate through, printing out each element
	ArrayList<Object> sorted_keys = new ArrayList<Object>(this.keySet());
	Collections.sort(sorted_keys, new ValComparator());
	for (int ii=0; ii<size(); ii++) {
	    Object key = sorted_keys.get(ii);
	    Object value = this.get(key);
	    
	    if (pretty) {
		Ptools.indentOut_(os, indent+indent_additive);
	    }
	    Ptools.prettyPrint(os, key);
	    os.print(":");

	    // For most values, use default output method
	    if (value instanceof Tab) {
		Tab t = (Tab)value;
		t.prettyPrintHelper_(os, pretty ? indent+indent_additive : 0,
				     pretty, indent_additive);
	    } else if (value instanceof Arr) {
		Arr a = (Arr)value;
		a.prettyPrintHelper_(os, pretty ? indent+indent_additive : 0,
				     pretty, indent_additive);
	    } else {
		Ptools.prettyPrint(os, value);
	    }

	    if (size()>1 && ii!=size()-1) os.print(",");
	    if (pretty) os.println();
	}
	if (pretty) Ptools.indentOut_(os, indent);
	os.print("}");
    }


    
}; 

// Examples of how we'd like to manipulate the dynamic dict:
//  Object v = mt.recv();
//  Tab tt = (Tab) v;
//  Object under = tt.get("nest", "nest2");
//  int i = (Integer)under;
//
//  Tab t = new Tab("{'a':[1,2,3], 'b':2}");
//  Arr a = (Arr)t.get("a");
//  Object v2  = t.get("a", 0);
