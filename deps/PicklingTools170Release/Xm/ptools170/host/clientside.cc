
#include "shmboot.h"    // ServerSide, ClientSide and SHMMain classes
#include <primitive.h>

void mainroutine()
{
  string shared_mem_name = m_apick(1);
  string input_pipename  = m_apick(2);
  bool debug = true;


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



  // Set-up a client.  Note we have to specify both a shared memory
  // region name AND a pipe name.  The shared memory region is where
  // to look for the pipe, and the pipename is how we figure out
  // which CQ to use.
  ClientSide client(shared_mem_name, input_pipename, debug);
 

  m_sync();


  // Client must be started (which attaches to shared memory), then
  // we can get the pipe we will be dequeueing from
  client.start();
  CQ& q = client.pipe();

  for (int up=0; ;up++) {

    // Break early if needed
    if (Mc->break_) break;

    // Get packet: if we fail to dequeue a packet in .1 seconds,
    // the dequeue fails, and we try again (but only after checking
    // Mc->break_ so we can get out if needed).
    Val packet;
    bool valid = q.dequeue(.1, packet);
    if (!valid) continue;

    // Packet is valid, do stuff with it
    Tab& t = packet;
    t.prettyPrint(cout);
    
  }
  
}
