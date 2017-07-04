/* part of Pyrolite, by Irmen de Jong (irmen@razorvine.net) */

using System;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.Text;
using NUnit.Framework;
using Razorvine.Pickle;

namespace Pyrolite.Tests.Pickle
{

/// <summary>
/// Unit tests for Unpickling every pickle opcode (protocol 0,1,2,3). 
/// </summary>
[TestFixture]
public class UnpickleOpcodesTests {

	Unpickler u;
	static string STRING256;
	static string STRING255;
	
	static UnpickleOpcodesTests() {
		StringBuilder sb=new StringBuilder();
		for(int i=0; i<256; ++i) {
			sb.Append((char)i);
		}
		STRING256=sb.ToString();
		STRING255=STRING256.Substring(1);
	}
	
	object U(string strdata) {
		return u.loads(PickleUtils.str2bytes(strdata));
	}
	
	[TestFixtureSetUp]
	public void setUp() {
		u=new Unpickler();
	}

	[TestFixtureTearDown]
	public void tearDown() {
		u.close();
	}

	[Test]
	public void testStr2Bytes() {
		byte[] bytes=PickleUtils.str2bytes(STRING256);
		for(int i=0; i<256; ++i) {
			int b=bytes[i];
			if(b<0) b+=256;
			Assert.AreEqual(i, b, "byte@"+i);
		}
	}
	
	[Test, ExpectedException(typeof(InvalidOpcodeException))]
	public void testNotExisting() {
		U("%.");  // non existing opcode '%' should crash
	}

	[Test]
	public void testMARK() {
		// MARK           = b'('   # push special markobject on stack
		Assert.IsNull(U("(((N."));
	}

	[Test, ExpectedException(typeof(ArgumentOutOfRangeException))]
	public void testSTOP() {
		//STOP           = b'.'   # every pickle ends with STOP
		U(".."); // a stop without any data on the stack will throw an array exception
	}

	[Test]
	public void testPOP() {
		//POP            = b'0'   # discard topmost stack item
		Assert.IsNull(U("}N."));
		Assert.AreEqual(new Hashtable(), U("}N0."));
	}

	[Test]
	public void testPOPMARK() {
		//POP_MARK       = b'1'   # discard stack top through topmost markobject
		Assert.AreEqual(2, U("I1\n(I2\n(I3\nI4\n1."));
	}

	[Test]
	public void testDUP() {
		//DUP            = b'2'   # duplicate top stack item
		object[] tuple=new object[] { 42,42};
		Assert.AreEqual(tuple, (object[]) U("(I42\n2t."));
	}

	[Test]
	public void testFLOAT() {
		//FLOAT          = b'F'   # push float object; decimal string argument
		Assert.AreEqual(0.0d, U("F0\n."));
		Assert.AreEqual(0.0d, U("F0.0\n."));
		Assert.AreEqual(1234.5678d, U("F1234.5678\n."));
		Assert.AreEqual(-1234.5678d, U("F-1234.5678\n."));
		Assert.AreEqual(2.345e+202d, U("F2.345e+202\n."));
		Assert.AreEqual(-2.345e-202d, U("F-2.345e-202\n."));
		
		try {
			U("F1,2\n.");
			Assert.Fail("expected numberformat exception");
		} catch (FormatException) {
			// ok
		}
	}

	[Test]
	public void testINT() {
		//INT            = b'I'   # push integer or bool; decimal string argument
		Assert.AreEqual(0, U("I0\n."));
		Assert.AreEqual(0, U("I-0\n."));
		Assert.AreEqual(1, U("I1\n."));
		Assert.AreEqual(-1, U("I-1\n."));
		Assert.AreEqual(1234567890, U("I1234567890\n."));
		Assert.AreEqual(-1234567890, U("I-1234567890\n."));
		Assert.AreEqual(1234567890123456L, U("I1234567890123456\n."));
		try {
			U("I123456789012345678901234567890\n.");
			Assert.Fail("expected overflow exception");
		} catch (OverflowException) {
			// ok
		}
		try {
			U("I123456789@012345678901234567890\n.");
			Assert.Fail("expected format exception");
		} catch (FormatException) {
			// ok
		}
	}

