
#include "shmboot.h"    // ServerSide, ClientSide and SHMMain classes
#include <primitive.h>

void mainroutine()
{
  string shared_mem_name = m_apick(1);
  int    bytes_in_mem    = m_lpick(2);
  string output_pipename = m_apick(3);
  int    packet_capacity = m_lpick(4);
  bool   debug = true;

  // Probe how memory is used
  void* mem1;
  cerr << "32-bit or 64-bit? " << (sizeof(void*)*8) << endl; 
  cerr << "stack area: " << & mem1 << endl;
  cerr << "heap area : " << ((void*)new char[1]) << endl;
  cerr << "text area : " << (void*)&mainroutine << endl;
  FILE* (*fptr)(const char*, const char*) = fopen;
  cerr << "mmap area : " << (void*)fptr << endl;
  cerr << "big heap  : " << ((void*)new char[1000000]) << endl;
  int m;
  cerr << "stack grows " <<( ((((char*)&m) - ((char*)mem1)) < 0) ? "high to low"
 : "low to high") << endl; 


 // On Linux machines, stack typically grows from high to low (i.e., down),
  // and usually right next to kernel barrier (0x800000000000 on 64-bit
  // machine, and 0xC0000000 on 32-bit machines).  Everything else
  // (text, BSS, heap, mmap) tends to be at lower addresses.
  // Yes, this is non-portable, but placing shared memory 
  // in a location where no-one else uses it and that is 
  // the same across non-inheritance related process is difficult!
  if (sizeof(void*)==4) { // 32-bit
    // Kernel starts at 0xC0000000, grows up from there: stack
    // Stack starts at 0xBfffxxxx (maybe guard page between kernel and stack)
    // Gives 268 Meg for shared memory and stack to share ...
    mem1 = (void*)0xB0000000; 
  } else if (sizeof(void*)==8) {// 64-bit 
    // ... (actually only 48-bits of address space in current gen of Intel)
    // Kernel starts at 0x800000000000, grows low to high 
    // Stack starts at  0x7fffffffxxxx, grows high to low
    // Gives 104Tbytes for shared memory and stack to share...
    mem1 = (void*)0x700000000000ULL;  
  } else { // 
    cerr << "Unknown model" << endl;
    exit(1);
  }

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


  // Get the main memory section up before everyone else looks for it:
  // Do this before m_sync. You only have to this ONCE for the ENTIRE
  // application, usually in the very head primitive.
  SHMMain main(shared_mem_name, bytes_in_mem, debug, mem1);
  main.start();  // Must start to create
    
  // Once the shared memory region is started, then any number
  // of servers and clients can be created inside that shared memory region.
  ServerSide server(shared_mem_name, output_pipename, packet_capacity, debug);
  
  m_sync();

  // Once the server is created with appropriate parameters, start it.
  // This connects to the shared memory and creates a pipe with
  // the given name and capacity.
  server.start();
  CQ& q = server.pipe();  // pipe to enqueue into

  // Main loop
  for (int up=0; ; up++) {

    // Break early if needed
    if (Mc->break_) break;

    // Create a table in shared memory. Note the use of the Shared 
    // function using the shared memory of the server's pool.
    Val packet = Shared(server.pool(), Tab("{'data':array([],'b'),'hdr':{}}"));
    Array<int_u1>& a = packet["data"];
    for (int ii=0; ii<10; ii++) {
      a.append(ii);
    }
    // ... once the table is created, any inserts into that table
    // will put the entries into shared memory so you only have to create
    // the top-level table using the Shared mechanism.
    for (int ii=0; ii<100; ii++) {
      packet[Stringize(ii)+"abc"] = ii;
    }
    packet["sequence_number"] = up;
   
    // Could also just read a table and immediately copy it into 
    // Shared memory like so:
    //   Tab in;
    //   ReadTabFromFile("/tmp/yyy", in);
    //   Val packet=Shared(server.pool(), in); // Copy table into shared memory

    // Put the packet on the pipe: note that all were putting
    // on the pipe is the proxy, so this is a ref-counted thingee
    bool enqueued = false;
    while (!enqueued ) {

      // Try to enqueue packet: if it can't within the given .1 seconds,
      // it will return false to indicate the packet wasn't queued.
      enqueued = q.enqueue(packet, .1);

      // Need to check the break every so often, that's why we used timed enq
      if (Mc->break_) break;
    }

  }


  
}
