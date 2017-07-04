
// Read Java style streams. 

package com.picklingtools.pythonesque;

import java.io.Reader;

// A StringReader exists to read some ASCII tables from a string

public class PythonStreamReader extends PythonReaderA {

    public PythonStreamReader (Reader r, boolean supports_context) {
	super(new PStreamReader(r, supports_context));
    }

}; // PythonStreamReader