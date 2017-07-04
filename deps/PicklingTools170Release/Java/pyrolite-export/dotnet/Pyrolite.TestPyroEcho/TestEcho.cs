/* part of Pyrolite, by Irmen de Jong (irmen@razorvine.net) */

using System;
using System.Text;
using Razorvine.Pickle;
using Razorvine.Pyro;

namespace Pyrolite.TestPyroEcho
{
	
/// <summary>
/// Test Pyro with the Pyro echo server. 
/// </summary>
public class TestEcho {

	public static void Main(String[] args) {
		try {
			Test();
		} catch (Exception x) {
			Console.WriteLine("unhandled exception: {0}",x);
		}
	}
	
	public static void Test() {

		Console.WriteLine("Testing Pyro echo server (make sure it's running on localhost 9999)...");
		Console.WriteLine("Pyrolite version: "+Config.PYROLITE_VERSION);

		setConfig();
		
		PyroProxy p=new PyroProxy("localhost",9999,"test.echoserver");

		Object x=42;
		Console.WriteLine("echo param:");
		PrettyPrint.print(x);
		Object result=p.call("echo", x);
		Console.WriteLine("return value:");
		PrettyPrint.print(result);
		
		String s="This string is way too long. This string is way too long. This string is way too long. This string is way too long. ";
		s=s+s+s+s+s;
		Console.WriteLine("echo param:");
		PrettyPrint.print(s);
		result=p.call("echo", s);
		Console.WriteLine("return value:");
		PrettyPrint.print(result);

		Console.WriteLine("error test.");
		try {
			result=p.call("error");
		} catch (PyroException e) {
			Console.WriteLine("Pyro Exception (expected)! {0}",e.Message);
			Console.WriteLine("Pyro Exception cause: {0}",e.InnerException);
			Console.WriteLine("Pyro Exception remote traceback:\n>>>\n{0}<<<",e._pyroTraceback);
		}

		Console.WriteLine("shutting down the test echo server.");
		p.call("shutdown");
	}

	static void setConfig()
	{
		string hmackey=Environment.GetEnvironmentVariable("PYRO_HMAC_KEY");
		if(hmackey!=null) {
			Config.HMAC_KEY=Encoding.UTF8.GetBytes(hmackey);
		}
		string tracedir=Environment.GetEnvironmentVariable("PYRO_TRACE_DIR");
		if(tracedir!=null) {
			Config.MSG_TRACE_DIR=tracedir;
		}
	}
}

}