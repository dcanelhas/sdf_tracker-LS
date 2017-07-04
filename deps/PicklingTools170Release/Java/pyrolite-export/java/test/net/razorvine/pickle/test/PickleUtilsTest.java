package net.razorvine.pickle.test;

import static org.junit.Assert.*;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.math.BigInteger;

import net.razorvine.pickle.PickleException;
import net.razorvine.pickle.PickleUtils;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

/**
 * Unit tests for the pickler utils.
 *  
 * @author Irmen de Jong (irmen@razorvine.net)
 */
public class PickleUtilsTest {

	private byte[] filedata;

	@Before
	public void setUp() throws Exception {
		filedata="str1\nstr2  \n  str3  \nend".getBytes("UTF-8");
	}

	@After
	public void tearDown() throws Exception {
	}
	
	
	@Test
	public void testReadline() throws IOException {
		InputStream bis=new ByteArrayInputStream(filedata);
		PickleUtils p=new PickleUtils(bis);
		assertEquals("str1", p.readline());
		assertEquals("str2  ", p.readline());
		assertEquals("  str3  ", p.readline());
		assertEquals("end", p.readline());
		try
		{
			p.readline();
			fail("expected IOException");
		}
		catch(IOException x) {}
	}

	@Test
	public void testReadlineWithLF() throws IOException {
		InputStream bis=new ByteArrayInputStream(filedata);
		PickleUtils p=new PickleUtils(bis);
		assertEquals("str1\n", p.readline(true));
		assertEquals("str2  \n", p.readline(true));
		assertEquals("  str3  \n", p.readline(true));
		assertEquals("end", p.readline(true));
		try
		{
			p.readline(true);
			fail("expected IOException");
		}
		catch(IOException x) {}
	}

	@Test
	public void testReadbytes() throws IOException {
		InputStream bis=new ByteArrayInputStream(filedata);
		PickleUtils p=new PickleUtils(bis);
		
		assertEquals(115, p.readbyte());
		assertArrayEquals(new byte[]{}, p.readbytes(0));
		assertArrayEquals(new byte[]{116}, p.readbytes(1));
		assertArrayEquals(new byte[]{114,49,10,115,116}, p.readbytes(5));
		try
		{
			p.readbytes(999);
			fail("expected IOException");
		}
		catch(IOException x) {}
	}

	@Test
	public void testReadbytes_into() throws IOException {
		InputStream bis=new ByteArrayInputStream(filedata);
		PickleUtils p=new PickleUtils(bis);
		byte[] bytes = new byte[] {0,0,0,0,0,0,0,0,0,0};
		p.readbytes_into(bytes, 1, 4);
		assertArrayEquals(new byte[] {0,115,116,114,49,0,0,0,0,0}, bytes);
		p.readbytes_into(bytes, 8, 1);
		assertArrayEquals(new byte[] {0,115,116,114,49,0,0,0,10,0}, bytes);
	}

	@Test
	public void testBytes_to_integer() {
		try {
			PickleUtils.bytes_to_integer(new byte[] {});
			fail("expected PickleException");
		} catch (PickleException x) {}
		try {
			PickleUtils.bytes_to_integer(new byte[] {0});
			fail("expected PickleException");
		} catch (PickleException x) {}
		assertEquals(0x00000000, PickleUtils.bytes_to_integer(new byte[] {0x00, 0x00}));
		assertEquals(0x00003412, PickleUtils.bytes_to_integer(new byte[] {0x12, 0x34}));
		assertEquals(0x0000ffff, PickleUtils.bytes_to_integer(new byte[] {(byte)0xff, (byte)0xff}));
		assertEquals(0x00000000, PickleUtils.bytes_to_integer(new byte[] {0,0,0,0}));
		assertEquals(0x12345678, PickleUtils.bytes_to_integer(new byte[] {0x78, 0x56, 0x34, 0x12}));
		assertEquals(0xff802040, PickleUtils.bytes_to_integer(new byte[] {0x40, 0x20, (byte)0x80, (byte)0xff}));
		assertEquals(0x01cc02ee, PickleUtils.bytes_to_integer(new byte[] {(byte)0xee, 0x02, (byte)0xcc, 0x01}));
		assertEquals(0xcc01ee02, PickleUtils.bytes_to_integer(new byte[] {0x02, (byte)0xee, 0x01, (byte)0xcc}));
		assertEquals(0xeefffffe, PickleUtils.bytes_to_integer(new byte[] {(byte)0xfe, -1,-1,(byte)0xee}));
		try
		{
			PickleUtils.bytes_to_integer(new byte[] {(byte) 200,50,25,100,1,2,3,4});
			fail("expected PickleException");
		} catch (PickleException x) {}
	}

