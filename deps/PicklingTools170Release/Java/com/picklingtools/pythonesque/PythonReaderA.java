
// A class to help read "human-readable" strings of Python-esque 
// Objects (Tabs, Arrs, numbers, Numeric arrays).  Format is straight-forward:
//
//   PythonReader v = New PythonReader(" { 'a':'nice', 'table':1 }");
//   Tab t = v.expectTab(); 
//
// From Files (streams):
//   StreamPythonReader sv = new StreamPythonReader(istream& is);
//   Tab t = new Tab();
//   sv.expectTab(t);
//
// The constructors of Tab and Arr have been augmented so as to allow
// easier construction:
//   
//   Tab t = new Tab("{ 'a':1, 'b':2, 'c':3, 'list':[1,2,3] }");
//   Arr a = new Arr("[ 'a', 2, 3.3, (1+2j), { 'empty':'ish' } ]");

package com.picklingtools.pythonesque;

import java.io.IOException;
import java.math.BigInteger;
import java.text.ParseException;


// Abstract base class: All the code for parsing the letters one by
// one is here.  The code for actually getting the letters (from a
// string, stream, etc.) defers to the derived class.

public class PythonReaderA { 

    static int EOF = -1;

    // Adopt a reader (string, stream, whatever)
    public PythonReaderA (ReaderA adopted_reader) {
	reader_ = adopted_reader;
    }
    
    
    // Look ahead and see that that next thing coming is an EOF
    public boolean EOFComing () throws IOException { return reader_.EOFComing(); }
    
    // Expect any number (including complex)
    public Object expectNumber () throws ParseException, IOException
    {
	consumeWS_();
	int c=peekChar_();
	if (c=='(') {
	    return expectComplex_();
	}
	return expectSimpleNumber();
    }
    

    public Object expectSimpleNumber () throws ParseException, IOException
    { return expectSimpleNumber(false); }
    public Object expectSimpleNumber (boolean return_string) throws ParseException, IOException
    {
	consumeWS_();
	
	// Get the integer part, if any, of the number
	String integer_part = getSignedDigits_('.', true);
	

	// == instead of equals on purpose
	if (integer_part.equals("+inf")) { // will print as nan, -inf and inf
	    if (return_string) {
		return integer_part;
	    } else {
		return new Double(Double.POSITIVE_INFINITY);
	    }
	} else if (integer_part.equals("-inf")) {
	    if (return_string) {
		return integer_part;
	    } else {
		return new Double(Double.NEGATIVE_INFINITY);
	    }
	} else if (integer_part.equals("nan")) {
	    if (return_string) {
		return integer_part;
	    } else {
		return new Double(Double.NaN);
	    }
	}
	// really, just fall through
	
	// Get the fractional part, if any
	String fractional_part = new String();
	int c = peekChar_();
	if (c=='.') {     
	    c = getChar_(); // consume the '.'
	    fractional_part = "."+getDigits_();
	    if (fractional_part.length()==1) { 
		int i_len = integer_part.length();
		if (i_len==0 || 
		    (i_len>0 && 
		     !Character.isDigit(integer_part.charAt(i_len-1)))) {
		    syntaxError_("Expecting some digits after a decimal point, but saw '"+saw_(peekChar_())+"'");
		}
	    }
	    c = peekChar_();
	}
	
	// Get the exponent part, if any
	String exponent_part = "";
	if (c=='e' || c=='E') {
	    c = getChar_();  // consume the 'e'
	    exponent_part = "e"+getSignedDigits_(' ', false);	    
	    if (exponent_part.length()==1) { // only an e
		syntaxError_("Expected '+', '-' or digits after an exponent, but saw '"+saw_(peekChar_())+"'");
	    }
	    c = peekChar_();
	}
	
	// At this point, we are (mostly) finished with the number, and we
	// have to build the proper type of number.
	if (fractional_part.length()>0 || exponent_part.length()>0) {
	    // If we have either a fractional part or an exponential part,
	    // then we have a floating point number
	    String stringized_number=integer_part+fractional_part+exponent_part;
	    if (return_string) {
		return stringized_number;
	    } else {
		return new Double(stringized_number);
	    }
	}
	
	// Well, no fractional part or exponential.  There had better be
	// some digits!
	if (integer_part.length()==0 || 
	    (integer_part.length()>0 && 
	     !Character.isDigit(integer_part.charAt(integer_part.length()-1))))
	    syntaxError_("Expected some digits for a number");
	
	c=peekChar_();
	if (c=='l' || c=='L') { // Okay, it's a long
	    getChar_();  // consume long
	    //if (integer_part.charAt(0)=='-') {
	    //	return new BigInteger(integer_part);
	    //} else {
	    //return new BigInteger(integer_part);
	    //}
	    if (return_string) {
		return integer_part;
	    } else {
		return new BigInteger(integer_part);
	    }
	} else { // plain integer
	    if (return_string) {
		return integer_part;
	    } else {
		return convertInt_(integer_part); // assumes some int, with string inside
	    }
	}
    }
    

