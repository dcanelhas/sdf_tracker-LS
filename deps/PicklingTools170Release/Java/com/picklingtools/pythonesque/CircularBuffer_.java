
// A CircularBuffer used to hold elements by value: the end the front
// can inserted/deleted in constant time: it can request infinite
// Circular buffer (meaning that puts will never over-write read
// data).  (Don't complain to me to use Java's Circularbuffer:
// it doesn't support primitive types!)

package com.picklingtools.pythonesque;



class CircularBuffer_ {

    // Construct a circular buffer (with buffer length)
    public CircularBuffer_ (int initial_length, boolean infinite) {
	init_(initial_length, infinite);
    }
    public CircularBuffer_ (int initial_length) { 
	init_(initial_length, false);
    }
    public CircularBuffer_ () {
	init_(4, false);
    }
    protected void init_(int initial_length, boolean infinite)
    {
	nextPut_ = 0;
	nextGet_ = 0;
	empty_   = true;
	infinite_= infinite;
	
	buffLen_ = initial_length;
	buffCap_ = initial_length;
	buff_ = new char[buffLen_];
    }


    public boolean empty ()    { return empty_; } 
    public boolean full ()     { return !empty_ && nextGet_==nextPut_; }
    public boolean infinite () { return infinite_; }
    public int capacity ()     { return buffCap_; }
    public int length () 
    { 
	if (empty()) { return 0; }
	else if (full()) { return capacity(); }
	else if (nextGet_>nextPut_) { return capacity()-(nextGet_-nextPut_); }
	else { return nextPut_-nextGet_; }
    }
    
    // Put a single element into the buffer.  If in infinite mode, a put
    // into a "full" buffer will cause it to re-expand and double the
    // size.  If in finite mode, it will throw a IndexOutOfBoundsError.
    public void put (char c) 
    {     
	checkFullness_();
	// Space available, just plop it in
	buff_[nextPut_] = c;
	nextPut_ = (nextPut_+1) % capacity();
	empty_ = false;
    }
    
    // Get a single element out of the circular buffer.  If the buffer
    // is empty, throw an IndexOutofBoundsError
    public char get () 
    {
	if (empty()) { // Empty, can't get anything
	    throw new IndexOutOfBoundsException("Circular Buffer Empty");
	} else {       // nextGet always tells us where we are
	    char c = buff_[nextGet_];
	    nextGet_ = (nextGet_+1) % capacity();
	    empty_ = (nextGet_ == nextPut_);
	    return c;
	}
    }
    
    // Peek at the nth element (element 0 would be the first thing "get"
    // would return, element 1 would be the next).  Throws the
    // IndexOutOfBoundsError exception if try to peek beyond what the buffer
    // has.  This does NOT advance the circular buffer: it only looks
    // inside.
    public char peek (int where)
    {
	if (where<0 || where>=length()) {
	    throw new IndexOutOfBoundsException("Trying to peek beyond the end of the Circ. Buff");
	}
	int index = (nextGet_+where) % capacity();
	return buff_[index];
    }
    public char peek () { return peek(0); }

    // This implements performing "n" gets, but in O(1) time.  If asked
    // to consume more elements that the CircularBuffer contains, a
    // IndexOutOfBoundsError will be thrown.
    public void consume (int n)
    {
	if (n<0 || n>length()) {
	    throw new IndexOutOfBoundsException("Trying to consume more data than in Circ. Buff");
	}
	empty_ = (n==length());
	nextGet_ = (nextGet_+n) % capacity();
    }
    
    // The "get()" always pulls from one side of the circular buffer:
    // Sometimes, you want to be able to pushback some entry
    // you just got as if it were never "get()" ed.   This is
    // very similar to "put", but it is simply doing it on the other
    // side of the circular buffer.  The pushback can fail if the
    // queue is full (not infinite mode) with a IndexOutOfBoundsError.
    // If it is an infiite queue, it will simply re-expand.
    public void pushback (char pushback_val) 
    {
	checkFullness_();
	// Space available, just plop it in
	nextGet_ = (nextGet_+capacity()-1) % capacity();
	buff_[nextGet_] = pushback_val;
	empty_ = false;
    }
    
    // Drop the last "put()" as if it never went in: this can throw
    // an exception if the buffer is empty.  This will ONLY remain
    // a valid reference until the next non-modifying operation on the Circ. Q
    public char drop ()
    {
	if (empty()) { // Empty, can't get anything
	    throw new IndexOutOfBoundsException("Circular Buffer Empty");
	} else {       // nextPut always tells us where we are
	    nextPut_ = (nextPut_+capacity()-1) % capacity();
	    char c = buff_[nextPut_];
	    empty_ = (nextGet_ == nextPut_);
	    return c;
	}
    }
    
    // Output
    //ostream& print (ostream& os) 
    //{
    //	for (int ii=0, jj=nextGet_; ii<length(); ii++, jj=(jj+1)%capacity()) {
    //	    os << buff_[jj] << " ";
    //	}
    //	return os << endl;
    //}
    
   
    // Arrays of chars and their lengths and capacity
    protected char[]  buff_;
    protected int buffCap_;
    protected int buffLen_;
    
    protected int nextPut_;          // Points where next put will occur
    protected int nextGet_;          // Points to where next get will occur
    protected boolean empty_;        // nextPut_==nextGet is either empty/full
    protected boolean infinite_;     // Puts into empty cause a doubling

    // Centralize fullness check and re-expansion code
    protected void checkFullness_ ()
    {
	if (full()) { // Circ Buffer Full, expand and remake
	    if (!infinite()) { 
		throw new IndexOutOfBoundsException("Circular Buffer full");
	    } else {
		// Create a new Circ. Buffer of twice the size
		int len = buffCap_;
		char[] temp = new char[len*2];
		
		// Recopy to the new, larger circ. buffer
		for (int ii=nextGet_, jj=0; jj<len; ii=(ii+1)%len, jj++) {
		    temp[jj] = buff_[ii];
		}
		
		// Install new buffer
		buff_ = temp;
		buffCap_ = len*2;
		buffLen_ = len;

		nextPut_ = len;
		nextGet_ = 0;
	    }
	} // ... and return now that space is available
    }
    
}; // CircularBuffer