	@Test
	public void testBytes_to_uint() {
		try {
			PickleUtils.bytes_to_uint(new byte[] {},0);
			fail("expected PickleException");
		} catch (PickleException x) {}
		try {
			PickleUtils.bytes_to_uint(new byte[] {0},0);
			fail("expected PickleException");
		} catch (PickleException x) {}
		assertEquals(0x000000000L, PickleUtils.bytes_to_uint(new byte[] {0,0,0,0} ,0));
		assertEquals(0x012345678L, PickleUtils.bytes_to_uint(new byte[] {0x78, 0x56, 0x34, 0x12} ,0));
		assertEquals(0x0ff802040L, PickleUtils.bytes_to_uint(new byte[] {0x40, 0x20, (byte)0x80, (byte)0xff} ,0));
		assertEquals(0x0eefffffeL, PickleUtils.bytes_to_uint(new byte[] {(byte)0xfe, -1,-1,(byte)0xee} ,0));
	}

	@Test
	public void testBytes_to_long() {
		try {
			PickleUtils.bytes_to_long(new byte[] {}, 0);
			fail("expected PickleException");
		} catch (PickleException x) {}
		try {
			PickleUtils.bytes_to_long(new byte[] {0}, 0);
			fail("expected PickleException");
		} catch (PickleException x) {}
		assertEquals(0x00000000L, PickleUtils.bytes_to_long(new byte[] {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} ,0));
		assertEquals(0x00003412L, PickleUtils.bytes_to_long(new byte[] {0x12, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} ,0));
		assertEquals(0xff000000000000ffL, PickleUtils.bytes_to_long(new byte[] {(byte)0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, (byte)0xff} ,0));
		assertEquals(0L, PickleUtils.bytes_to_long(new byte[] {0,0,0,0,0,0,0,0} ,0));
		assertEquals(0x8877665544332211L, PickleUtils.bytes_to_long(new byte[] {0x11,0x22,0x33,0x44,0x55,0x66,0x77,(byte)0x88} ,0));
		assertEquals(0x1122334455667788L, PickleUtils.bytes_to_long(new byte[] {(byte)0x88,0x77,0x66,0x55,0x44,0x33,0x22,0x11} ,0));
		assertEquals(-1L, PickleUtils.bytes_to_long(new byte[] {(byte)0xff, (byte)0xff, (byte)0xff, (byte)0xff, (byte)0xff, (byte)0xff, (byte)0xff, (byte)0xff} ,0));
		assertEquals(-2L, PickleUtils.bytes_to_long(new byte[] {(byte)0xfe, (byte)0xff, (byte)0xff, (byte)0xff, (byte)0xff, (byte)0xff, (byte)0xff, (byte)0xff} ,0));
	}
	
	
	@Test
	public void testInteger_to_bytes()
	{
		PickleUtils p=new PickleUtils(null);
		assertArrayEquals(new byte[]{0,0,0,0}, p.integer_to_bytes(0));
		assertArrayEquals(new byte[]{0x78, 0x56, 0x34, 0x12}, p.integer_to_bytes(0x12345678));
		assertArrayEquals(new byte[]{0x40, 0x20, (byte)0x80, (byte)0xff}, p.integer_to_bytes(0xff802040));
		assertArrayEquals(new byte[]{(byte)0xfe, -1,-1,(byte)0xee}, p.integer_to_bytes(0xeefffffe));
		assertArrayEquals(new byte[]{(byte)0xff, (byte)0xff, (byte)0xff, (byte)0xff}, p.integer_to_bytes(-1));
		assertArrayEquals(new byte[]{(byte)0xee, 0x02, (byte)0xcc, 0x01}, p.integer_to_bytes(0x01cc02ee));
		assertArrayEquals(new byte[]{0x02, (byte)0xee, 0x01, (byte)0xcc}, p.integer_to_bytes(0xcc01ee02));
	}
	