	[Test]
	public void testBININT() {
		//BININT         = b'J'   # push four-byte signed int (little endian)
		Assert.AreEqual(0, U("J\u0000\u0000\u0000\u0000."));
		Assert.AreEqual(1, U("J\u0001\u0000\u0000\u0000."));
		Assert.AreEqual(33554433, U("J\u0001\u0000\u0000\u0002."));
		Assert.AreEqual(-1, U("J\u00ff\u00ff\u00ff\u00ff."));
		Assert.AreEqual(-251658255, U("J\u00f1\u00ff\u00ff\u00f0."));
	}

	[Test]
	public void testBININT1() {
		//BININT1        = b'K'   # push 1-byte unsigned int
		Assert.AreEqual(0, U("K\u0000."));
		Assert.AreEqual(128, U("K\u0080."));
		Assert.AreEqual(255, U("K\u00ff."));
	}

	[Test]
	public void testLONG() {
		//LONG           = b'L'   # push long; decimal string argument
		Assert.AreEqual(0L, U("L0\n."));
		Assert.AreEqual(0L, U("L-0\n."));
		Assert.AreEqual(1L, U("L1\n."));
		Assert.AreEqual(-1L, U("L-1\n."));
		Assert.AreEqual(1234567890L, U("L1234567890\n."));
		Assert.AreEqual(1234567890123456L, U("L1234567890123456\n."));
		Assert.AreEqual(-1234567890123456L, U("L-1234567890123456\n."));
		//Assert.AreEqual(new BigInteger("1234567890123456789012345678901234567890"), U("L1234567890123456789012345678901234567890\n."));
		try {
			U("L1234567890123456789012345678901234567890\n.");
			Assert.Fail("expected pickle exception because c# doesn't have bigint");
		} catch (PickleException) {
			// ok
		}
		try {
			U("I1?0\n.");
			Assert.Fail("expected numberformat exception");
		} catch (FormatException) {
			// ok
		}
	}

	[Test]
	public void testBININT2() {
		//BININT2        = b'M'   # push 2-byte unsigned int (little endian)
		Assert.AreEqual(0, U("M\u0000\u0000."));
		Assert.AreEqual(255, U("M\u00ff\u0000."));
		Assert.AreEqual(32768, U("M\u0000\u0080."));
		Assert.AreEqual(65535, U("M\u00ff\u00ff."));
	}

	[Test]
	public void testNONE() {
		//NONE           = b'N'   # push None
		Assert.AreEqual(null, U("N."));
	}

	[Test, ExpectedException(typeof(InvalidOpcodeException))]
	public void testPERSID() {
		//PERSID         = b'P'   # push persistent object; id is taken from string arg
		U("Pbla\n."); // this opcode is not implemented and should raise an exception
	}

	[Test, ExpectedException(typeof(InvalidOpcodeException))]
	public void testBINPERSID() {
		//BINPERSID      = b'Q'   #  "       "         "  ;  "  "   "     "  stack
		U("I42\nQ."); // this opcode is not implemented and should raise an exception
	}

	[Test]
	public void testREDUCE_and_GLOBAL() {
		//GLOBAL         = b'c'   # push self.find_class(modname, name); 2 string args
		//REDUCE         = b'R'   # apply callable to argtuple, both on stack
		//"cdecimal\nDecimal\n(V123.456\ntR."
		decimal dec=123.456m;
		Assert.AreEqual(dec, (decimal)U("cdecimal\nDecimal\n(V123.456\ntR."));
	}

	[Test]
	public void testSTRING() {
		//STRING         = b'S'   # push string; NL-terminated string argument
		Assert.AreEqual("", U("S''\n."));
		Assert.AreEqual("", U("S\"\"\n."));
		Assert.AreEqual("a", U("S'a'\n."));
		Assert.AreEqual("a", U("S\"a\"\n."));
		Assert.AreEqual("\u00a1\u00a2\u00a3", U("S'\\xa1\\xa2\\xa3'\n."));
		Assert.AreEqual("a\\x00y", U("S'a\\\\x00y'\n."));
		
		StringBuilder p=new StringBuilder("S'");
		for(int i=0;i<256;++i) {
			p.Append("\\x");
			p.Append(i.ToString("X2"));
		}
		p.Append("'\n.");
		Assert.AreEqual(STRING256, U(p.ToString()));
		
		try {
			U("S'bla\n."); // missing quote
			Assert.Fail("expected pickle exception");
		} catch (PickleException) {
			//ok
		}
	}

