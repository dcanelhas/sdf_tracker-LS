/* part of Pyrolite, by Irmen de Jong (irmen@razorvine.net) */

using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;
using NUnit.Framework;
using Razorvine.Pickle;
using Razorvine.Pickle.Objects;

namespace Pyrolite.Tests.Pickle
{

/// <summary>
/// Unit tests for the unpickler. 
/// </summary>
[TestFixture]
public class UnpicklerTests {

	[TestFixtureSetUp]
	public void setUp()  {
	}

	[TestFixtureTearDown]
	public void tearDown() {
	}

	object U(string strdata) 
	{
		return U(PickleUtils.str2bytes(strdata));
	}
	object U(byte[] data) 
	{
		Unpickler u=new Unpickler();
		object o=u.loads(data);
		u.close();
		return o;		
	}
	
	
	[Test]
	public void testSinglePrimitives()  {
		// protocol level 1
		Assert.IsNull(U("N."));		// none
		Assert.AreEqual(123.456d, U("F123.456\n."));	// float
		Assert.AreEqual(true,U("I01\n."));	// true boolean
		Assert.AreEqual(false,U("I00\n."));	// false boolean
		Assert.AreEqual(new byte[]{97,98,99},(byte[]) U("c__builtin__\nbytes\np0\n((lp1\nL97L\naL98L\naL99L\natp2\nRp3\n.")); // python3 bytes
		Assert.AreEqual(new byte[]{97,98,99},(byte[]) U("c__builtin__\nbytes\n(](KaKbKcetR.")); // python3 bytes
		Assert.AreEqual(new byte[]{97,98,99,100,101,102}, (byte[]) U("C\u0006abcdef.")); // python3 bytes
		Assert.AreEqual(123,U("I123\n."));   // int
		Assert.AreEqual(999999999,U("I999999999\n."));   // int
		Assert.AreEqual(-999999999,U("I-999999999\n."));   // int
		Assert.AreEqual(9999999999L,U("I9999999999\n."));   // int (From 64-bit python)
		Assert.AreEqual(-9999999999L,U("I-9999999999\n."));   // int (From 64-bit python)
		Assert.AreEqual(19999999999L,U("I19999999999\n."));   // int (From 64-bit python)
		Assert.AreEqual(-19999999999L,U("I-19999999999\n."));   // int (From 64-bit python)
		Assert.AreEqual(0x45443043,U("JC0DE."));	// 4 byte signed int 0x45443043 (little endian)
		Assert.AreEqual(-285212674,U("J\u00fe\u00ff\u00ff\u00ee."));	// 4 byte signed int (little endian)
		Assert.AreEqual(255,U("K\u00ff."));   // unsigned int
		Assert.AreEqual(1234L,U("L1234\n.")); // long (as long)
		Assert.AreEqual(12345678987654321L,U("L12345678987654321L\n.")); // long (as long)
		// Assert.AreEqual(new BigInteger("9999888877776666555544443333222211110000"),U("L9999888877776666555544443333222211110000L\n.")); // long (as bigint)
		Assert.AreEqual(12345,U("M90."));    // 2 byte unsigned
		Assert.AreEqual(65535,U("M\u00ff\u00ff."));    // 2 byte unsigned
		Assert.AreEqual("Foobar",U("S'Foobar'\n."));  // string with quotes
		Assert.AreEqual("abc",U("T\u0003\u0000\u0000\u0000abc."));  // counted string
		Assert.AreEqual("abc",U("U\u0003abc."));  // short counted string
		Assert.AreEqual("unicode",U("Vunicode\n."));
		Assert.AreEqual("unicode",U("X\u0007\u0000\u0000\u0000unicode."));
		Assert.AreEqual(new Hashtable(),U("}."));
		Assert.AreEqual(new ArrayList(),U("]."));
		Assert.AreEqual(new object[0], (object[]) U(")."));
		Assert.AreEqual(1234.5678d, U("G@\u0093JEm\\\u00fa\u00ad."));  // 8-byte binary coded float
		// protocol level2
		Assert.AreEqual(true,U("\u0088."));	// True
		Assert.AreEqual(false,U("\u0089."));	// False
		//Assert.AreEqual(12345678987654321L, U("\u008a\u0007\u00b1\u00f4\u0091\u0062\u0054\u00dc\u002b."));
		//Assert.AreEqual(12345678987654321L, U("\u008b\u0007\u0000\u0000\u0000\u00b1\u00f4\u0091\u0062\u0054\u00dc\u002b."));
		// Protocol 3 (Python 3.x)
		Assert.AreEqual(new byte[]{65,66,67}, (byte[]) U("B\u0003\u0000\u0000\u0000ABC."));
		Assert.AreEqual(new byte[]{65,66,67}, (byte[]) U("C\u0003ABC."));
	}
	