	@Test
	public void testBytes_to_double() {
		try {
			PickleUtils.bytes_to_double(new byte[] {}, 0);
			fail("expected PickleException");
		} catch (PickleException x) {}
		try {
			PickleUtils.bytes_to_double(new byte[] {0}, 0);
			fail("expected PickleException");
		} catch (PickleException x) {}
		assertTrue(0.0d == PickleUtils.bytes_to_double(new byte[] {0,0,0,0,0,0,0,0}, 0));
		assertTrue(1.0d == PickleUtils.bytes_to_double(new byte[] {0x3f,(byte)0xf0,0,0,0,0,0,0} ,0));
		assertTrue(1.1d == PickleUtils.bytes_to_double(new byte[] {0x3f,(byte)0xf1,(byte)0x99,(byte)0x99,(byte)0x99,(byte)0x99,(byte)0x99,(byte)0x9a} ,0));
		assertTrue(1234.5678d == PickleUtils.bytes_to_double(new byte[] {0x40,(byte)0x93,0x4a,0x45,0x6d,0x5c,(byte)0xfa,(byte)0xad} ,0));
		assertTrue(2.17e123d == PickleUtils.bytes_to_double(new byte[] {0x59,(byte)0x8a,0x42,(byte)0xd1,(byte)0xce,(byte)0xf5,0x3f,0x46} ,0));
		assertTrue(1.23456789e300d == PickleUtils.bytes_to_double(new byte[] {0x7e,0x3d,0x7e,(byte)0xe8,(byte)0xbc,(byte)0xaf,0x28,0x3a} ,0));
		assertTrue(Double.POSITIVE_INFINITY == PickleUtils.bytes_to_double(new byte[] {0x7f,(byte)0xf0,0,0,0,0,0,0} ,0));
		assertTrue(Double.NEGATIVE_INFINITY == PickleUtils.bytes_to_double(new byte[] {(byte)0xff,(byte)0xf0,0,0,0,0,0,0} ,0));
		try
		{
			PickleUtils.bytes_to_double(new byte[] {(byte) 200,50,25,100}, 0);
			fail("expected PickleException");
		} catch (PickleException x) {}

		// test offset
		assertTrue(1.23456789e300d == PickleUtils.bytes_to_double(new byte[] {0,0,0,0x7e,0x3d,0x7e,(byte)0xe8,(byte)0xbc,(byte)0xaf,0x28,0x3a} ,3));
		assertTrue(1.23456789e300d == PickleUtils.bytes_to_double(new byte[] {0x7e,0x3d,0x7e,(byte)0xe8,(byte)0xbc,(byte)0xaf,0x28,0x3a,0,0,0} ,0));
	}

	@Test
	public void testBytes_to_float() {
		try {
			PickleUtils.bytes_to_float(new byte[] {}, 0);
			fail("expected PickleException");
		} catch (PickleException x) {}
		try {
			PickleUtils.bytes_to_float(new byte[] {0}, 0);
			fail("expected PickleException");
		} catch (PickleException x) {}
		assertTrue(0.0f == PickleUtils.bytes_to_float(new byte[] {0,0,0,0}, 0));
		assertTrue(1.0f == PickleUtils.bytes_to_float(new byte[] {0x3f,(byte)0x80,0,0} ,0));
		assertTrue(1.1f == PickleUtils.bytes_to_float(new byte[] {0x3f,(byte)0x8c,(byte)0xcc,(byte)0xcd} ,0));
		assertTrue(1234.5678f == PickleUtils.bytes_to_float(new byte[] {0x44,(byte)0x9a,0x52,0x2b} ,0));
		assertTrue(Float.POSITIVE_INFINITY == PickleUtils.bytes_to_float(new byte[] {0x7f,(byte)0x80,0,0} ,0));
		assertTrue(Float.NEGATIVE_INFINITY == PickleUtils.bytes_to_float(new byte[] {(byte)0xff,(byte)0x80,0,0} ,0));

		// test offset
		assertTrue(1234.5678f == PickleUtils.bytes_to_float(new byte[] {0,0,0, 0x44,(byte)0x9a,0x52,0x2b} ,3));
		assertTrue(1234.5678f == PickleUtils.bytes_to_float(new byte[] {0x44,(byte)0x9a,0x52,0x2b,0,0,0} ,0));
	}