    // Read a string from the current place in the input
    public String expectStr () throws ParseException, IOException
    { 
	consumeWS_();
	
	int quote_mark = peekNWSChar_();
	if (quote_mark!='\'' && quote_mark!='"') {
	    syntaxError_("A string needs to start with ' or \"");
	}
	
	expectChar_((char)quote_mark); // Start quote
	
	// Read string, keeping all escapes, and let DeImage handle escapes
	StringBuffer a = new StringBuffer();
	for (int c=getChar_(); c!=quote_mark; c=getChar_()) {
	    if (c==EOF) syntaxError_("Unexpected EOF inside of string");
	    char cc = (char)c;
	    a.append(cc);
	    if (cc=='\\') { // escape sequence
		int next = getChar_(); // Avoid '
		if (next==EOF) syntaxError_("Unexpected EOF inside of string");
		a.append((char)next);
	    } 
	}    
	String temp = new String(a);
	String ss = StringTools.DeImage(temp, false); // Do escapes 
	return ss;
    }
   
    
    // Expect Table on the input
    public Tab expectTab () throws ParseException, IOException {
	Tab t=new Tab();
	expectSomeTable_(t,"{"); 
	return t;
    }

    // Expect Table on the input
    public void expectTab (Tab add_to) throws ParseException, IOException {
	expectSomeTable_(add_to,"{"); 
    }
    
    // Expect an Arr
    public Arr expectArr () throws ParseException, IOException {
	Arr a=new Arr(); 
	expectSomeCommaList_(a, '[', ']', false); 
	return a;
    }

    // Expect an Arr
    public void expectArr (Arr a) throws ParseException, IOException {
	expectSomeCommaList_(a, '[', ']', false); 
    }


    // Read in a Numeric Array
    Object expectNumericArray () throws ParseException, IOException {
	Arr a = new Arr();
	String typetag = ""; // default int list of no default
	consumeWS_();

	expect_("array(");
	int peek = peekNWSChar_();
	if (peek==EOF) syntaxError_("Unexpected EOF after array(");

	// Python Array
	if (peek=='\'' || peek=='"') {
	    typetag = expectStr();
	    expect_(",");
	    expectArrOfNumberStrings_(a);
	} 
	// Numeric or NumPy array
	else if (peek=='[') {
	    expectArrOfNumberStrings_(a);
	    peek = peekNWSChar_();
	    if (peek==EOF) syntaxError_("Unexpected EOF during array");
	    if (peek=='\'' || peek=='"') {
		typetag = expectStr();
	    } else if (peek=='d') {
		expect_("dtype=");
		// Got dtype ... want everything upto ')'
		StringBuilder sb = new StringBuilder();
		while (true) {
		    int ch = getChar_();
		    if (ch==EOF) {
			syntaxError_("Saw EOF before seeing ')'");	
		    } else if (Character.isJavaIdentifierPart(ch)) {
			sb.append((char)ch);
		    } else {
			pushback_(ch);
			break;
		    }
		}
		if (typetag.length()==0) {
		    syntaxError_("Empty dtype?");
		} else {
		    typetag = new String(sb);
		}
	    } else if (peek!=')') {
		syntaxError_("Expected ')' after array start");
	    }
	}
	expect_(")");
	return convertArray_(a, typetag);
    }
    