	[Test]
	public void testZeroToTwoFiveSix() {
		byte[] bytes=new byte[256];
		for(int b=0; b<256; ++b) {
			bytes[b]=(byte)b;
		}
		StringBuilder sb=new StringBuilder();
		for(int i=0; i<256; ++i) {
			sb.Append((char)i);
		}
		string str=sb.ToString();
		
		Pickler p=new Pickler(false);
		Unpickler u=new Unpickler();
		
		MemoryStream bos=new MemoryStream(434);
		bos.WriteByte(Opcodes.PROTO); bos.WriteByte(2);
		byte[] data=Encoding.Default.GetBytes("c__builtin__\nbytearray\n");
		bos.Write(data,0,data.Length);
		bos.WriteByte(Opcodes.BINUNICODE);
		bos.Write(new byte[] {(byte)0x80,0x01,0x00,0x00},0,4);
		byte[] utf8=Encoding.UTF8.GetBytes(str);
		bos.Write(utf8,0,utf8.Length);
		bos.WriteByte(Opcodes.BINUNICODE);
		bos.Write(new byte[] {7,0,0,0},0,4);
		data=Encoding.Default.GetBytes("latin-1");
		bos.Write(data,0,data.Length);
		bos.WriteByte(Opcodes.TUPLE2);
		bos.WriteByte(Opcodes.REDUCE);
		bos.WriteByte(Opcodes.STOP);
		
		byte[] bytesresult=bos.ToArray();
		byte[] output=p.dumps(bytes);
		Assert.AreEqual(bytesresult, output);
		Assert.AreEqual(bytes, (byte[])u.loads(output)); 
		
		bos=new MemoryStream(434);
		bos.WriteByte(Opcodes.PROTO); bos.WriteByte(2);
		bos.WriteByte(Opcodes.BINUNICODE);
		bos.Write(new byte[] {(byte)0x80,0x01,0x00,0x00},0,4);
		utf8=Encoding.UTF8.GetBytes(str);
		bos.Write(utf8,0,utf8.Length);
		bos.WriteByte(Opcodes.STOP);
		bytesresult=bos.ToArray();

		output=p.dumps(str);
		Assert.AreEqual(bytesresult, output);
		Assert.AreEqual(str, u.loads(output));
	}

	[Test]
	public void testUnicodeStrings() 
	{
		Assert.AreEqual("\u00ff", U("S'\\xff'\n."));
		Assert.AreEqual("\u20ac", U("V\\u20ac\n."));
		
		Assert.AreEqual("euro\u20ac", U("X\u0007\u0000\u0000\u0000euro\u00e2\u0082\u00ac."));   // utf-8 encoded
		
		Assert.AreEqual("\u0007\u00db\u007f\u0080",U(new byte[]{(byte)'T',0x04,0x00,0x00,0x00,0x07,0xdb,0x7f,0x80,(byte)'.'}));  // string with non-ascii symbols
		Assert.AreEqual("\u0007\u00db\u007f\u0080",U(new byte[]{(byte)'U',0x04,0x07,0xdb,0x7f,0x80,(byte)'.'}));  // string with non-ascii symbols
		Assert.AreEqual("\u0007\u00db\u007f\u0080",U(new byte[]{(byte)'V',0x07,0xdb,0x7f,0x80,(byte)'\n',(byte)'.'}));  // string with non-ascii symbols
	}
	