	[Test]
	public void testBINSTRING() {
		//BINSTRING      = b'T'   # push string; counted binary string argument
		Assert.AreEqual("", U("T\u0000\u0000\u0000\u0000."));
		Assert.AreEqual("a", U("T\u0001\u0000\u0000\u0000a."));
		Assert.AreEqual("\u00a1\u00a2\u00a3", U("T\u0003\u0000\u0000\u0000\u00a1\u00a2\u00a3."));
		Assert.AreEqual(STRING256,U("T\u0000\u0001\u0000\u0000"+STRING256+"."));
		Assert.AreEqual(STRING256+STRING256,U("T\u0000\u0002\u0000\u0000"+STRING256+STRING256+"."));
	}

	[Test]
	public void testSHORT_BINSTRING() {
		//SHORT_BINSTRING= b'U'   #  push string; counted binary string argument < 256 bytes
		Assert.AreEqual("", U("U\u0000."));
		Assert.AreEqual("a", U("U\u0001a."));
		Assert.AreEqual("\u00a1\u00a2\u00a3", U("U\u0003\u00a1\u00a2\u00a3."));
		Assert.AreEqual(STRING255,U("U\u00ff"+STRING255+"."));
	}

	[Test]
	public void testUNICODE() {
		//UNICODE        = b'V'   # push Unicode string; raw-unicode-escaped'd argument
		Assert.AreEqual("", U("V\n."));
		Assert.AreEqual("abc", U("Vabc\n."));
		Assert.AreEqual("\u20ac", U("V\\u20ac\n."));
		Assert.AreEqual("a\\u00y", U("Va\\u005cu00y\n."));
		Assert.AreEqual("\u0080\u00a1\u00a2", U("V\u0080\u00a1\u00a2\n."));
	}

	[Test]
	public void testBINUNICODE() {
		//BINUNICODE     = b'X'   # push Unicode string; counted UTF-8 string argument
		Assert.AreEqual("", U("X\u0000\u0000\u0000\u0000."));
		Assert.AreEqual("abc", U("X\u0003\u0000\u0000\u0000abc."));
		Assert.AreEqual("\u20ac", u.loads(new byte[]{Opcodes.BINUNICODE, 0x03,0x00,0x00,0x00,(byte)0xe2,(byte)0x82,(byte)0xac,Opcodes.STOP}));
	}

	[Test]
	public void testAPPEND() {
		//APPEND         = b'a'   # append stack top to list below it
		ArrayList list=new ArrayList();
		list.Add(42);
		list.Add(43);
		Assert.AreEqual(list, U("]I42\naI43\na."));
	}

	
	public class ThingyWithSetstate {
		public string a;
		public ThingyWithSetstate(string param) {
			a=param;
		}
		public void __setstate__(Hashtable values) {
			a=(string)values["a"];
		}
	}
	class ThingyConstructor : IObjectConstructor {

		public object construct(object[] args) {
			return new ThingyWithSetstate((string)args[0]);
		}
	}

	[Test]
	public void testBUILD() {
		//BUILD          = b'b'   # call __setstate__ or __dict__.update()
		Unpickler.registerConstructor("unittest", "Thingy", new ThingyConstructor());
		// create a thing with initial value for the field 'a',
		// the use BUILD to __setstate__() it with something else ('foo').
		ThingyWithSetstate thingy = (ThingyWithSetstate) U("cunittest\nThingy\n(V123\ntR}S'a'\nS'foo'\nsb.");
		Assert.AreEqual("foo",thingy.a);
	}

