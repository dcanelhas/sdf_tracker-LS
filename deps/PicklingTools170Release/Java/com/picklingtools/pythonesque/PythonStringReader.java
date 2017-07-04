

// Read Java style strings.  NOTE that these are unicode strings!

package com.picklingtools.pythonesque;



public class PythonStringReader extends PythonReaderA {

    public PythonStringReader (String s) {
	super(new PStringReader(s));
    }

}; // PythonStringReader