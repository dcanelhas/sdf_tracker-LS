
package com.picklingtools.pythonesque;

import java.io.IOException;
import java.text.ParseException;

// A PStringReader exists to read some ASCII tables from a string
public class PStringReader extends ReaderA {

    // Read input from array of char: may specify if we can just read
    // or f we have to make a copy
    public PStringReader (char[] a, boolean make_copy, boolean supports_context){
	super(supports_context);
	init_(a, make_copy);
    }
    public PStringReader (char[] a, boolean make_copy) { 
	super(true);
	init_(a, make_copy);
    }
    public PStringReader (char[] a) {
	super(true);
	init_(a, false);
    }
    public void init_ (char[] a, boolean make_copy) {
	length_ = a.length;
	current_ = 0;

	if (make_copy) {
	    data_ = new char[length_];
	    for (int ii=0; ii<length_; ii++) {
		data_[ii] = a[ii];
	    }
	    adopting_ = true;
	} else {
	    data_ = a;
	    adopting_ = false;
	}
    }

    // Read a StringBuffer: may have to make own copy
    public PStringReader (StringBuffer sb, boolean make_copy,
			 boolean supports_context) {
	super(supports_context);
	init_(sb, make_copy);
    }
    public PStringReader (StringBuffer sb, boolean make_copy) {
	super(true);
	init_(sb, make_copy);
    }
    public PStringReader (StringBuffer sb) { 
	super(true);
	init_(sb, false); 
    }
    public void init_ (StringBuffer sb, boolean make_copy) {
	length_ = sb.length();
	current_ = 0;

	data_ = new char[length_];
	for (int ii=0; ii<length_; ii++) {
	    data_[ii] = sb.charAt(ii);
	}
	if (make_copy) {
	    adopting_ = true;
	} else {
	    // Can't really discriminate in Java
	    adopting_ = false;
	}
    }

    // Read a String: may have to make own copy
    public PStringReader (String s, boolean make_copy,
			 boolean supports_context) {
	super(supports_context);
	init_(s, make_copy);
    }
    public PStringReader (String s, boolean make_copy) {
	super(true);
	init_(s, make_copy);
    }
    public PStringReader (String s) { 
	super(true);
	init_(s, false); 
    }
    public void init_ (String s, boolean make_copy) {
	length_ = s.length();
	current_ = 0;

	data_ = new char[length_];
	for (int ii=0; ii<length_; ii++) {
	    data_[ii] = s.charAt(ii);
	}
	if (make_copy) {
	    adopting_ = true;
	} else {
	    // Can't really discriminate in Java
	    adopting_ = false;
	}
    }

    
    // virtual ~PStringReader ()  { if (adopting_) delete [] data_; }
    
    
    
    // ///// Data Members
    
    protected char[] data_;      // pointer to start of data: not necessarily adopted
    
    // length of full data: may just be buffer_.length() or not
    protected int length_;     

    // Determines how we clean up: 
    protected boolean adopting_;   

    // Current place in the input stream, regardless of input 
    protected int current_;   


    // Return the index of the next Non-White Space character.  This is
    // where comments are handled: comments are counted as white space.
    // The default implementation treats # and \n as comments
    protected int indexOfNextNWSChar_ () 
    {
	final int len=length_;
	int cur = current_;
	if (cur==len) return cur;
	// Look for WS or comments that start with #
	boolean comment_mode = false;
	for (; cur<len; cur++) {
	    if (comment_mode) {
		if (data_[cur]=='\n') { comment_mode = false; }
		continue;
	    } else {
		if (Character.isWhitespace(data_[cur])) continue;
		else if (data_[cur]=='#') { 
		    comment_mode = true;
		    continue;
		} else {
		    break;
		}
	    }
	}
	return cur;
    }
    
    // Get a the next non-white character from input
    protected int getNWSChar_ () throws IOException
    {
	int index = indexOfNextNWSChar_();
	
	// Save all chars read into 
	int old_current = current_;
	current_ = index;
	if (context_ != null) 
	    context_.addData(data_, old_current, current_-old_current);
	
	return getChar_();
    }
    
    // Peek at the next non-white character
    protected int peekNWSChar_ () throws IOException
    {
	int index = indexOfNextNWSChar_();
	if (index>=length_) return -1;
	char c = data_[index]; // TODO: should be unsigned conversion?
	return c;
    }
    
    // get the next character
    protected int getChar_ () throws IOException
    { 
	final int len=length_; 
	if (current_==len) return -1;
	
	char c = data_[current_++]; // avoid EOF/int-1 weirdness
	if (context_!=null) context_.addChar(c);
	
	// Next char
	return c;
    }
    
    // look at the next char without getting it
    protected int peekChar_ () throws IOException
    { 
	final int len=length_; 
	if (current_==len) return -1;
	char c = data_[current_]; // avoid EOF/int-1 weirdness
	return c;
    }
    
    // Consume the next bit of whitespace
    protected int consumeWS_ () throws IOException
    {
	int index = indexOfNextNWSChar_();
	
	int old_current = current_;
	current_ = index;
	if (context_!=null) 
	    context_.addData(data_, old_current, current_-old_current);
	
	if (index==length_) return -1;
	char c = data_[index]; // avoid EOF/int-1 weirdness
	return c;
    }
    
    // The pushback allows just a little extra flexibility in parsing:
    // Note that we can only pushback chracters that were already there!
    protected void pushback_ (int pushback_char) throws ParseException
    {
	// EOF pushback
	if (pushback_char==-1) {
	    if (current_!=length_) {
		syntaxError("Internal Error: Attempt to pushback EOF when not at end");
	    } else {
		return;
	    }
	}
	if (current_<=0) {
	    syntaxError("Internal Error: Attempted to pushback beginning of file");
	}
	// Normal char pushback
	else if (data_[--current_]!=pushback_char) {
	    //cout << "** pushback_char" << pushback_char << " buffer" << buffer_[current_] << endl;
	    syntaxError("Internal Error: Attempt to pushback diff char");
	}
	if (context_!=null) 
	    context_.deleteLastChar();
	return;
    }
    
}; // PStringReader



