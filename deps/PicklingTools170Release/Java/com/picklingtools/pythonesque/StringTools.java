

package com.picklingtools.pythonesque;

import java.text.ParseException;

public class StringTools {


    // Helper Function 
    static void PrintBufferToArray_ (String vbuff, int bytes, 
				     StringBuffer a,
				     char quote_to_escape)
    {
	String singlebyte = "\n\\\r\t\'\"";
	String       code = "n\\rt'\"";
	String        hex = "0123456789abcdef";
	
	// TODO: revisit for unicode, utf-8
	for (int ii=0; ii<bytes; ii++) {
	    char c = vbuff.charAt(ii);
	    int where = vbuff.indexOf(singlebyte, c);
	    if (c!='\0' && where!=-1 && c!=quote_to_escape) {  // Awkward escape sequence?
		a.append('\\');
		a.append(code.charAt(where));
	    } else if (c>=32 && c<=127) { // Is printable? As you are
		a.append(c); 
	    } else { // Not strange, not printable, hex escape sequence it!
		a.append('\\'); a.append('x');
		int high_nibble = c >> 4; 
		char msn = hex.charAt(high_nibble);
		a.append(msn);
		
		int low_nibble = c & 0x0F; 
		char lsn = hex.charAt(low_nibble);
		a.append(lsn);
	    }
	}
	// a.append('\0'); // Since this is a C-Ctyle printable string
    }
    

    // Return the "image" of the string, but like Python: this means
    // that we prefer '' for strings, but if we have ' inside 
    // a string (and no other "), we use "" instead.  This is just
    // like image except for the "" occasionally
    public static String PyImage (String data)
    {
	// Preprocess looking for the different quotes
	boolean seen_single_quote = false;
	boolean seen_double_quote = false;
	for (int ii=0; ii<data.length(); ii++) {
	    char c = data.charAt(ii);
	    if (c=='\'') seen_single_quote = true;
	    if (c=='"')  seen_double_quote = true;
	}
	char quote_char =(seen_single_quote && !seen_double_quote) ? '"' : '\'';
	char quote_to_esc = (quote_char=='"') ? '\'' : '"';
	
	// Let PrintBuffer do work
	StringBuffer a = new StringBuffer();
	a.append(quote_char);
	PrintBufferToArray_(data, data.length(), a, quote_to_esc);
	a.append(quote_char);
	return a.toString();
    }


    static String hex = "0123456789abcdef";
    static String singlebyte = "\n\\\r\t\'\"";
    static String code        = "n\\rt'\"";


    // Helper function for Depickling
    public static StringBuffer 
	CopyPrintableBufferToStringBuffer (char[] pb, int offset, int pb_bytes) 
	throws ParseException
    {

	StringBuffer a = new StringBuffer();
	
	for (int jj=0; jj<pb_bytes;) {
	    int ii = offset + jj;
	    if (pb[ii]=='\\' && ii+1<pb_bytes) { // non-printable, so was escaped
		int where = code.indexOf(pb[ii+1]);
		if (where != -1) {
		    a.append(singlebyte.charAt(where)); 
		    jj+=2;
		    continue;
		} else if (ii+3<pb_bytes && pb[ii+1]=='x') { 
		    a.append(  
			     (hex.indexOf(pb[ii+2]))*16 + 
			     (hex.indexOf(pb[ii+3]))
			       );
		    jj+=4;
		    continue;
		} else {
		    String m = new String(pb, 0, pb_bytes);
		    throw new ParseException("Malformed Numeric vector string:"+
					     m+" ... Error happened at:", ii);
		}
		
	    } else { // printable, so take as is
		a.append(pb[ii]);
		jj++;
	    }
	}
	return a;
    }

    public static String DeImage (String orig) throws ParseException 
    { return DeImage(orig, false); }
    public static String DeImage (String orig, boolean with_quotes) 
	throws ParseException
    {
	char [] ch_arr = orig.toCharArray();
	StringBuffer sb;
	if (with_quotes) { // quotes are on front and back
	    sb = CopyPrintableBufferToStringBuffer(ch_arr, 1, ch_arr.length-2); 
	} else { // no quotes on front and back
	    sb = CopyPrintableBufferToStringBuffer(ch_arr, 0, ch_arr.length); 
	}
	return new String(sb);
    }

}; // StringTools