	[Test]
	public void testTuples() 
	{
		Assert.AreEqual(new object[0], (object[])U(")."));	// ()
		Assert.AreEqual(new object[]{97}, (object[])U("Ka\u0085.")); // (97,)
		Assert.AreEqual(new object[]{97,98}, (object[])U("KaKb\u0086.")); // (97,98)
		Assert.AreEqual(new object[]{97,98,99}, (object[])U("KaKbKc\u0087.")); // (97,98,99)
		Assert.AreEqual(new object[]{97,98,99,100}, (object[])U("(KaKbKcKdt.")); // (97,98,99,100)
	}

	[Test]
	public void testLists() 
	{
		IList<int> list=new List<int>(0);
		
		Assert.AreEqual(list, U("]."));	// []
		list.Add(97);
		Assert.AreEqual(list, U("]Kaa."));	// [97]
		Assert.AreEqual(list, U("(Kal."));	// [97]
		list.Add(98);
		list.Add(99);
		Assert.AreEqual(list, U("](KaKbKce."));	// [97,98,99]
	}
	
	[Test]
	public void testDicts() 
	{
		Hashtable map=new Hashtable();
		Hashtable map2=new Hashtable();
		ArrayList list=new ArrayList();
		Assert.AreEqual(map, U("}.") );	// {}
		map.Add(97, 98);
		map.Add(99, 100);
		Assert.AreEqual(map, U("}(KaKbKcKdu."));  // {97: 98, 99: 100}
		Assert.AreEqual(map, U("(dI97\nI98\nsI99\nI100\ns.")); // {97: 98, 99: 100}
		Assert.AreEqual(map, U("(I97\nI98\nI99\nI100\nd.")); // {97: 98, 99: 100}
	
		map.Clear();
		map.Add(1,2);
		map.Add(3,4);
		map2.Add(5,6);
		map2.Add(7,8);
		list.Add(map);
		list.Add(map2);
		Assert.AreEqual(list, U("(lp0\n(dp1\nI1\nI2\nsI3\nI4\nsa(dp2\nI5\nI6\nsI7\nI8\nsa."));  // [{1:2, 3:4}, {5:6, 7:8}]
		Assert.AreEqual(list, U("\u0080\u0002]q\u0000(}q\u0001(K\u0001K\u0002K\u0003K\u0004u}q\u0002(K\u0005K\u0006K\u0007K\u0008ue."));  // [{1:2, 3:4}, {5:6, 7:8}]
		
		map.Clear();
		map2.Clear();
		list.Clear();
		
		map["abc"]=null;
		Assert.AreEqual(map, U("(dp0\nS'abc'\np1\nNs.")); // {'abc': None}
		Assert.AreEqual(map, U("\u0080\u0002}q\u0000U\u0003abcq\u0001Ns.")); // {'abc': None}
		map["abc"]=111;
		Assert.AreEqual(map, U("(dp0\nS'abc'\np1\nI111\ns.")); // {'abc': 111}
		Assert.AreEqual(map, U("\u0080\u0002}q\u0000U\u0003abcq\u0001Kos.")); // {'abc': 111}
		list.Add(111);
		list.Add(111);
		map["abc"]=list;
		AssertUtils.AssertEqual(map, U("(dp0\nS'abc'\np1\n(lp2\nI111\naI111\nas.")); // {'abc': [111,111]}
		AssertUtils.AssertEqual(map, U("\u0080\u0002}q\u0000U\u0003abcq\u0001]q\u0002(KoKoes.")); // {'abc': 111}
		map["abc"]=map2;
		AssertUtils.AssertEqual(map, U("(dp0\nS'abc'\np1\n(dp2\ns.")); // {'abc': {} }
		AssertUtils.AssertEqual(map, U("\u0080\u0002}q\u0000U\u0003abcq\u0001}q\u0002s.")); // {'abc': {} }
		map2["def"]=111;
		AssertUtils.AssertEqual(map, U("(dp0\nS'abc'\np1\n(dp2\nS'def'\np3\nI111\nss.")); // {'abc': {'def':111}}
		AssertUtils.AssertEqual(map, U("\u0080\u0002}q\u0000U\u0003abcq\u0001}q\u0002U\u0003defq\u0003Koss.")); // {'abc': {'def':111}}

		map2["def"]=list;
		AssertUtils.AssertEqual(map, U("(dp0\nS'abc'\np1\n(dp2\nS'def'\np3\n(lp4\nI111\naI111\nass.")); // {'abc': {'def': [111,111] }}
		AssertUtils.AssertEqual(map, U("\u0080\u0002}q\u0000U\u0003abcq\u0001}q\u0002U\u0003defq\u0003]q\u0004(KoKoess.")); // {'abc': {'def': [111,111] }}

		ArrayList list2=new ArrayList();
		list2.Add(222);
		list2.Add(222);
		map2["ghi"]=list2;
		AssertUtils.AssertEqual(map, U("(dp0\nS'abc'\np1\n(dp2\nS'ghi'\np3\n(lp4\nI222\naI222\nasS'def'\np5\n(lp6\nI111\naI111\nass.")); // {'abc': {'def': [111,111], ghi: [222,222] }}
		AssertUtils.AssertEqual(map, U("\u0080\u0002}q\u0000U\u0003abcq\u0001}q\u0002(U\u0003ghiq\u0003]q\u0004(K\u00deK\u00deeU\u0003defq\u0005]q\u0006(KoKoeus.")); // {'abc': {'def': [111,111], ghi: [222,222] }}

		map2.Clear();
		map2["def"]=list;
		map2["abc"]=list;
		AssertUtils.AssertEqual(map, U("(dp0\nS'abc'\np1\n(dp2\ng1\n(lp3\nI111\naI111\nasS'def'\np4\ng3\nss.")); // {'abc': {'def': [111,111], abc: [111,111] }}
		AssertUtils.AssertEqual(map, U("\u0080\u0002}q\u0000U\u0003abcq\u0001}q\u0002(h\u0001]q\u0003(KoKoeU\u0003defq\u0004h\u0003us.")); // {'abc': {'def': [111,111], abc: [111,111] }}
	}
	