    protected void expectArrOfNumberStrings_ (Arr a) throws ParseException, IOException {
	expectSomeCommaList_(a, '[', ']', true); 
    }
    

    protected Object convertArray_(Arr a, String typetag) throws ParseException {
	char tag = 'i';
	if (typetag.length()!=0) {
	    tag = typetag.charAt(0);
	} 
	switch (tag) {
	case 'c':
	case 'b':
	case 'B': {
	    byte[] f = new byte[a.size()];
	    for (int ii=0; ii<f.length; ii++) {
		String s = (String)a.get(ii);
		f[ii] = new Byte(s);
	    }
	    return f;
	}
	case 'h':
	case 'H': {
	    short[] f = new short[a.size()];
	    for (int ii=0; ii<f.length; ii++) {
		String s = (String)a.get(ii);
		f[ii] = new Short(s);
	    }
	    return f;
	}
	case 'i':
	case 'I': {
	    int[] f = new int[a.size()];
	    for (int ii=0; ii<f.length; ii++) {
		String s = (String)a.get(ii);
		f[ii] = new Integer(s);
	    }
	    return f;
	}
	case 'l':
	case 'L': {
	    long[] f = new long[a.size()];
	    for (int ii=0; ii<f.length; ii++) {
		String s = (String)a.get(ii);
		f[ii] = new Long(s);
	    }
	    return f;
	}
	case 'f': {
	    float[] f = new float[a.size()];
	    for (int ii=0; ii<f.length; ii++) {
		String s = (String)a.get(ii);
		f[ii] = new Float(s);
	    }
	    return f;
	}
	case 'd': {
	    double[] f = new double[a.size()];
	    for (int ii=0; ii<f.length; ii++) {
		String s = (String)a.get(ii);
		f[ii] = new Double(s);
	    }
	    return f;
	}
	default: syntaxError_("Unknown typetag:"+typetag);
	}

	return 'i'; // should never reach here
    }

    
    /*
    // Expect one of two syntaxes:
    // o{ 'a':1, 'b':2 }
    //    or
    // OrderedDict([('a',1),('b',2)])
    boolean expectOTab (OTab& o) 
    {
	char c = peekNWSChar_();
	// Short syntax
	if (c=='o') { // o{ } syntax
	    return expectSomeTable_(o, "o{");
	} 
	// Long syntax
	if (!expect_("OrderedDict(")) return false;
	Arr kvpairs;
	// We let the Arr and Tup parse themselves correctly, but since we
	// are using swapKeyValueInto, this just plops them into the table
	// pretty quickly, without excessive extra deep copying.
	if (!expectArr(kvpairs)) return false;
	const int len = int(kvpairs.length());
	for (int ii=0; ii<len; ii++) {
	    Tup& t = kvpairs[ii];
	    o.swapInto(t[0], t[1]);
	} 
	expectChar_(')');
	return true;
    }
    */

    /*
    // Know a Tup is coming: starts with '(' and ends with ')'
    boolean expectTup (Tup& u) { return expectSomeCommaList_(u.impl(), '(', ')'); } 

    // In general, a '(' starts either a complex number such as (0+1j)
    // or a tuple such as (1,2.2,'three'): This parses and places the
    // appropriate thing in the return value.
    boolean expectTupOrComplex (Val& v)
    {
	expectChar_('(');
	
	// Handle empty tuple
	int peek = peekNWSChar_();
	if (peek == EOF) syntaxError_("Saw EOF after seeing '('");
	if (peek==')') {
	    v = Tup();
	    expectChar_(')');
	    return true;
	}
	
	// Anything other than empty, has a first component
	Val first;
	if (!expectAnything(first)) return false;
	
	// Well, plain jane number: if we see a + or -, it must be a complex
	peek = peekNWSChar_();
	if (OC_IS_NUMERIC(first) && !OC_IS_CX(first) && 
	    (peek=='+' || peek=='-')) {
	    // make sure we keep the "-" or "+" so we get the right sign!
	    Val second;
	    if (!expectSimpleNumber(second)) return false;
	    if (!expect_("j)")) return false;
	    if (first.tag=='f' || second.tag=='f') {
		v = complex_8(first, second);
	    } else {
		v = complex_16(first, second);
	    }
	    return true;
	}
	
	// If we don't see a plain number, it has to be a Tuple
	Tup& u = v = Tup(None); // just put something is first pos
	first.swap(u[0]);
	Array<Val>& a = u.impl();
	return continueParsingCommaList_(a, '(', ')');
    }
    
    */
    
