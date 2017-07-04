//----------------------------------------------------------------------------
// Sample X-Midas C++ primitive showing how to write an HTTPServer
// You need at least OpenContainers 1.5.1 and set some flags
// correctly (see the cfg directory)
//----------------------------------------------------------------------------
#include <primitive.h>
#include <simplehttpserver.h>

#if defined(OC_FORCE_NAMESPACE)
using namespace OC;
#endif


// To write your own HTTP socket server, you have to write two
// classes:  a "SpecialHTTPWorker" (which inherits from ThreadedHTTPWorker)
// a main server which inherits from SimpleHTTPServer.
// The basic idea is that the SimpleHTTPServer does most of the work:
// for every connection that comes across, it passes the data to
// a "SpecialHTTPWorker" (a thread) which handles all the processing
// and I/O for that particular connection.

// The purpose of the server below is to echo back the request that got 
// sent to it.

// The actual server code is pretty boilerplate (see after the worker
// code).  Most of the work happens in the Worker.  This sample
// shows some HTTP 1.1 chunked response as well as more natural
// 1.1 GET/POSTs.  The client basically inherits from ThreadedHTTPWorker
// and overrides:
//   handleGET_
//   handlePOST_
// There are a number of routines for sending responses back
// to the server, all from the httplib_: HTTPValidResponse
// is the most common, but for more complex
// responses. there are several other socket interactions.


// Each client has its own thread to handle the file descriptor
class EchoHTTPWorker : public ThreadedHTTPWorker {
  
  public:
  
  EchoHTTPWorker (const string& name, ThreadedServer& server) :
    ThreadedHTTPWorker(name, server)
  { }


  // User-hook for dealing with a GET: All initial-line, headers data
  // has been processed, but the rest of the message needs to be processed
  // (i.e,) and the response generated.
  virtual void handleGET_ (const string& url, const string& http_version, 
			   const OTab& in_header)
  {
    cerr << "GET " << "url:" << url << " http_version:" << http_version << endl;
    cerr << "headers:" << in_header << endl;

    // Echo the data back TWICE

    OTab out_header;
    string type = in_header.get("type", "HTTP/1.0");

    if (http_version=="HTTP/1.1") { // Then we support chunked encoding

      // Send out just out header, with no content.  Leave that to chunked
      out_header["Transfer-Encoding"] = "chunked";
      httptools_.HTTPValidResponse("", type, out_header);  // send header

      // now, send data, twice
      OTab footers;
      httptools_.sendSingleChunk(url);
      httptools_.sendSingleChunk(url);

      // Need to do this to finish request
      httptools_.sendChunkedFooter(footers);

    } else { // unknown, go back to 1.0, just make data twice as big
      string data = url + url;
      httptools_.HTTPValidResponse(data, type, out_header); // Just send back url as echo
    }

  }
 // User-hook for dealing with a POST: All initial-line, headers data
  // has been processed, but the rest of the message needs to be processed
  // (i.e,) and the response generated.
  virtual void handlePOST_ (const string& url, const string& http_version, 
			    const OTab& headers) 
  {
    cerr << "POST " << "url:" << url << " http_version:" << http_version<< endl;
    cerr << "headers:" << headers << endl;

    Array<char> result;

    // Chunked?
    if (headers.contains("transfer-encoding") && 
	headers("transfer-encoding")=="chunked") {
      if (http_version=="HTTP/1.0") {
	httptools_.HTTPBadRequest();
      }
      OTab footers;
      while (httptools_.getSingleChunkedResponse(result, footers)) {
	// Keep appending data into result
	cerr << "chunked data ... length of data in buffer" << result.length();
      }
      cerr << "FINAL chunked data ... length of data in buffer" << 
	result.length();
      cerr << "footers: " << footers << endl;
    } 
    
    // Not chunked
    else {
	int len = -1;    
	if (headers.contains("content-length")) {
	  // Just get in one chunk
	  len = headers("content-length");
	}
	httptools_.readUntilFull(result, len);
      }
    
    // Made it here, got result: 
    httptools_.HTTPValidResponse(string(result.data(), result.length()));
  } 
  

}; // EchoHTTPWorker 


// Simple Echo Server

class MyEchoServer : public SimpleHTTPServer {
  public:

    // Provide port and host.  Your host is almost always the same
    // as the localhost, unless you wish to use a different interface.
    MyEchoServer (string host="localhost", int port=8888) :
      SimpleHTTPServer(host, port)
    { }


  protected:
  
  // When a new connection comes in, we have to make sure
  // we create the proper type of client for this server.
  virtual ThreadedServerWorker* createThreadedServerWorker_ ()
  { return new EchoHTTPWorker("EchoHTTPServer", *this); }
  
}; // MyServer


void mainroutine()
{
  // Create the http server
  string host = m_apick(1);
  int port    = m_lpick(2);

  MyEchoServer mes(host, port);
  cout << "Opening MyEchoServer on host " << host << " port " << port << endl;
  mes.open();

  // We know the server has been created
  m_sync();

  // Wake up every so often to see if we are quitting
  while (!Mc->break_) {
    m_pause(1);    
  }
  mes.shutdown();
  mes.waitForMainLoopToFinish();
}