	[Test]
	public void testComplex() 
	{
		ComplexNumber c=new ComplexNumber(2.0, 4.0);
		Assert.AreEqual(c, U("c__builtin__\ncomplex\np0\n(F2.0\nF4.0\ntp1\nRp2\n."));
		Assert.AreEqual(c, U("c__builtin__\ncomplex\nq\u0000G@\u0000\u0000\u0000\u0000\u0000\u0000\u0000G@\u0010\u0000\u0000\u0000\u0000\u0000\u0000\u0086q\u0001Rq\u0002."));
	}
	
	[Test]
	public void testDecimal() 
	{
		Assert.AreEqual(12345.6789m, U("cdecimal\nDecimal\np0\n(S'12345.6789'\np1\ntp2\nRp3\n."));
		Assert.AreEqual(12345.6789m, U("\u0080\u0002cdecimal\nDecimal\nU\n12345.6789\u0085R."));
	}

	[Test]
	public void testDateTime() 
	{
		DateTime dt;
		TimeSpan ts;
		
		dt=(DateTime)U("cdatetime\ndate\nU\u0004\u0007\u00db\u000c\u001f\u0085R.");
		Assert.AreEqual(new DateTime(2011,12,31), dt);

		ts=(TimeSpan)U("cdatetime\ntime\nU\u0006\u000e!;\u0006\u00f5@\u0085R.");
		Assert.AreEqual(new TimeSpan(0,14,33,59,456), ts);
		
		dt=(DateTime)U("cdatetime\ndatetime\nU\n\u0007\u00db\u000c\u001f\u000e!;\u0006\u00f5@\u0085R.");
		Assert.AreEqual(new DateTime(2011, 12, 31, 14, 33, 59, 456), dt);
		dt=(DateTime)U("cdatetime\ndatetime\np0\n(S'\\x07\\xdb\\x0c\\x1f\\x0e!;\\x06\\xf5@'\np1\ntp2\nRp3\n.");
		Assert.AreEqual(new DateTime(2011, 12, 31, 14, 33, 59, 456), dt);
		
		ts=(TimeSpan)U("cdatetime\ntimedelta\nM\u00d9\u0002M\u00d5\u00d2JU\u00f8\u0006\u0000\u0087R.");
		Assert.AreEqual(new TimeSpan(729, 0, 0, 53973, 456), ts);
	}
	
