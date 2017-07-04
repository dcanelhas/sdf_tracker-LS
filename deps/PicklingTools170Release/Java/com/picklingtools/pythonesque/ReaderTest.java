


package com.picklingtools.pythonesque;

import java.io.IOException;
import java.text.ParseException;

public class ReaderTest {

    public static void main (String [] args) {

	try {
	    
	    Tab t = new Tab();
	    t.prettyPrint(System.out);
	    System.out.println();
	    try {
		t = new Tab("{ }");
		t.prettyPrint(System.out);
		System.out.println();
		
	    } catch (ParseException pe) {
		System.out.println(pe);
	    }
	    

	    try {
		t = new Tab("{'a':1 }");
		t.prettyPrint(System.out);
		System.out.println();
	    } catch (ParseException pe) {
		System.out.println(pe);
	    }
	    
	    try {
		t = new Tab("{'a':1, 'b':2.2, 'c':'three' }");
		t.prettyPrint(System.out);
		System.out.println();
		
		int i = (Integer)t.get("a");
		System.out.println("a = " + i);
		
		double d = (Double)t.get("b");
		System.out.println("b = " + d);
		
		String s = (String)t.get("c");
		System.out.println("c = " + s);
		
	    } catch (ParseException pe) {
		System.out.println(pe);
	    }
	    
	    try {
		t = new Tab("{'a':1, 'nest':{'aa':1, 'bb':2.2, 'cc':'three' }}");
		t.prettyPrint(System.out);
		System.out.println();
		
		// get separately
		Tab tt = (Tab)t.get("nest");
		int i = (Integer)tt.get("aa");
		System.out.println("aa = " + i);
		
		double d = (Double)tt.get("bb");
		System.out.println("bb = " + d);
		
		String s = (String)tt.get("cc");
		System.out.println("c = " + s);
		
		// nested get
		System.out.println(t.get("nest", "aa"));
		System.out.println(t.get("nest", "bb"));
		System.out.println(t.get("nest", "cc"));
		
	    } catch (ParseException pe) {
		System.out.println(pe);
	    }
	    
	    try {
		t = new Tab("{'a':1111111111111111111111111111111L}");
		t.prettyPrint(System.out);
		System.out.println();
		
	    } catch (ParseException pe) {
		System.out.println(pe);
	    }
	    
	    
	    try {
		t = new Tab("{'a':1e45}");
		t.prettyPrint(System.out);
		System.out.println();
		t = new Tab("{'a':1e-45}");
		t.prettyPrint(System.out);
		System.out.println();
		t = new Tab("{'a':-1e-45}");
		t.prettyPrint(System.out);
		System.out.println();
		t = new Tab("{'a':+1e-45}");
		t.prettyPrint(System.out);
		System.out.println();
		t = new Tab("{'a':-1.e-45}");
		t.prettyPrint(System.out);
		System.out.println();
		t = new Tab("{'a':-1.0e-45}");
		t.prettyPrint(System.out);
		System.out.println();
		
		
	    } catch (ParseException pe) {
		System.out.println(pe);
	    }
	    
	    try {
		t = new Tab("{'a':inf}");
		t.prettyPrint(System.out);
		System.out.println();
		
		t = new Tab("{'a':-inf}");
		t.prettyPrint(System.out);
		System.out.println();
		
		t = new Tab("{'a':+inf}");
		t.prettyPrint(System.out);
		System.out.println();
		
		t = new Tab("{'a':nan}");
		t.prettyPrint(System.out);
		System.out.println();
		
		t = new Tab("{ "+
			    "   'A':1, "+
			    "   'timedelta': 0.7834578923,"+
			    "   'mode':'tdoa',"+
			    "   'empty':None," + 
			    "   'valid':True, "+
			    "   'invalid': False, "+
			    "   'byte':65, "+
			    "   'char':65 "+
			    "}");
		t.prettyPrint(System.out);
		System.out.println();
		
	    } catch (ParseException pe) {	    
		System.out.println(pe);
	    }
	    
	    Arr a = new Arr();
	    a.prettyPrint(System.out);
	    System.out.println();
	    
	    try {
		a = new Arr("[1, 2.2, 'three']");
		a.prettyPrint(System.out);
		System.out.println();
	    } catch (ParseException pe) {
		System.out.println(pe);
	    }
	    
	    try {
		a = new Arr("[1, 2.2, 'three', []]");
		a.prettyPrint(System.out);
		System.out.println();
		
		a = new Arr("[1, 2.2, 'three', [11111111111111111111111L]]");
		a.prettyPrint(System.out);
		System.out.println();
		
		a = new Arr("[1, 2.2, 'three', \"abc'def'\", ['abc\"\"', {}, '']]");
		a.prettyPrint(System.out);
		System.out.println();
	    } catch (ParseException pe) {
		System.out.println(pe);
	    }
	    
	    try {
		a = new Arr("[1, 2.2, 'three', [5,6,7]]");
		a.prettyPrint(System.out);
		System.out.println();
		
		System.out.println(a.get(0));
		System.out.println();
		
		System.out.println(a.get(3, 0));
		System.out.println();
		
		
		
	    } catch (ParseException pe) {
		System.out.println(pe);
	    }
	    
	    try {
		a = new Arr("[1, 2.2, 'three', [5,6,7]]");
		a.prettyPrint(System.out);
		System.out.println();
		
		a.put(0, 100);
		a.prettyPrint(System.out);
		System.out.println();
		
		a.put(3, 0, 100);
		a.prettyPrint(System.out);
		System.out.println();
		
	    } catch (ParseException pe) {
		System.out.println(pe);
	    }
	    
	    try {
		a = new Arr("[1, 2.2, 'three', {'a':1, 'b':{'c':[1,2,3]}}]");
		a.prettyPrint(System.out);
		System.out.println();
		
		a.put(0, 100);
		a.prettyPrint(System.out);
		System.out.println();
		
		//a.put(3, "b", 666);
		a.put(3, "b", "c", 0, 666);
		a.prettyPrint(System.out);
		System.out.println();
		
		System.out.println(a);
		
	    } catch (ParseException pe) {
		System.out.println(pe);
	    }
	    
	    try {
		Ptools.WritePythonTextFile("test.arr", a);
		Ptools.WritePythonTextFile("test.tab", t);
	    } catch (Exception e) {
		System.out.println(e);
	    }
	    
	    try {
		a = (Arr)Ptools.ReadPythonTextFile("test.arr");
		System.out.println(a);
		
		t = (Tab)Ptools.ReadPythonTextFile("test.tab");
		System.out.println(t);
		
	    } catch (Exception e) {
		System.out.println(e);
	    }
	    

	    try {
		t = new Tab("{0.2:1}");
		a.prettyPrint(System.out);
		System.out.println();
		
	    } catch (ClassCastException pe) {
		System.out.println(pe);
	    } catch (ParseException pe) {
		System.out.println(pe);
	    }

	    try {
		t = new Tab();
		t.prettyPrint(System.out);
		System.out.println();
		
		t.put(2.2, 100);
		t.prettyPrint(System.out);
		System.out.println();

	    } catch (ClassCastException cce) {
		System.out.println(cce);
	    }

	    try {
		t = new Tab("{'a':{}}");
		t.prettyPrint(System.out);
		System.out.println();
		System.out.println("HEY!"+t.toString(true));
		System.out.println("HEY!"+t.toString(false));
		System.out.println("HEY!"+t.toString());

		a = new Arr("[1,2.2, 3.3]");
		System.out.println(a.toString(true));
		System.out.println(a.toString(false));
		System.out.println(a.toString());
		
		t.put("a", 2.2, 100);
		t.prettyPrint(System.out);
		System.out.println();


	    } catch (ClassCastException cce) {
		System.out.println(cce);
	    } catch (ParseException pe) {
		System.out.println(pe);
	    }

	    int ia[] = new int[33];
	    ia[0] = 1;
	    ia[1] = 2;
	    System.out.println(ia[0]);
	    System.out.println(ia.getClass());
	    System.out.println(ia instanceof int[]);
	    Object oo = (Object)ia;
	    System.out.println(oo instanceof int[]);
	    short aa[] = new short[11];
	    System.out.println(aa.getClass());
	    ReaderTest[] rr = new ReaderTest[11];
	    System.out.println(rr.getClass());
	    long ll[] = new long[22];
	    System.out.println(ll.getClass());
	    Integer lll[] = new Integer[12];
	    System.out.println(lll);
	    int hhh[][] = new int[33][33];
	    System.out.println(hhh.getClass());

	    System.out.println("HETRE!");

	    String s = "A\377";
	    System.out.println("HETRE2!"+ s);
	    //byte[] b = s.getBytes("UTF-8");
	    byte b[] = new byte[s.length()];
	    for (int ii=0; ii<s.length(); ii++) {
		b[ii] = (byte)s.charAt(ii);
	    }
	    System.out.println("len = "+b.length);
	    for (int ii=0; ii<b.length; ii++) {
		int temp = b[ii];
		System.out.println(temp);
	    }

	    try {
		t = new Tab("{'b':1}");
		int[] iia = {-2147483648, 2147483647, 0};
		t.put("iia", iia);
		short[] sia = {32767, -32768, 0};
		t.put("sia", sia);
		long[] lia = {55, 66, 77, -9223372036854775808L, 9223372036854775807L };
		t.put("lia", lia);
		float[] fia = {55f, -666.0f, 777.7f, 123456789.123456789f};
		t.put("fia", fia);
		double[] dia = {55, -666.0, 777.7, 123456789.123456789};
		t.put("dia", dia);
		byte[] bia = {0, 127, -128 };
		t.put("bia", bia);
		t.prettyPrint(System.out);
		System.out.println();
		
		Ptools.WritePythonTextFile("test.arrays", t);
		Tab ooo = (Tab)Ptools.ReadPythonTextFile("test.arrays");
		ooo.prettyPrint(System.out);
		System.out.println();

	    } catch (ClassCastException pe) {
		System.out.println(pe);
	    } catch (ParseException pe) {
		System.out.println(pe);
	    }

	}
	catch (IOException e) {
	    System.out.println(e);
	}
    }

}; // ReaderTest