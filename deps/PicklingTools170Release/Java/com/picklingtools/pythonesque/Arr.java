
package com.picklingtools.pythonesque;


import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.PrintStream;
import java.text.ParseException;
import java.util.ArrayList;

// Arr is a Python-esque list.
// It operates just list a ArrayList<Object> because it IS-A ArrayList<Object>.
// Boxing and unboxing in general make primitive types easier to manipulate:
//    ArrayList<Object> a = new ArrayList<Object>();
//    a.add(1);
//    a.add(2.2);
//    a.add("three");
//    a.add(new ArrayList<Object>());
//
// The Arr adds some features to the ArrayList that make it easier
// to manipulate.  The Arr "is-a" ArrayList<Object> so it still
// supports all the same methods.
//
// (1) Less typing
//
//        ArrayList<Object> a = new ArrayList<Object>();  
//        Arr a = new Arr();  
//
// (2) Support for string literals, where string literals
//     can be cut-and-paste directly between Java and Python
//
//        Arr a = new Arr("[1, 2.2, 'three', [44]]");  
//            // Same structure as above, in 1 line
//
// (3) Supports print and prettyPrint that looks EXACTLY like Python 
//     dictionaries so you can cut and paste Python dicts between Python 
//     and Java
//   
//        Arr a = new Arr("[1, 2.2, 'three', [44]]");
//
//        // prints showing nested structure
//        a.prettyPrint(System.out); 
//          --> results in
//          [
//            1,
//            2.2,
//            'three',
//            []
//          ]
//
//        // overrides toString so can print directly: only prints without
//        // out newlines and nested structure
//        System.out.println(a);
//          --> results in
//
//          [1,2.2,'three',[]]
//    
// (4) Supports cascading lookups in nested Arr/Tab
// 
//        a.get(3, 0);  
//           --> gets 44 from nested array
//
// (5) Supports cascading inserts into nested Arr/Tab
//
//        a.put(3,0, 777);  
//           --> results in [1, 2.2, 'three', [777]]
//
@SuppressWarnings("serial")
public class Arr extends ArrayList<Object> {
    public Arr(String s) throws ParseException, IOException {
	super ();
	PythonStringReader psr = new PythonStringReader(s);
	psr.expectArr(this);
    }

    public Arr() {
	super();
    }

    // Print the Arr as a Python list, exposing nested structure
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
    //   Arr a = new Arr("[ 1, 2.2, [4,5,6]]");
    //   int i = (Integer)a.get(0, 1);   
    //       --> i gets 5
    public Object get(Integer key1, Object... keys) throws ClassCastException {
	Object lookup = this.get(key1);  // First key handled by HashMap
	return Ptools.getBackend_(lookup, keys);
    }

    // Overloaded methods to make it easier to put into nested sub arrays. 
    // For example:
    //   Arr a = new Arr("[ 1, 2.2, [4,5,6]]");
    //   t.put(2, 0, 100);  // replace 4 with 100
    //      --> results in [1, 2.2, [100, 5, 6]] 
    public Object put(Integer key1, Object... keys) throws ClassCastException {
	if (keys.length==0) throw new ClassCastException();

	Object lookup = this.get(key1);  // First key handled by HashMap
	if (keys.length==1) {
	    // Don't want infinite loop
	    ArrayList<Object> aa = (ArrayList<Object>)this;
	    return aa.set(key1, keys[0]);
	}
	return Ptools.putBackend_(lookup, keys); 
    }


    // Prints dictionary on 1-line without extra lines and nesting.
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
	    if (pretty) os.print("[ ]");
	    else os.print("[]");
	    return;
	} 

	// Recursive case
	os.print("[");
	if (pretty) os.println();

	// Iterate through, printing out each element
	for (int ii=0; ii<size(); ii++) {
	    Object value = this.get(ii);
	    
	    if (pretty) {
		Ptools.indentOut_(os, indent+indent_additive);
	    }

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

	    if (size()>1 && ii!=size()-1) os.print(","); // commas all but last
	    if (pretty) os.println();
	}
	if (pretty) Ptools.indentOut_(os, indent);
	os.print("]");
    }


}; 