	[Test]
	public void testDateTimePython3() 
	{
		DateTime dt;
		TimeSpan ts;

		dt=(DateTime)U("cdatetime\ndate\nC\u0004\u0007\u00db\u000c\u001f\u0085R.");
		Assert.AreEqual(new DateTime(2011,12,31), dt);

		ts=(TimeSpan)U("cdatetime\ntime\nC\u0006\u000e!;\u0006\u00f5@\u0085R.");
		Assert.AreEqual(new TimeSpan(0,14,33,59,456), ts);

		dt=(DateTime)U("cdatetime\ndatetime\nC\n\u0007\u00db\u000c\u001f\u000e!;\u0006\u00f5@\u0085R.");
		Assert.AreEqual(new DateTime(2011, 12, 31, 14, 33, 59, 456), dt);
		
		ts=(TimeSpan)U("cdatetime\ntimedelta\nM\u00d9\u0002M\u00d5\u00d2JU\u00f8\u0006\u0000\u0087R.");
		Assert.AreEqual(new TimeSpan(729, 0, 0, 53973, 456), ts);
	}
	
	[Test]
	public void testBytes() 
	{
		byte[] bytes=new byte[]{1,2,127,(byte)128,(byte)255};
		Assert.AreEqual(bytes, (byte[])U("\u0080\u0003C\u0005\u0001\u0002\u007f\u0080\u00ffq\u0000."));
		Assert.AreEqual(bytes, (byte[])U("c__builtin__\nbytearray\np0\n(V\u0001\u0002\u007f\u0080\u00ff\np1\nS'latin-1'\np2\ntp3\nRp4\n."));
		bytes=new byte[]{1,2,3};
		Assert.AreEqual(bytes, (byte[])U("\u0080\u0002c__builtin__\nbytearray\nX\u0003\u0000\u0000\u0000\u0001\u0002\u0003X\u000a\u0000\u0000\u0000iso-8859-1\u0086R."));

		// the following bytecode pickle has been created in python by pickling a bytearray
		// from 0x00 to 0xff with protocol level 0.
		byte[] p0=new byte[] {
			0x63,0x5f,0x5f,0x62,0x75,0x69,0x6c,0x74,0x69,0x6e,0x5f,0x5f,0xa,0x62,0x79,0x74,0x65,0x61,0x72,0x72,0x61,0x79,0xa,0x70,0x30,0xa,0x28,0x56,
			0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0x5c,0x75,0x30,0x30,0x30,0x61,0xb,0xc,0xd,0xe,0xf,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,
			0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,
			0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,
			0x4d,0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x75,0x30,0x30,0x35,0x63,0x5d,0x5e,0x5f,0x60,0x61,
			0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,
			0x7c,0x7d,0x7e,0x7f,0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,
			0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,0xa0,0xa1,
			0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,
			0xba,0xbb,0xbc,0xbd,0xbe,0xbf,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,0xd0,0xd1,
			0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,
			0xea,0xeb,0xec,0xed,0xee,0xef,0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff,
			0xa,0x70,0x31,0xa,0x53,0x27,0x6c,0x61,0x74,0x69,0x6e,0x2d,0x31,0x27,0xa,0x70,0x32,0xa,0x74,0x70,0x33,0xa,0x52,0x70,0x34,0xa,0x2e
		};
		
		bytes=new byte[256];
		for(int i=0; i<256; ++i)
			bytes[i]=(byte)i;
		Assert.AreEqual(bytes, (byte[])U(p0));

		// the following bytecode pickle has been created in python by pickling a bytearray
		// from 0x00 to 0xff with protocol level 2.
		byte[] p2=new byte[] {
			0x80,0x2,0x63,0x5f,0x5f,0x62,0x75,0x69,0x6c,0x74,0x69,0x6e,0x5f,0x5f,0xa,0x62,0x79,0x74,0x65,0x61,0x72,0x72,0x61,
			0x79,0xa,0x71,0x0,0x58,0x80,0x1,0x0,0x0,0x0,0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,0xf,0x10,0x11,
			0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,
			0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,
			0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,
			0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,0x70,0x71,
			0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f,0xc2,0x80,0xc2,0x81,
			0xc2,0x82,0xc2,0x83,0xc2,0x84,0xc2,0x85,0xc2,0x86,0xc2,0x87,0xc2,0x88,0xc2,0x89,0xc2,0x8a,0xc2,0x8b,0xc2,0x8c,
			0xc2,0x8d,0xc2,0x8e,0xc2,0x8f,0xc2,0x90,0xc2,0x91,0xc2,0x92,0xc2,0x93,0xc2,0x94,0xc2,0x95,0xc2,0x96,0xc2,0x97,
			0xc2,0x98,0xc2,0x99,0xc2,0x9a,0xc2,0x9b,0xc2,0x9c,0xc2,0x9d,0xc2,0x9e,0xc2,0x9f,0xc2,0xa0,0xc2,0xa1,0xc2,0xa2,
			0xc2,0xa3,0xc2,0xa4,0xc2,0xa5,0xc2,0xa6,0xc2,0xa7,0xc2,0xa8,0xc2,0xa9,0xc2,0xaa,0xc2,0xab,0xc2,0xac,0xc2,0xad,
			0xc2,0xae,0xc2,0xaf,0xc2,0xb0,0xc2,0xb1,0xc2,0xb2,0xc2,0xb3,0xc2,0xb4,0xc2,0xb5,0xc2,0xb6,0xc2,0xb7,0xc2,0xb8,
			0xc2,0xb9,0xc2,0xba,0xc2,0xbb,0xc2,0xbc,0xc2,0xbd,0xc2,0xbe,0xc2,0xbf,0xc3,0x80,0xc3,0x81,0xc3,0x82,0xc3,0x83,
			0xc3,0x84,0xc3,0x85,0xc3,0x86,0xc3,0x87,0xc3,0x88,0xc3,0x89,0xc3,0x8a,0xc3,0x8b,0xc3,0x8c,0xc3,0x8d,0xc3,0x8e,
			0xc3,0x8f,0xc3,0x90,0xc3,0x91,0xc3,0x92,0xc3,0x93,0xc3,0x94,0xc3,0x95,0xc3,0x96,0xc3,0x97,0xc3,0x98,0xc3,0x99,
			0xc3,0x9a,0xc3,0x9b,0xc3,0x9c,0xc3,0x9d,0xc3,0x9e,0xc3,0x9f,0xc3,0xa0,0xc3,0xa1,0xc3,0xa2,0xc3,0xa3,0xc3,0xa4,
			0xc3,0xa5,0xc3,0xa6,0xc3,0xa7,0xc3,0xa8,0xc3,0xa9,0xc3,0xaa,0xc3,0xab,0xc3,0xac,0xc3,0xad,0xc3,0xae,0xc3,0xaf,
			0xc3,0xb0,0xc3,0xb1,0xc3,0xb2,0xc3,0xb3,0xc3,0xb4,0xc3,0xb5,0xc3,0xb6,0xc3,0xb7,0xc3,0xb8,0xc3,0xb9,0xc3,0xba,
			0xc3,0xbb,0xc3,0xbc,0xc3,0xbd,0xc3,0xbe,0xc3,0xbf,0x71,0x1,0x55,0x7,0x6c,0x61,0x74,0x69,0x6e,0x2d,0x31,0x71,0x2,0x86,0x71,0x3,0x52,0x71,0x4,0x2e
		};
		Assert.AreEqual(bytes, (byte[])U(p2));
	}
	