	@Test
	public void testDouble_to_bytes()
	{
		PickleUtils p=new PickleUtils(null);
		assertArrayEquals(new byte[]{0,0,0,0,0,0,0,0}, p.double_to_bytes(0.0d));
		assertArrayEquals(new byte[]{0x3f,(byte)0xf0,0,0,0,0,0,0}, p.double_to_bytes(1.0d));
		assertArrayEquals(new byte[]{0x3f,(byte)0xf1,(byte)0x99,(byte)0x99,(byte)0x99,(byte)0x99,(byte)0x99,(byte)0x9a}, p.double_to_bytes(1.1d));
		assertArrayEquals(new byte[]{0x40,(byte)0x93,0x4a,0x45,0x6d,0x5c,(byte)0xfa,(byte)0xad}, p.double_to_bytes(1234.5678d));
		assertArrayEquals(new byte[]{0x59,(byte)0x8a,0x42,(byte)0xd1,(byte)0xce,(byte)0xf5,0x3f,0x46}, p.double_to_bytes(2.17e123d));
		assertArrayEquals(new byte[]{0x7e,0x3d,0x7e,(byte)0xe8,(byte)0xbc,(byte)0xaf,0x28,0x3a}, p.double_to_bytes(1.23456789e300d));
		assertArrayEquals(new byte[]{0x7f,(byte)0xf8,0,0,0,0,0,0}, p.double_to_bytes(Double.NaN));
		assertArrayEquals(new byte[]{0x7f,(byte)0xf0,0,0,0,0,0,0}, p.double_to_bytes(Double.POSITIVE_INFINITY));
		assertArrayEquals(new byte[]{(byte)0xff,(byte)0xf0,0,0,0,0,0,0}, p.double_to_bytes(Double.NEGATIVE_INFINITY));
	}
	
	@Test
	public void testDecode_long()
	{
		PickleUtils p=new PickleUtils(null);
		assertEquals(0L, p.decode_long(new byte[0]));
		assertEquals(0L, p.decode_long(new byte[]{0}));
		assertEquals(1L, p.decode_long(new byte[]{1}));
		assertEquals(10L, p.decode_long(new byte[]{10}));
		assertEquals(255L, p.decode_long(new byte[]{(byte)0xff,0x00}));
		assertEquals(32767L, p.decode_long(new byte[]{(byte)0xff,0x7f}));
		assertEquals(-256L, p.decode_long(new byte[]{0x00,(byte)0xff}));
		assertEquals(-32768L, p.decode_long(new byte[]{0x00,(byte)0x80}));
		assertEquals(-128L, p.decode_long(new byte[]{(byte)0x80}));
		assertEquals(127L, p.decode_long(new byte[]{0x7f}));
		assertEquals(128L, p.decode_long(new byte[]{(byte)0x80, 0x00}));

		assertEquals(128L, p.decode_long(new byte[]{(byte)0x80, 0x00}));
		assertEquals(0x78563412L, p.decode_long(new byte[]{0x12,0x34,0x56,0x78}));
		assertEquals(0x785634f2L, p.decode_long(new byte[]{(byte)0xf2,0x34,0x56,0x78}));
		assertEquals(0x12345678L, p.decode_long(new byte[]{0x78,0x56,0x34,0x12}));
		
		assertEquals(-231451016L, p.decode_long(new byte[]{0x78,0x56,0x34,(byte)0xf2}));
		assertEquals(0xf2345678L, p.decode_long(new byte[]{0x78,0x56,0x34,(byte)0xf2,0x00}));
	}
	
	@Test
	public void testEncode_long()
	{
		PickleUtils p=new PickleUtils(null);
		assertArrayEquals(new byte[]{42}, p.encode_long(BigInteger.valueOf(42)));
		assertArrayEquals(new byte[]{-1}, p.encode_long(BigInteger.valueOf(-1)));
		assertArrayEquals(new byte[]{0x00, (byte)0x80}, p.encode_long(BigInteger.valueOf(-32768)));
		assertArrayEquals(new byte[]{0x56,0x34,0x12}, p.encode_long(BigInteger.valueOf(0x123456)));
		assertArrayEquals(new byte[]{(byte)0x99,(byte)0x88,0x77,0x66,0x55,0x44,0x33,0x22,0x11}, p.encode_long(new BigInteger("112233445566778899",16)));
		
		assertArrayEquals(new byte[]{0x78,0x56,0x34,(byte)0xf2}, p.encode_long(BigInteger.valueOf(-231451016L)));	
		assertArrayEquals(new byte[]{0x78,0x56,0x34,(byte)0xf2,0x00}, p.encode_long(BigInteger.valueOf(0xf2345678L)));
	}
}