	[Test]
	public void testDICT() {
		//DICT           = b'd'   # build a dict from stack items
		var dict=new Hashtable();
		dict["a"]=42;
		dict["b"]=99;
		Assert.AreEqual(dict, U("(S'a'\nI42\nS'b'\nI99\nd."));
	}

	[Test]
	public void testEMPTY_DICT() {
		//EMPTY_DICT     = b'}'   # push empty dict
		Assert.AreEqual(new Hashtable(), U("}."));
	}

	[Test]
	public void testAPPENDS() {
		//APPENDS        = b'e'   # extend list on stack by topmost stack slice
		ArrayList list=new ArrayList();
		list.Add(42);
		list.Add(43);
		Assert.AreEqual(list, U("](I42\nI43\ne."));
	}

	[Test]
	public void testGET_and_PUT() {
		//GET            = b'g'   # push item from memo on stack; index is string arg
		//PUT            = b'p'   # store stack top in memo; index is string arg
		
		// a list with three times the same string in it.
		// the string is stored ('p') and retrieved from the memo ('g').
		var list=new List<string>();
		string str="abc";
		list.Add(str);
		list.Add(str);
		list.Add(str);
		Assert.AreEqual(list, U("(lp0\nS'abc'\np1\nag1\nag1\na."));

		try {
			U("(lp0\nS'abc'\np1\nag2\nag2\na."); // invalid memo key
			Assert.Fail("expected pickle exception");
		} catch (PickleException) {
			// ok
		}
	}

	[Test]
	public void testBINGET_and_BINPUT() {
		//BINGET         = b'h'   # push item from memo on stack; index is 1-byte arg
		//BINPUT         = b'q'   # store stack top in memo; index is 1-byte arg
		// a list with three times the same string in it.
		// the string is stored ('p') and retrieved from the memo ('g').
		var list=new List<string>();
		string str="abc";
		list.Add(str);
		list.Add(str);
		list.Add(str);
		Assert.AreEqual(list, U("]q\u0000(U\u0003abcq\u0001h\u0001h\u0001e."));

		try {
			U("]q\u0000(U\u0003abcq\u0001h\u0002h\u0002e."); // invalid memo key
			Assert.Fail("expected pickle exception");
		} catch (PickleException) {
			// ok
		}
	}

	[Test, ExpectedException(typeof(InvalidOpcodeException))]
	public void testINST() {
		//INST           = b'i'   # build & push class instance
		U("i.");
	}

	[Test]
	public void testLONG_BINGET_and_LONG_BINPUT() {
		//LONG_BINGET    = b'j'   # push item from memo on stack; index is 4-byte arg
		//LONG_BINPUT    = b'r'   # store stack top in memo; index is 4-byte arg
		// a list with three times the same string in it.
		// the string is stored ('p') and retrieved from the memo ('g').
		var list=new List<string>();
		string str="abc";
		list.Add(str);
		list.Add(str);
		list.Add(str);
		Assert.AreEqual(list, U("]r\u0000\u0000\u0000\u0000(U\u0003abcr\u0001\u0002\u0003\u0004j\u0001\u0002\u0003\u0004j\u0001\u0002\u0003\u0004e."));

		try {
			// invalid memo key
			U("]r\u0000\u0000\u0000\u0000(U\u0003abcr\u0001\u0002\u0003\u0004j\u0001\u0005\u0005\u0005j\u0001\u0005\u0005\u0005e.");
			Assert.Fail("expected pickle exception");
		} catch (PickleException) {
			// ok
		}
	}

	[Test]
	public void testLIST() {
		//LIST           = b'l'   # build list from topmost stack items
		var list=new List<int>();
		list.Add(1);
		list.Add(2);
		Assert.AreEqual(list, U("(I1\nI2\nl."));
	}

	[Test]
	public void testEMPTY_LIST() {
		//EMPTY_LIST     = b']'   # push empty list
		Assert.AreEqual(new ArrayList(), U("]."));
	}

	[Test, ExpectedException(typeof(InvalidOpcodeException))]
	public void testOBJ() {
		//OBJ            = b'o'   # build & push class instance
		U("o.");
	}