	[Test]
	public void testArray() 
	{
		// c=char -->char
		char[] testc=new char[]{'a','b','c'};
		char[] arrayc=(char[]) U("carray\narray\np0\n(S'c'\np1\n(lp2\nS'a'\np3\naS'b'\np4\nag1\natp5\nRp6\n.");
		Assert.AreEqual(testc,arrayc);
		testc=new char[]{'x','y','z'};
		arrayc=(char[])U("carray\narray\nU\u0001c](U\u0001xU\u0001yU\u0001ze\u0086R.");
		Assert.AreEqual(testc,arrayc);
		
		// u=unicode char -->char
		testc=new char[]{'a','b','c'};
		arrayc=(char[]) U("carray\narray\np0\n(S'u'\np1\n(lp2\nVa\np3\naVb\np4\naVc\np5\natp6\nRp7\n.");
		Assert.AreEqual(testc,arrayc);

		// b=signed char -->sbyte
		sbyte[] testsb=new sbyte[]{1,2,-1,-2};
		sbyte[] arraysb=(sbyte[]) U("carray\narray\np0\n(S'b'\np1\n(lp2\nI1\naI2\naI-1\naI-2\natp3\nRp4\n.");
		Assert.AreEqual(testsb,arraysb);

		// B=unsigned char -->byte
		byte[] testb=new byte[]{1,2,128,255};
		byte[] arrayb=(byte[]) U("carray\narray\np0\n(S'B'\np1\n(lp2\nI1\naI2\naI128\naI255\natp3\nRp4\n.");
		Assert.AreEqual(testb,arrayb);

		// h=signed short -->short
		short[] tests=new short[]{1,2,128,255,32700,-32700};
		short[] arrays=(short[]) U("carray\narray\np0\n(S'h'\np1\n(lp2\nI1\naI2\naI128\naI255\naI32700\naI-32700\natp3\nRp4\n.");
		Assert.AreEqual(tests,arrays);
		
		// H=unsigned short -->ushort
		ushort[] testus=new ushort[]{1,2,40000,65535};
		ushort[] arrayus=(ushort[]) U("carray\narray\np0\n(S'H'\np1\n(lp2\nI1\naI2\naI40000\naI65535\natp3\nRp4\n.");
		Assert.AreEqual(testus,arrayus);

		// i=signed integer -->int
		int[] testi=new int[]{1,2,999999999,-999999999};
		int[] arrayi=(int[]) U("carray\narray\np0\n(S'i'\np1\n(lp2\nI1\naI2\naI999999999\naI-999999999\natp3\nRp4\n.");
		Assert.AreEqual(testi,arrayi);
		
		// I=unsigned integer -->uint
		uint[] testui=new uint[]{1,2,999999999};
		uint[] arrayui=(uint[]) U("carray\narray\np0\n(S'I'\np1\n(lp2\nL1L\naL2L\naL999999999L\natp3\nRp4\n.");
		Assert.AreEqual(testui,arrayui);

		// l=signed long -->long
		long[] testl=new long[]{1,2,999999999999,-999999999999};
		long[] arrayl=(long[]) U("carray\narray\np0\n(S'l'\np1\n(lp2\nI1\naI2\naI999999999999\naI-999999999999\natp3\nRp4\n.");
		Assert.AreEqual(testl,arrayl);

		// L=unsigned long -->ulong
		ulong[] testul=new ulong[]{1,2,999999999999};
		ulong[] arrayul=(ulong[]) U("carray\narray\np0\n(S'L'\np1\n(lp2\nL1L\naL2L\naL999999999999L\natp3\nRp4\n.");
		Assert.AreEqual(testul,arrayul);

		// f=float 4
		float[] testf=new float[]{-4.4f, 4.4f};
		float[] arrayf=(float[]) U("carray\narray\np0\n(S'f'\np1\n(lp2\nF-4.400000095367432\naF4.400000095367432\natp3\nRp4\n.");
		Assert.AreEqual(testf,arrayf);

		// d=float 8
		double[] testd=new double[]{-4.4d, 4.4d};
		double[] arrayd=(double[]) U("carray\narray\np0\n(S'd'\np1\n(lp2\nF-4.4\naF4.4\natp3\nRp4\n.");
		Assert.AreEqual(testd,arrayd);
	}
	
