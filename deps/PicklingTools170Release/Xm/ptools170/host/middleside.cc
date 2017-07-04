
#include "shmboot.h"    // ServerSide, ClientSide and SHMMain classes
#include <primitive.h>

// This is just a primitive that sits in the middle and can "add" to
// the packet as it goes by
void mainroutine()
{
  string shared_mem_name = m_apick(1);
  string input_pipename  = m_apick(2);
  string output_pipename = m_apick(3);
  int    packet_capacity = m_lpick(4);
  bool   debug = true;

  cout << "******** FOR REPRODUCIBLE RESULTS: Start up X-Midas under \n"
       << " a /bin/tcsh (or appropriate shell) where the shell has been \n"
       << " started with setarch:" << endl;
  if (sizeof(void*)==4) {
    cout << "  % setarch i386 -L -R /bin/tcsh " << endl;
  } else if (sizeof(void*)==8) {
    cout << "  % setarch x86_64 -L -R /bin/tcsh";
  } else {
    cout << " ???  Unknown arch ??? use uname -i to find your arch " 
	 << " and use " << endl
	 << "  % setarch xxx -L -R " << endl
	 << "     where xxx is the result of 'uname -i'" << endl;
  }
  cout << "  (new shell created ... now start X-Midas in this shell)\n"
       << "  % setenv XMVER 4_10_2 \n" 
       << "  % source /xmidas/cshrc \n"
       << "  % xmstart \n";
  cout << "***********************************************" << endl;



  // Main memory is already set-up by first serverside

  // client is for input pipe, server is for output pipe
  ClientSide client(shared_mem_name, input_pipename, debug); 
  ServerSide server(shared_mem_name, output_pipename, packet_capacity, debug);
  
  m_sync();

  // Always start after m_sync so we know shared memory segment is there
  client.start();
  server.start();
  CQ& input  = client.pipe();  // pipe to read
  CQ& output = server.pipe();  // pipe to write

  // Main loop
  for (int up=0; ; up++) {

    // Break early if needed
    if (Mc->break_) break;

    // Get packet: if we fail to dequeue a packet in .1 seconds,
    // the dequeue fails, and we try again (but only after checking
    // Mc->break_ so we can get out if needed).
    Val packet;
    bool valid = input.dequeue(.1, packet);
    if (!valid) continue;

    // Packet is valid, do stuff with it
    {
      TransactionLock tl(packet);  // Lock access so only this process access
      Tab& t = packet;
      t["new_seq"] = up;
    }
    packet.prettyPrint(cerr);

    // Put the packet on the pipe: note that all were putting
    // on the pipe is the proxy, so this is a ref-counted thingee
    bool enqueued = false;
    while (!enqueued ) {

      // Try to enqueue packet: if it can't within the given .1 seconds,
      // it will return false to indicate the packet wasn't queued.
      enqueued = output.enqueue(packet, .1);

      // Need to check the break every so often, that's why we used timed enq
      if (Mc->break_) break;
    }

  }


  
}