    // We could be starting ANYTHING ... we have to dispatch to the
    // proper thing
    public Object expectAnything () throws ParseException, IOException
    {
	Object o = null;
	int c = peekNWSChar_();
	if (c==EOF) syntaxError_("EOF encountered");
	switch ((char)c) {
	case '{' : { return expectTab(); }
	case '[' : { return expectArr();   }
	case '(' : { return o; /*expectTupOrComplex(v);*/ }
	case '\'':
	case '"' : { return expectStr(); }
        case 'a' : { return expectNumericArray(); }
	case 'N' : { Object empty=null; return expect_("None", empty); }
	case 'T' : { Boolean t = new Boolean(true); return expect_("True", t); }
	case 'F' : { Boolean f=new Boolean(false); return expect_("False", f); }
	case 'o' : // o{ and OrderedDict start OTab
        case 'O' : { return o; /*expectOTab(ot);*/ } 
	default:   { return expectNumber(); }
	}
	
    }
    
    	
    // Helper Methods for syntax error: multiple evals would
    // construct way too many strings when all we pass is ptrs    
    protected void syntaxError_ (String s) throws ParseException
    {
	reader_.syntaxError(s);
    }
    
    
    protected String saw_ (int cc)
    {
	if (cc==EOF) return "EOF";
	char[] ccc = new char[1];
	ccc[0] = (char)cc;
	return new String(ccc);
    }


    protected void expectChar_ (char expected) throws ParseException,IOException
    { 
	int get=getNWSChar_(); 
	if (get!=expected) { 
	    String get_string, expected_string; 
	    if (get==EOF) {
		get_string="EOF";
	    } else { 
		char a[] = new char[1];
		a[0] = (char)get;
		get_string=new String(a); 
	    }
	    char a[] = new char[1];
	    a[0] = expected;
	    expected_string= new String(a);
	    syntaxError_("Expected:'"+expected_string+"', but saw '"+
			 get_string+"' on input"); 
	} 
    } 


    protected void expect_ (String s) throws ParseException, IOException
    {
	int s_len = s.length();
	for (int ii=0; ii<s_len; ii++) 
	    expectChar_(s.charAt(ii));
    }

    protected Object expect_ (String s, Object o) throws ParseException, IOException
    {
	int s_len = s.length();
	for (int ii=0; ii<s_len; ii++) 
	    expectChar_(s.charAt(ii));
	return o;
    }
    
    protected void expect_ (StringBuffer sb) throws ParseException, IOException
    {
	int sb_len = sb.length();
	for (int ii=0; ii<sb_len; ii++) 
	    expectChar_(sb.charAt(ii));
    }


    // Expect a complex number:  assumes it will have form (#+#j)
    Object expectComplex_ () throws ParseException, IOException
    {
	Object o;
	expectChar_('(');
	Object real_part, imag_part;
	real_part = expectNumber();
	int ic=peekChar_();
	if (ic==EOF) syntaxError_("Premature EOF");
	char c = (char)ic;
	if (c=='+' || c=='-') {
	    imag_part = expectNumber();
	}
	expect_("j)");
	
	//result.re = real_part;
	//result.im = imag_part;
	//n = result;
	o = real_part;
	return o;
    }
   

