

// A helper class that keeps track of where we are within the parsing:
// allows us to return better error messages from parsing the tabs.

package com.picklingtools.pythonesque;

import java.text.ParseException;
import java.util.ArrayList;

public class Context_ {

    // Create a Parsing Context and remember the last n lines for when
    // error messages happen
    public Context_ (int keep_last_n_lines) { init_(keep_last_n_lines); }
    public Context_ ()                      { init_(5); }
    public void init_ (int keep_last_n_lines) {
	contextLines_ = keep_last_n_lines;
	data_         = new CircularBuffer_(1024);
	lineNumber_   = 1;
	charNumber_   = 0; 
    }

    // Add a character to the context
    public void addChar (char c) 
    {
	// Notice the \n so we can notice when new lines begin
	if (c=='\n') {
	    lineNumber_ += 1;
	    charNumber_ =  0;
	}
	// Keep the last 1024 or so characters
	if (data_.full()) {
	    data_.get();
	}
	data_.put(c);
	charNumber_++;
    }

    // Delete a character from the context
    void deleteLastChar () 
    {
	char c = data_.drop();
	// Notice the \n so we can notice when new lines begin
	if (c=='\n') {
	    lineNumber_ -= 1;
	    // Find last \n ... if we can
	    int index_of_last_newline = -1;
	    for (int ii=0; ii<data_.length(); ii++) {
		if (data_.peek(data_.length()-ii-1)=='\n') {
		    index_of_last_newline = ii;
		    break;
		}
	    }
	    charNumber_ = (index_of_last_newline==-1) ? 80: index_of_last_newline;
	} else {
	    charNumber_--;
	}
    }
    
    // Add from this buffer, the amount of data
    void addData (char[] buffer, int len)
    {
	for (int ii=0; ii<len; ii++) {
	    addChar(buffer[ii]);
	}
    }

    // Add from this buffer, the amount of data
    void addData (char[] buffer, int offset, int len)
    {
	for (int ii=0; ii<len; ii++) {
	    addChar(buffer[offset+ii]);
	}
    }


    // Generate a string which has the full context (the last n lines)
    public String generateReport () 
    {
	StringBuffer report = new StringBuffer();
	
	// Post processing: Create an array of the lines of input.  The
	// last line probably won't end with an newline because the error
	// probably happened in the middle of the line.
	ArrayList<StringBuffer> lines = new ArrayList<StringBuffer>();
	StringBuffer current_line = new StringBuffer();
	for (int ii=0; ii<data_.length(); ii++) {
	    char c = data_.peek(ii);
	    current_line.append(c);
	    if (c=='\n') {
		lines.add(current_line);
		current_line = new StringBuffer();
	    }
	}
	if (current_line.length() != 0) {
	    current_line.append('\n');
	    lines.add(current_line);
	}
	
	// Only take the last few lines for error reporting
	int context_lines = contextLines_;
	if (lines.size() < contextLines_) context_lines = lines.size();
	if (context_lines != 0) {
	    int start_line = lines.size()-context_lines;
	    StringBuffer s = new StringBuffer("");
	    if (context_lines>1) s = new StringBuffer("s");
	    report.append("****Syntax Error on line:").append(lineNumber_);
	    report.append(" Last ").append(context_lines).append(" line");
	    report.append(s).append(" of input (lines ").append(start_line+1);
	    report.append("-").append(start_line+context_lines);
	    report.append(") shown below****\n");
	    
	    for (int ii=0; ii<context_lines; ii++) {
		report.append("  " + lines.get(start_line+ii));
	    }
	    // Show, on last line, where!
	    StringBuffer cursor = new StringBuffer();
	    for (int ii=0; ii<charNumber_+1; ii++) 
		cursor.append('-');
	    cursor.append("^\n");
	    report.append(cursor);
	}
	return new String(report);
    }
    
    // Have everything do a syntax error the same way
    public void syntaxError (String s) throws ParseException
    { 
	String report = generateReport() + s;
	throw new ParseException(report, lineNumber_);
    }
    
    
    // When holding the data, we make the "keeping context" operations
    // cheap, but when we need to actually supply the information, it's
    // a little more expensive, as we have to look for line breaks, etc.
    
    // how many lines of context to hold (i.e., how much do we buffer)
    protected int contextLines_;  
    
    // The current "context", i.e., that last few lines 
    protected CircularBuffer_ data_;  
    
    protected int lineNumber_;    // Current line number we are on
    protected int charNumber_;   // current character within that line
    

}; // Context_
