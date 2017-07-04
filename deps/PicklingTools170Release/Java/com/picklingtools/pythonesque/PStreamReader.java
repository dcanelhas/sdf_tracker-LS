
package com.picklingtools.pythonesque;

import java.io.IOException;
import java.io.Reader;
import java.text.ParseException;

// A PStreamReader exists to read some text (UTF?) tables from a string
public class PStreamReader extends ReaderA {

    // Read from some Java reader
    public PStreamReader (Reader r, boolean supports_context) {
	super(supports_context);
	cb_ = new CircularBuffer_(1024, true);
	reader_ = r;
    }
    
    
    // ///// Data Members
    
    // For pushback data and whitespace lookahead
    protected CircularBuffer_ cb_;  

    // Where we are reading from
    protected Reader reader_;

    // Return how many chars into CB is the next Non-White Space character.  
    // This is where comments are handled: comments are counted as white space.
    // The default implementation treats # and \n as comments
    protected int fillUntilNWSChar_ () throws IOException
    {
	//System.out.println("fillUntilNWSChar: cb.len = "+ cb_.length());
	// Look for WS or comments that start with #
	boolean comment_mode = false;
	int curr = 0;
	int c = 0;
	while (true) {
	    // Read char from pushback or stream
	    if (curr >= cb_.length()) {
		c = reader_.read();   // read from stream, make sure pushback sees
		//System.out.println("****c read inside of fillUntilNWSChar"+ c);
		if (c==-1) break;
		cb_.put((char)c);
	    } else { 
		c = cb_.peek(curr); // Read only from pushback buffer at first
	    }
	    curr++;
	    // Decide on whitespaciness
	    if (comment_mode) {
		if (c=='\n') { comment_mode = false; }
		continue;
	    } else {
		if (Character.isWhitespace(c)) continue;
		else if (c=='#') { 
		    comment_mode = true;
		    continue;
		} else {
		    break;
		}
	    }
	}
	//System.out.println("retval is "+ curr);
	return curr;
    }
    
    // Get a the next non-white character from input
    protected int getNWSChar_ () throws IOException
    {
	//System.out.println("getNWSChar_ ENTER");
	int how_many = fillUntilNWSChar_();
	int c = -1;
	for (int ii=0; ii<how_many; ii++) {
	    c = getChar_();
	}
	//System.out.println("getNWSChar EXIT:"+c);
	return c;
    }
    
    // Peek at the next non-white character
    protected int peekNWSChar_ () throws IOException
    {
	//System.out.println("peekNWSChar_ ENTER");
	int how_many = fillUntilNWSChar_();
	char c = cb_.peek(how_many-1);
	if (Character.isWhitespace(c)) {
	    return -1;  // EOF!
	}
	//System.out.println("peekNWSChar_ EXIT:"+c);
	return c; 	// last char appended was NOT whitespace!
    }
    
    // get the next character
    protected int getChar_ () throws IOException
    { 
	//System.out.println("getChar_ ENTER");
	int c; 
	// Data still in pushback
	if (!cb_.empty()) {
	    c =cb_.get(); 
	}
	
	// CB is empty, get straight from input
	else {
	    c = reader_.read();
	    //System.out.println("****c read inside of fillUntilNWSChar"+ c);
	}

	// Next char
	if (context_!=null) { 
	    if (c==-1) {
		char eof[] = { 'E', 'O', 'F' };
		context_.addData(eof, 3);
	    } else {
		context_.addChar((char)c);
	    }
	}
	//System.out.println("getChar_ EXIT: GET is"+ c);
	return c;
    }
    
    // look at the next char without getting it
    protected int peekChar_ () throws IOException
    { 
	//System.out.println("peekChar");
	int c;
	if (cb_.empty()) {
	    c = reader_.read();
	    //System.out.println("****c read inside of fillUntilNWSChar"+ c);
	    if (c==-1) return -1;
	    cb_.pushback((char)c);
	}
	return cb_.peek();
    }
    
    // Consume the next bit of whitespace
    protected int consumeWS_ () throws IOException
    {
	//System.out.println("consumeWS");
	boolean comment_mode = false;
	int c;
	while (true) {
	    c = peekChar_();
	    if (c==-1) break;
	    // Decide on whitespaciness
	    if (comment_mode) {
		if (c=='\n') { comment_mode = false; }
		getChar_();
		continue;
	    } else {
		if (Character.isWhitespace(c)) { 
		    getChar_();
		    continue;
		}
		else if (c=='#') { 
		    comment_mode = true;
		    getChar_();
		    continue;
		} else {
		    break;
		}

	    }

	}
	return c;
    }
    
    // The pushback allows just a little extra flexibility in parsing:
    // Note that we can only pushback chracters that were already there!
    protected void pushback_ (int pushback_char) throws ParseException
    { 
	//System.out.println("pushback:"+pushback_char);
	if (pushback_char == -1) { // don't pushback EOF
	    return; 
	} else {
	    cb_.pushback((char)pushback_char); 
	}
    }
    
}; // StreamReader