    // From current point of input, consume all digits until
    // next non-digit.  Leaves non-digit on input
    protected String getDigits_ () throws IOException
    {
	StringBuffer digits = new StringBuffer();
	while (true) {
	    int c = peekChar_();
	    if (c==EOF) 
		break;
	    if (Character.isDigit(c)) {
		digits.append((char)c);
		getChar_();
	    }
	    else 
		break;
	}
	return new String(digits);
    }
    
    // From current point in input, get the signed sequence of 
    // digits
    protected String getSignedDigits_ (char next_marker, boolean inf_nan_okay) throws ParseException, IOException
    {
	// Get the sign of the number, if any
	char c=(char)peekChar_();
	char the_sign='\0'; // Sentry
	
	// Add ability to recognize nan, inf, and -inf
	if (inf_nan_okay) {
	    if (c=='n') { // expecting nan
		expect_("nan");
		return new String("nan");
		
	    } else if (c=='i') {
		expect_("inf");
		return new String("+inf");
	    }  
	}
	
	char saw_sign = ' ';
	if (c=='+'||c=='-') {
	    saw_sign = c;
	    the_sign = c;
	    getChar_();    // consume the sign
	    c=(char)peekChar_(); // .. and see what's next
	}
	
	if (inf_nan_okay) {    
	    if (saw_sign!=' ' && c=='i') { // must be -inf
		expect_("inf");
		if (saw_sign=='-') {
		    return new String("-inf");
		} else { // if (saw_sign=='+') {
		    return new String("+inf");
		}
	    }
	}
	
	// Assertion: passed sign, now should see number or .
	if (!Character.isDigit(c)&&c!=next_marker) {
	    syntaxError_("Expected numeric digit or '"+saw_(next_marker)+"' for number, but saw '"+saw_(c)+"'");
	}

	String digits = getDigits_();
	if (the_sign != '\0') {
	    StringBuffer sb=new StringBuffer();
	    sb.append(the_sign);
	    digits = new String(sb) + digits;
	}
	return digits;
    }

/*
    protected <T>  readNumericArray_ (arr, const Val& first_value)
    { 
	arr = Array<T>();  // initialize the array
	Array<T>& a = arr; // .. then a reference to it
	T v = first_value;
	a.append(v);
	
	// Continue getting each item one at a time
	while (1) {
	    char peek = peekNWSChar_();
	    if (peek==',') {
		expectChar_(',');
		continue;
	    } else if (peek==']') {
		break;
	    } 
	    
	    Val n; 
	    if (!expectNumber(n)) return false;
	    T v = n;
	    a.append(v);
	}
	return true;
    }
*/
    static final String smallest_int_4 = "2147483648";
    static final String smallest_int_8 = "9223372036854775808";
    
    static final String biggest_int_4 = "2147483647";
    static final String biggest_int_8 = "9223372036854775807";
    static final String biggest_int_u8 = "18446744073709551615";

    // the string of +- and digits: convert to some int
    Object convertInt_ (String v)  throws ParseException, IOException
    {
	// Strip into sign and digit part 
	char sign_part = '+';
	String digit_part = v;
	if (!Character.isDigit(digit_part.charAt(0))) {
	    sign_part = digit_part.charAt(0);
	    digit_part = digit_part.substring(1); 
	}
	int len = digit_part.length();
	
	// Try to turn into int_4 in general, otherwise slowly move from
	// int_8 to real_8
	if (sign_part=='-') { // negative number

	    if (len<smallest_int_4.length() ||
		(len==smallest_int_4.length() && 
		 digit_part.compareTo(smallest_int_4)<=0)) { 
		int i4_val = Integer.parseInt(v);
		return new Integer(i4_val);
	    } else if (len<smallest_int_8.length() ||
		       (len==smallest_int_8.length() && 
			digit_part.compareTo(smallest_int_8)<=0)) {
		long i8_val = Long.parseLong(v);
		return new Long(i8_val);
	    } else {
		BigInteger bi = new BigInteger(v);
		return bi;
	    }
	    
	} else { // positive number

	    if (len<biggest_int_4.length() || 
		(len==biggest_int_4.length() && 
		 digit_part.compareTo(biggest_int_4)<=0)) {
		int i4_val = Integer.parseInt(v);
		return new Integer(i4_val);
	    } else if (len<biggest_int_8.length() ||
		       (len==biggest_int_8.length() && 
			digit_part.compareTo(biggest_int_8)<=0)) {
		long i8_val = Long.parseLong(v);
		return new Long(i8_val);
	    } else if (len<biggest_int_u8.length() ||
		       (len==biggest_int_u8.length() && 
			digit_part.compareTo(biggest_int_u8)<=0)) {
		long iu8_val = Long.parseLong(v);
		return new Long(iu8_val);
	    } else {
	        BigInteger bi = new BigInteger(v);
		return bi;
	    }
	}
	
    }
    