	[Test]
	public void testSETITEM() {
		//SETITEM        = b's'   # add key+value pair to dict
		var dict=new Hashtable();
		dict["a"]=42;
		dict["b"]=43;
		Assert.AreEqual(dict, U("}S'a'\nI42\nsS'b'\nI43\ns."));
	}

	[Test]
	public void testTUPLE() {
		//TUPLE          = b't'   # build tuple from topmost stack items
		object[] tuple=new object[] {1,2};
		Assert.AreEqual(tuple, (object[]) U("(I1\nI2\nt."));
	}

	[Test]
	public void testEMPTY_TUPLE() {
		//EMPTY_TUPLE    = b')'   # push empty tuple
		Assert.AreEqual(new object[0], (object[]) U(")."));
	}

	[Test]
	public void testSETITEMS() {
		//SETITEMS       = b'u'   # modify dict by adding topmost key+value pairs
		var dict=new Hashtable();
		dict["b"]=43;
		dict["c"]=44;
		Assert.AreEqual(dict, U("}(S'b'\nI43\nS'c'\nI44\nu."));

		dict.Clear();
		dict["a"]=42;
		dict["b"]=43;
		dict["c"]=44;
		Assert.AreEqual(dict, U("}S'a'\nI42\ns(S'b'\nI43\nS'c'\nI44\nu."));
	}

	[Test]
	public void testBINFLOAT() {
		//BINFLOAT       = b'G'   # push float; arg is 8-byte float encoding
		Assert.AreEqual(2.345e123, u.loads(new byte[]{Opcodes.BINFLOAT, 0x59,(byte)0x8c,0x60,(byte)0xfb,(byte)0x80,(byte)0xae,0x2f,(byte)0xbb, Opcodes.STOP}));
		Assert.AreEqual(1.172419264827552e+123, u.loads(new byte[]{Opcodes.BINFLOAT, 0x59,0x7c,0x60,0x7b,0x70,0x7e,0x2f,0x7b, Opcodes.STOP}));
		Assert.AreEqual(Double.PositiveInfinity, u.loads(new byte[]{Opcodes.BINFLOAT, (byte)0x7f,(byte)0xf0,0,0,0,0,0,0, Opcodes.STOP}));
		Assert.AreEqual(Double.NegativeInfinity, u.loads(new byte[]{Opcodes.BINFLOAT, (byte)0xff,(byte)0xf0,0,0,0,0,0,0, Opcodes.STOP}));
		Assert.AreEqual(Double.NaN, u.loads(new byte[]{Opcodes.BINFLOAT, (byte)0xff,(byte)0xf8,0,0,0,0,0,0, Opcodes.STOP}));
	}
	

	[Test]
	public void testTRUE() {
		//TRUE           = b'I01\n'  # not an opcode; see INT docs in pickletools.py
		Assert.IsTrue((bool) U("I01\n."));
	}

	[Test]
	public void testFALSE() {
		//FALSE          = b'I00\n'  # not an opcode; see INT docs in pickletools.py
		Assert.IsFalse((bool) U("I00\n."));
	}

	[Test]
	public void testPROTO() {
		//PROTO          = b'\x80'  # identify pickle protocol
		U("\u0080\u0000N.");
		U("\u0080\u0001N.");
		U("\u0080\u0002N.");
		U("\u0080\u0003N.");
		try {
			U("\u0080\u0004N."); // unsupported protocol 4.
			Assert.Fail("expected pickle exception");
		} catch (PickleException) {
			// ok
		}
	}

	[Test]
	public void testNEWOBJ() {
		//NEWOBJ         = b'\x81'  # build object by applying cls.__new__ to argtuple
		//GLOBAL         = b'c'   # push self.find_class(modname, name); 2 string args
		//"cdecimal\nDecimal\n(V123.456\nt\x81."
		decimal dec=123.456m;
		Assert.AreEqual(dec, (decimal)U("cdecimal\nDecimal\n(V123.456\nt\u0081."));
	}

	[Test, ExpectedException(typeof(InvalidOpcodeException))]
	public void testEXT1() {
		//EXT1           = b'\x82'  # push object from extension registry; 1-byte index
		U("\u0082\u0001."); // not implemented
	}

