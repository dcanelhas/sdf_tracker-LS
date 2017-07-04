

package com.picklingtools.pythonesque;

import java.io.IOException;
import java.text.ParseException;
    
// Interface for all Readers parsing ASCII streams.  They all have a
// context for holding the current line numbers for error reporting
// purposes.
public abstract class ReaderA {
    
    // We can turn off the context if we are doing parsing where we
    // don't care about the context
    public ReaderA (boolean supports_context) {
	if (supports_context) context_ = new Context_();
    }

    public void syntaxError (String s) throws ParseException
    {
	// Syntax Error generates a full report, which is expensive:
	// you can turn it off if you are doing silent reads
	if (context_!=null) {
	    context_.syntaxError(s); 
	} else {
	    throw new ParseException(s, 0);
	}
    }

    public boolean EOFComing () throws IOException { return peekNWSChar_() == -1; }

    // Inherired only interface

    protected Context_ context_; 

    abstract int getNWSChar_ () throws IOException;
    abstract int peekNWSChar_ () throws IOException;
    abstract int getChar_ () throws IOException;
    abstract int peekChar_ () throws IOException;
    abstract int consumeWS_ () throws IOException;
    abstract void pushback_ (int pushback_char) throws ParseException;

}; // ReaderA

