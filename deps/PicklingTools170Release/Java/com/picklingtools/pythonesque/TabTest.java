
package com.picklingtools.pythonesque;

import java.io.IOException;

public class TabTest {
    public static void main(String [] args) throws IOException {

	System.out.println("***Empty Table");
	Tab t = new Tab();
	t.prettyPrint(System.out);
	System.out.println();

	System.out.println("**Simple 1 level table");
	t.put("a", 1);
	t.put("b", 2.2);
	t.put("c", "three");
	Object c = null;
	t.put("empty", c);
	t.prettyPrint(System.out);
	System.out.println();
	System.out.println(t);

	System.out.println("**** Nest tables and try pretty print");
	Tab n = new Tab();
	n.put("x", 1.234567890123456789);
	n.put("y", "hello");
	Tab n2 = new Tab();
	n2.put("zzz", 666);
	n.put("nnn", n2);
	t.put("nested", n);
	t.prettyPrint(System.out);
	System.out.println();
	System.out.println(t);

	System.out.println("**** normal get test");
	Object o1 = t.get("a");  // normal lookup
	System.out.println(o1);
	System.out.println("**** get out as int test");
	int simpler = (Integer)t.get("a");
	System.out.println(simpler);

	System.out.println("**** normal get test NOT THERE (return null)");
	Object ooo = t.get("NOTHERE");  // normal lookup
	System.out.println(ooo);
	Ptools.prettyPrint(System.out, ooo);
	System.out.println();

	System.out.println("**** get cascade lookup");
	Object o2 = t.get("nested", "x");  // cascading lookups
	System.out.println(o2);

	System.out.println("**** get more cascade lookup");
	Object o3 = t.get("nested", "nnn", "zzz");  // cascading lookups
	System.out.println(o3);

	System.out.println("**** get more cascade lookup NOT THERE");
	try {
	    Object o4 = t.get("nested", "nnn", "zzz", "NOT THERE");  // cascading lookups
	    System.out.println(o4);
	} catch (ClassCastException e) {
	    System.out.println("... okay.  Expected NOT THERE to not be there");
	}

    }

};