    // Expect Table on the input
    protected void expectSomeTable_ (Tab table, String start_table) throws ParseException, IOException
    {
	expect_(start_table);
	// Special case, empty Table
	char peeka = (char)peekNWSChar_();
	if (peeka!='}') {
	    
	    for (;;) { // Continue getting key value pairs
		{ 
		    Object key = expectAnything();
		    // Let put handle it consistently?
		    if (!(key instanceof String) && !(key instanceof Integer)) {
			syntaxError_("keys can only be Integers and Strings currently");
		    }
		    expectChar_(':');
		    Object value = expectAnything();
		    
		    table.put(key, value); 
		}
		
		int peek = peekNWSChar_();
		if (peek==',') {
		    expectChar_(',');     // Another k-v pair, grab comma and move on
		    peek = peekNWSChar_();
		    if (peek=='}') break; // CAN have extra , at the end of the table
		    continue;
		} else if (peek=='}') { // End of table
		    break;
		} else {
		    syntaxError_("Expecting a '}' or ',' for table, but saw '"+
				 saw_(peek)+"'");
		}
	    }
	}
	expectChar_('}');
    }
  

    protected void continueParsingCommaList_ (Arr a, 
					      char start, char end,
					      boolean grabbing_numbers)
	throws ParseException, IOException
    {
	// Continue getting each item one at a time
	while (true) {         
	    int peek = peekNWSChar_();
	    if (peek==',') {
		expectChar_(',');
		peek = peekNWSChar_();
		if (peek==end) break;  // CAN have extra , at the end of the table		
		Object next = null;
		if (grabbing_numbers) {
		    next = expectSimpleNumber(true);
		} else {
		    next = expectAnything();
		}
		a.add(next);
      
		continue;
	    } else if (peek==end) {
		break;
	    } else {
		char c[] = { start, end, '\0' };
		String s = new String(c);
		syntaxError_("Problems parsing around '"+saw_(peek)+" for "+s);
	    }
	}
	expectChar_(end);
	return;
    }
    
    protected void expectSomeCommaList_ (Arr a, char start, char end, 
					 boolean grabbing_numbers) 
	throws ParseException, IOException
    {
	expectChar_(start);

	// Special case, empty list
	char peek = (char)peekNWSChar_();
	if (peek==end) {
	    expectChar_(end);
	} else {
	    // Continue getting each item one at a time, after gotten first
	    Object next = null;
	    if (!grabbing_numbers) {
		next = expectAnything();
	    } else {
		next = expectSimpleNumber(grabbing_numbers);
	    }
	    a.add(next);
	    continueParsingCommaList_(a, start, end, grabbing_numbers);
	}
    }


    // Dispatch for input
    int getNWSChar_ ()  throws IOException { return reader_.getNWSChar_(); }
    int peekNWSChar_ () throws IOException { return reader_.peekNWSChar_(); }
    int getChar_ ()     throws IOException { return reader_.getChar_(); }
    int peekChar_ ()    throws IOException { return reader_.peekChar_(); }
    int consumeWS_ ()   throws IOException { return reader_.consumeWS_(); }
    void pushback_ (int pushback_char) throws ParseException { reader_.pushback_(pushback_char); }
    
    // Defer IO to another class.  All sorts of discussion on why
    // didn't we inherit, etc.  Look at the Design Patterns book.
    ReaderA reader_; 
    
}; // PythonReaderA