	[Test, ExpectedException(typeof(InvalidOpcodeException))]
	public void testEXT2() {
		//EXT2           = b'\x83'  # ditto, but 2-byte index
		U("\u0083\u0001\u0002."); // not implemented
	}

	[Test, ExpectedException(typeof(InvalidOpcodeException))]
	public void testEXT4() {
		//EXT4           = b'\x84'  # ditto, but 4-byte index
		U("\u0084\u0001\u0002\u0003\u0004."); // not implemented
	}

	[Test]
	public void testTUPLE1() {
		//TUPLE1         = b'\x85'  # build 1-tuple from stack top
		object[] tuple=new object[] { 42 };
		Assert.AreEqual(tuple, (object[]) U("I41\nI42\n\u0085."));
	}

	[Test]
	public void testTUPLE2() {
		//TUPLE2         = b'\x86'  # build 2-tuple from two topmost stack items
		object[] tuple=new object[] { 42, 43 };
		Assert.AreEqual(tuple, (object[]) U("I41\nI42\nI43\n\u0086."));
	}

	[Test]
	public void testTUPLE3() {
		//TUPLE3         = b'\x87'  # build 3-tuple from three topmost stack items
		object[] tuple=new object[] { 42, 43, 44 };
		Assert.AreEqual(tuple, (object[]) U("I41\nI42\nI43\nI44\n\u0087."));
	}

	[Test]
	public void testNEWTRUE() {
		//NEWTRUE        = b'\x88'  # push True
		Assert.IsTrue((bool) U("\u0088."));
	}

	[Test]
	public void testNEWFALSE() {
		//NEWFALSE       = b'\x89'  # push False
		Assert.IsFalse((bool) U("\u0089."));
	}

	[Test]
	public void testLONG1() {
		//LONG1          = b'\x8a'  # push long from < 256 bytes
		Assert.AreEqual(0L, U("\u008a\u0000."));
		Assert.AreEqual(0L, U("\u008a\u0001\u0000."));
		Assert.AreEqual(1L, U("\u008a\u0001\u0001."));
		Assert.AreEqual(-1L, U("\u008a\u0001\u00ff."));
		Assert.AreEqual(0L, U("\u008a\u0002\u0000\u0000."));
		Assert.AreEqual(1L, U("\u008a\u0002\u0001\u0000."));
		Assert.AreEqual(513L, U("\u008a\u0002\u0001\u0002."));
		Assert.AreEqual(-256L, U("\u008a\u0002\u0000\u00ff."));
		Assert.AreEqual(65280L, U("\u008a\u0003\u0000\u00ff\u0000."));

		Assert.AreEqual(0x12345678L, U("\u008a\u0004\u0078\u0056\u0034\u0012."));
		Assert.AreEqual(-231451016L, U("\u008a\u0004\u0078\u0056\u0034\u00f2."));
		Assert.AreEqual(0xf2345678L, U("\u008a\u0005\u0078\u0056\u0034\u00f2\u0000."));

		Assert.AreEqual(0x0102030405060708L, u.loads(new byte[] {(byte)Opcodes.LONG1,0x08,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,Opcodes.STOP}));
		//BigInteger big=new BigInteger("010203040506070809",16);
		try {
			u.loads(new byte[] {(byte)Opcodes.LONG1,0x09,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,Opcodes.STOP});
			Assert.Fail("expected PickleException due to number overflow");
		} catch (PickleException) {
			// ok
		}
	}