	[Test]
	public void testArrayPython3() {
		// python 3 array reconstructor
		short[] testi=new short[]{1,2,3};
		short[] arrayi=(short[])U("\u0080\u0003carray\n_array_reconstructor\nq\u0000(carray\narray\nq\u0001X\u0001\u0000\u0000\u0000hq\u0002K\u0004C\u0006\u0001\u0000\u0002\u0000\u0003\u0000q\u0003tq\u0004Rq\u0005.");
							  
		Assert.AreEqual(testi, arrayi);
	}
	
	[Test]
	public void testSet() 
	{
		var set=new HashSet<object>();
		set.Add(1);
		set.Add(2);
		set.Add("abc");
		
		AssertUtils.AssertEqual(set, (HashSet<object>)U("c__builtin__\nset\np0\n((lp1\nI1\naI2\naS'abc'\np2\natp3\nRp4\n."));
	}
	
	[Test]
	public void testMemoing() 
	{
	    ArrayList list=new ArrayList();
        list.Add("irmen");	 
        list.Add("irmen");	 
        list.Add("irmen");	 
		Assert.AreEqual(list, U("]q\u0000(U\u0005irmenq\u0001h\u0001h\u0001e."));
		
		ArrayList a=new ArrayList();
		a.Add(111);
		ArrayList b=new ArrayList();
		b.Add(222);
		ArrayList c=new ArrayList();
		c.Add(333);
		
		object[] array=new object[] {a,b,c,a,b,c};
		Assert.AreEqual(array, (object[]) U("((lp0\nI111\na(lp1\nI222\na(lp2\nI333\nag0\ng1\ng2\ntp3\n."));
		
		list.Clear();
		list.Add("a");
		list.Add("b");
		list.Add(list);//recursive
		a=(ArrayList) U("(lp0\nS'a'\np1\naS'b'\np2\nag0\na.");
		Assert.AreEqual("[a, b, (this Collection), ]", PrettyPrint.printToString(a));
		a=(ArrayList) U("\u0080\u0002]q\u0000(U\u0001aq\u0001U\u0001bq\u0002h\u0000e.");
		Assert.AreEqual("[a, b, (this Collection), ]", PrettyPrint.printToString(a));
		a=(ArrayList)U("]q\u0000(]q\u0001(K\u0001K\u0002K\u0003e]q\u0002(h\u0001h\u0001ee.");
		Assert.AreEqual("[[1, 2, 3, ], [[1, 2, 3, ], [1, 2, 3, ], ], ]", PrettyPrint.printToString(a));
	}
	
	[Test]
	public void testBinint2WithObject() 
	{
		Unpickler u=new Unpickler();
		Unpickler.registerConstructor("Pyro4.core", "URI", new StringConstructor());
		byte[] data=PickleUtils.str2bytes("\u0080\u0002cPyro4.core\nURI\n)\u0081M\u0082#.");
		int result=(int) u.loads(data);
		Assert.AreEqual(9090,result);
	}
	
}

}
