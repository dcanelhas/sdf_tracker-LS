
package com.picklingtools.pythonesque;

// Compare two keys of a Tab (HashTable<Object, Object>), which
// allows keys to be Integers or Strings.  TODO: keys as tuples?

import java.util.Comparator;

public class ValComparator implements Comparator<Object> {
    public int compare(Object o1, Object o2) {
	if (o1 instanceof Integer) {
	    if (o2 instanceof Integer) {
		Integer i1 = (Integer)o1;
		Integer i2 = (Integer) o2;
		return i1.compareTo(i2);
	    }
	} else {
	    // Convert to string 
	    String s1 = o1.toString();
	    String s2 = o2.toString();
	    return s1.compareTo(s2);
	}
	return 0; // ??? Java signs this as an error?
    }
}; // ValComparator