	[Test]
	public void testLONG4() {
		//LONG4          = b'\x8b'  # push really big long
		Assert.AreEqual(0L, u.loads(new byte[] {(byte)Opcodes.LONG4, 0x00, 0x00, 0x00, 0x00, Opcodes.STOP}));
		Assert.AreEqual(0L, u.loads(new byte[] {(byte)Opcodes.LONG4, 0x01, 0x00, 0x00, 0x00, 0x00, Opcodes.STOP}));
		Assert.AreEqual(1L, u.loads(new byte[] {(byte)Opcodes.LONG4, 0x01, 0x00, 0x00, 0x00, 0x01, Opcodes.STOP}));
		Assert.AreEqual(-1L, u.loads(new byte[] {(byte)Opcodes.LONG4, 0x01, 0x00, 0x00, 0x00, (byte)0xff, Opcodes.STOP}));
		Assert.AreEqual(0L, u.loads(new byte[] {(byte)Opcodes.LONG4, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, Opcodes.STOP}));
		Assert.AreEqual(1L, u.loads(new byte[] {(byte)Opcodes.LONG4, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, Opcodes.STOP}));
		Assert.AreEqual(513L, u.loads(new byte[] {(byte)Opcodes.LONG4, 0x02, 0x00, 0x00, 0x00, 0x01, 0x02, Opcodes.STOP}));
		Assert.AreEqual(-256L, u.loads(new byte[] {(byte)Opcodes.LONG4, 0x02, 0x00, 0x00, 0x00, 0x00, (byte)0xff, Opcodes.STOP}));
		Assert.AreEqual(65280L, u.loads(new byte[] {(byte)Opcodes.LONG4, 0x03, 0x00, 0x00, 0x00, 0x00, (byte)0xff, 0x00, Opcodes.STOP}));

		Assert.AreEqual(0x12345678L, U("\u008b\u0004\u0000\u0000\u0000\u0078\u0056\u0034\u0012."));
		Assert.AreEqual(-231451016L, U("\u008b\u0004\u0000\u0000\u0000\u0078\u0056\u0034\u00f2."));
		Assert.AreEqual(0xf2345678L, U("\u008b\u0005\u0000\u0000\u0000\u0078\u0056\u0034\u00f2\u0000."));

		Assert.AreEqual(0x0102030405060708L, u.loads(new byte[] {(byte)Opcodes.LONG4,0x08, 0x00, 0x00, 0x00,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,Opcodes.STOP}));
		//BigInteger big=new BigInteger("010203040506070809",16);
		try {
			u.loads(new byte[] {(byte)Opcodes.LONG4,0x09, 0x00, 0x00, 0x00,0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01,Opcodes.STOP});
			Assert.Fail("expected PickleException due to number overflow");
		} catch (PickleException) {
			// ok
		}
	}

	[Test]
	public void testBINBYTES() {
		//BINBYTES       = b'B'   # push bytes; counted binary string argument
		byte[] bytes;
		bytes=new byte[]{};
		Assert.AreEqual(bytes, (byte[]) U("B\u0000\u0000\u0000\u0000."));
		bytes=new byte[]{(byte)'a'};
		Assert.AreEqual(bytes, (byte[]) U("B\u0001\u0000\u0000\u0000a."));
		bytes=new byte[]{(byte)0xa1, (byte)0xa2, (byte)0xa3};
		Assert.AreEqual(bytes, (byte[]) U("B\u0003\u0000\u0000\u0000\u00a1\u00a2\u00a3."));
		bytes=new byte[512];
		for(int i=1; i<512; ++i) {
			bytes[i]=(byte)(i&0xff);
		}
		Assert.AreEqual(bytes, (byte[]) U("B\u0000\u0002\u0000\u0000"+STRING256+STRING256+"."));
	}

	[Test]
	public void testSHORT_BINBYTES() {
		//SHORT_BINBYTES = b'C'   #  push bytes; counted binary string argument < 256 bytes
		byte[] bytes;
		bytes=new byte[]{};
		Assert.AreEqual(bytes, (byte[]) U("C\u0000."));
		bytes=new byte[]{(byte)'a'};
		Assert.AreEqual(bytes, (byte[]) U("C\u0001a."));
		bytes=new byte[]{(byte)0xa1, (byte)0xa2, (byte)0xa3};
		Assert.AreEqual(bytes, (byte[]) U("C\u0003\u00a1\u00a2\u00a3."));
		bytes=new byte[255];
		for(int i=1; i<256; ++i) {
			bytes[i-1]=(byte)i;
		}
		Assert.AreEqual(bytes, (byte[]) U("C\u00ff"+STRING255+"."));
	}

}

}
