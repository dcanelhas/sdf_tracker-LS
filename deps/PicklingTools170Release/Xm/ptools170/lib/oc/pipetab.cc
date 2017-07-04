#include <iostream>
#include <iomanip>
#include <fstream>
#include <pipetab.h>
#include <vector>

PTOOLS_BEGIN_NAMESPACE

// HISTORICAL NOTE: the *T4Tab* routines were written to flow OCVals (usually
// containing OCTabs, thus the names) down a T4000 pipe.  It was hoped that
// the Vrb/T4000 pipe mechanism already had frame sync logic for piping
// across a socket, saving us from having to write our own.  This turned out
// not to be the case, so the *T1Tab* routines were written, complete with
// frame sync logic.

// NOTES: Yes, the name argument should be passed as a string reference
// (string&) instead of by value, but the most common use of the init*
// functions will be something like: initT1TabPipe(phout, m_apick(1), true);
// The m_apick(#) function is a wrapper around a FORTRAN routine which
// doesn't return a true string.  This object can't be converted to a string&
// (no constructor or some similar excuse) but it can be converted into a
// string, hence the pass by value.  This inefficiency should not be too
// concerning because pipes are usually initialized once in the preamble of
// a primitive and never again (not in the primitive's processing loop).

void initT1TabPipe(CPHEADER* o, string name, bool input)
{
	m_init(*o, name, "1000", "SB", 0);
	if (input) {
		m_open(*o, HCBF_INPUT);
		o->cons_len = -2;  // for non-blocking grabs
	}
	else {
		m_open(*o, HCBF_OUTPUT | HCBF_NOABORT);
	}
}

void initT4TabPipe(CPHEADER* o, string name, bool input)
{
	m_init(*o, name, "4000", "", 0);
	if (input) {
		m_open(*o, HCBF_INPUT);
		o->xfer_len = 1;
		o->cons_len = -2;  // for non-blocking grabs
	}
	else {
		m_open(*o, HCBF_OUTPUT | HCBF_NOABORT);
		o->vrecord_length = 0;
	}
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION: sendT4Tab
//
// DECRIPTION:
//   This function sends a tab through an already open T4000 file/pipe.
//   This is done by using a VRBSTRUCT simply as a pointer to an external
//   buffer which is then sent down the pipe.  The receiving end must know
//   that what it receives is not a normal VRBSTRUCT and handle the buffer
//   appropriately.  In this case the buffer contains a serialized Tab
//   object, so the receiver must deserialize it to gain access to the Tab.
//   recvT4Tab was written for this purpose.
//
// NOTES:
//   {send,recv}T4Tab functions read/write OCVals (containing
//   OCTabs, thus the names) to/from X-Midas T4000 pipes.  In order to use
//   these functions properly, initT4TabPipe should be used and when
//   sendT4Tab is used, recvT4Tab must be used.
//   They provide NO frame sync logic for network-spanning pipes.  If
//   T4000 pipes are to be sent across sockets, the mechanism allowing
//   this must provide this logic.  The recvT4Tab function is non-blocking,
//   so the returned Val must be emptiness-tested before use.
//
///////////////////////////////////////////////////////////////////////////////
const int_4 MAX_BUF_SIZE = 327868/8;

void sendT4Tab(CPHEADER* o, const Val& v, string vrb_key)
{
	// a little up front error checking
  if (o->type != 4000) throw logic_error(o->file_name + " is not type 4000");
  
  VRB vrb;
  (void)m_vrbinit(vrb, 0, o->vrecord_length);

  int sz = BytesToSerialize(v);
  char* buf = new char[sz];
  Serialize(v, buf);
	
  //int_4 pval = 
  (void)m_vrbput(vrb, vrb_key, buf, sz, 0, 'B');
  m_filad(*o, &vrb, 1);

	m_vrbfree(vrb, 0);

  delete[] buf;
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: recvT4Tab
//
// DECRIPTION:
//   This function receives a Tab through an already open T4000 file/pipe.
//   This is done by using the T4000 VRBSTRUCT.
//
/// NOTES:
//   {send,recv}T4Tab functions read/write OCVals (containing
//   OCTabs, thus the names) to/from X-Midas T4000 pipes.  In order to use
//   these functions properly, initT4TabPipe should be used and when
//   sendT4Tab is used, recvT4Tab must be used.
//   They provide NO frame sync logic for network-spanning pipes.  If
//   T4000 pipes are to be sent across sockets, the mechanism allowing
//   this must provide this logic.  The recvT4Tab function is non-blocking,
//   so the returned Val must be emptiness-tested before use.
//
///////////////////////////////////////////////////////////////////////////////
bool recvT4Tab(CPHEADER* o, Tab& t, string vrb_key)
{
  // a little up front error checking
  if (o->type != 4000) throw logic_error(o->file_name + " is not type 4000");
  if (o->cons_len > -2) 
    throw logic_error(o->file_name + " uses blocking grabs");

  static int ngot;
  VRB vrb;
	static Val v;
  vector<char> cut_buf;
  cut_buf.reserve(MAX_BUF_SIZE);

  (void)m_vrbinit(vrb, 0, -1);
  m_vrbclr(vrb, 0);
  m_grabx(*o, &vrb, ngot);
  if (ngot < 1) {
		m_vrbfree(vrb, 0);
		return false;
	}
  void* in_buf;
  int_4 mbs = MAX_BUF_SIZE;
  char fmt = 'B';
  int_4 buf_size = m_vrbfind(vrb, vrb_key, in_buf, mbs, 0, fmt);
  if (buf_size != -1) {
    cut_buf.resize(buf_size);
    //int num_bytes = 
    (void)m_vrbget(vrb, vrb_key, &cut_buf[0], 
			     buf_size, 0, fmt);
		try {
			v = None;
    	Deserialize(v, &cut_buf[0]);
			if (v.tag == 't') {
			  //cerr << " received tab " << v << endl;
				t = v;
				m_vrbfree(vrb, 0);
				return true;
			}
		} catch (exception& e) {
			cerr << "[recvT4Tab] " << e.what() << endl;
			m_vrbfree(vrb, 0);
			return false;
		}
  }
	m_vrbfree(vrb, 0);
	return false;
}

//
// NOTES: {send,recv}T1Tab functions read/write OCVals (usually containing
// OCTabs, thus the names) to/from X-Midas T1000 pipes.  These can be network-
// spanning pipes, so there is some frame sync logic for those pipe<->socket
// primitives that allow dropped data (eg, ZPIPE).  Most of these primitives
// use TCP/IP to connect pipes across a socket connection which, being a
// delivery-guaranteed protocol, should mean that we'll never loose sync
// once established.  Therefore checking for lost sync and resyncing should
// be unecessary steps; however, code to do just this has been left in.
// The recvT1Tab function is non-blocking, so the returned Val must be
// emptiness-tested before use.
//

void sendT1Tab(CPHEADER* o, const Val& v, const int_4 xfer)
{
	// a little up front error checking
	if (o->type != 1000) throw logic_error(o->file_name + " is not type 1000");
	if (o->format != "SB") throw logic_error(o->file_name + " is not SB formatted");

	int_u1 sync = SYNC_PATTERN;
	int_4 sz = BytesToSerialize(v);
	//int_4 xfersz = (xfer < 1) ? sz : sz + xfer - (sz % xfer) - SYNC_REPEATS - 4;
	//int_4 xfersz = (xfer > 0) ? sz + xfer - (sz % xfer) - SYNC_REPEATS - 4 : sz;
	int_4 xfersz = sz;
	if (xfer > 0)
		xfersz += xfer - (sz % xfer) - SYNC_REPEATS - 4;
	char* buf = new char[xfersz];
	Serialize(v, buf);
	
	for (int i=0; i<SYNC_REPEATS; ++i)
	  m_filad(*o, &sync, 1);
	m_filad(*o, &sz, 4);
	m_filad(*o, buf, xfersz);

	delete[] buf;
}

// TODO: Remove or fix the resync logic.  The resync isn't optimal as
// currently implemented due to the inability to reread previously read
// bytes in an X-Midas pipe (actually, this can be done but isn't supposed
// to be a pipe feature and therefore doesn't work in all cases).  One way
// to fix resync is to build up a buffer of bytes read which is then re-
// scanned for the sync pattern if lost sync is detected.  Of course, if
// the resync is left in (and/or fixed), the check for lost sync should
// also remain.
//
// One argument against removing resync: if our sync pattern isn't extremely
// unique (and what 3-byte pattern is?), and we ZPIPE our pipe over the
// network, the initial client connect may sync at the wrong place and from
// then on just receive gibberish (or crash).
//
void recvT1Tab(CPHEADER* o, Val& v)
{
	static Tab w;  // file/pipe info
	int_4 ngot;

	// this routine only valid for certain files/pipes
	if (o->type != 1000) throw logic_error(o->file_name + " is not type 1000");
	if (o->format != "SB") throw logic_error(o->file_name + " is not SB formatted");
	if (o->cons_len > -2) throw logic_error(o->file_name + " uses blocking grabs");

	// cannot do anything if file is closed
	if (!o->open) return;

	// handle static vars
	//int_u4 p = int_u4(o);  // pipe id must be Tab-supported type
	AVLP p = AVLP(o);  // pipe id must be Tab-supported type
	if (!w.contains(p)) {
		w[p] = Tab();
		                          // sync          | size          | data
								  // --------------+---------------+---------------
		w[p]["i"] = (double)1.0;  // read offset   | read offset   | read offset
		w[p]["b"] = (int_u4)0;    // not used      | not used      | buffer pointer
		w[p]["z"] = (int_4)0;     // not used      | buffer itself | buffer size
		w[p]["n"] = (int_4)0;     // pattern count | bytes read    | bytes read

		w[p]["step"] = "sync";    // process phase (could infer from other entries
		                          // but clearer this way)
	}

	//cerr << w << endl;

	// just loop
	while (!Mc->break_) {

		// frame sync
		//
		if (w[p]["step"] == "sync") {
			for (int_4 i=w[p]["n"]; i<SYNC_REPEATS; ) {
				int_u1 byt;

				m_grab(*o, &byt, w[p]["i"], 1, ngot);
				if (ngot < 0) throw logic_error("m_grab = " + ngot);
				w[p]["i"] = (double)w[p]["i"] + ngot;

				if (ngot < 1) {
/////////////////////////////////////////////////////////////////////////////////////////
//cerr << ">>>>>>>>>>>  partial sync [0]" << endl;
/////////////////////////////////////////////////////////////////////////////////////////
					w[p]["n"] = i;
					return;  // don't block (continue would block)
				}

				(byt == SYNC_PATTERN) ? ++i : i=0;
			}

			// prepare for next step
			w[p]["n"] = (int_4)0;
			w[p]["z"] = (int_4)0;
			w[p]["step"] = "size";
		}


		// get size of data
		//
		if (w[p]["step"] == "size") {
			union {
				char buf[4];
				int_4 sz;
			} u;
			u.sz = w[p]["z"];

			// offset into buffer to write to
			int_4 offset = w[p]["n"];

			// how many bytes to read
			int_4 nread = 4 - offset;
			
			m_grab(*o, u.buf+offset, w[p]["i"], nread, ngot);
			if (ngot < 0) throw logic_error("m_grab = " + ngot);
			w[p]["i"] = (double)w[p]["i"] + ngot;

			if (ngot < nread) {
/////////////////////////////////////////////////////////////////////////////////////////
if (ngot > 0) {
ostringstream oss;
oss << setfill('0') << setw(8) << hex << u.sz << ends;
cerr << ">>>>>>>>>>>  " << o->file_name << ": partial size [nbytes = " << ngot
<< ", hex(sz) = " << oss.str() << ", dec(sz) = " << u.sz << "]" << endl;
}
/////////////////////////////////////////////////////////////////////////////////////////
				w[p]["z"] = u.sz;
				w[p]["n"] = offset + ngot;
				return;
			}

/////////////////////////////////////////////////////////////////////////////////////////
if (offset > 0) {
ostringstream oss;
oss << setfill('0') << setw(8) << hex << u.sz << ends;
cerr << ">>>>>>>>>>>  " << o->file_name << ": resume size [hex(sz) = "
<< oss.str() << ", dec(sz) = " << u.sz << "]" << endl;
}
/////////////////////////////////////////////////////////////////////////////////////////

			// sanity check size, resync if fail (side effect: prevents huge
			// dynamic memory mallocs later)
			if (u.sz < 0 || u.sz > MAX_MSG_SIZE) {
/////////////////////////////////////////////////////////////////////////////////////////
ostringstream oss;
oss << setfill('0') << setw(8) << hex << u.sz << ends;
//cerr << ">>>>>>>>>>>  resync size [out of bounds: dec(sz) = " << u.sz
//<< ", hex(sz) = " << oss.str() << "]" << endl;
m_emergency(">>>>>>>>>>>  " + o->file_name + ": resync size [out of bounds: dec(sz) = "
+ Stringize(u.sz) + ", hex(sz) = " + oss.str() + "]");
/////////////////////////////////////////////////////////////////////////////////////////

				// rescan for sync pattern
				w[p]["i"] = (double)w[p]["i"] - 4;
				w[p]["n"] = (int_4)(SYNC_REPEATS-1);
				w[p]["step"] = "sync";
				continue;
			}

			// prepare for next step
			w[p]["b"] = (int_u4)0;
			w[p]["n"] = (int_4)0;
			w[p]["z"] = u.sz;
			w[p]["step"] = "data";
		}



		// get data itself
		//
		if (w[p]["step"] == "data") {
			// how big a buffer?
			int_4 sz = w[p]["z"];

			// new buffer or existing one?
			//char* buf = (char*)(int_u4)w[p]["b"];
			char* buf = (char*)(AVLP)w[p]["b"];
			if (buf == NULL)
				buf = new char[sz];

			// offset into buffer to write to
			int_4 offset = w[p]["n"];

			// how many bytes to read
			int_4 nread = sz - offset;
			
			m_grab(*o, buf+offset, w[p]["i"], nread, ngot);
			if (ngot < 0) throw logic_error("m_grab = " + ngot);
			w[p]["i"] = (double)w[p]["i"] + ngot;

			if (ngot < nread) {
/////////////////////////////////////////////////////////////////////////////////////////
if (ngot > 0)
cerr << ">>>>>>>>>>>  " << o->file_name << ": partial data [" << ngot << "]" << endl;
/////////////////////////////////////////////////////////////////////////////////////////
				//w[p]["b"] = int_u4(buf);
				w[p]["b"] = AVLP(buf);
				w[p]["n"] = (int_4)w[p]["n"] + ngot;
				return;
			}

/////////////////////////////////////////////////////////////////////////////////////////
if (offset > 0)
cerr << ">>>>>>>>>>>  " << o->file_name << ": resume data [" << offset << " + "
<< ngot << " = " << sz << "]" << endl;
/////////////////////////////////////////////////////////////////////////////////////////

			// sanity check data, resync if fail (side effect: produces
			// requested Val)
			try { Deserialize(v, buf); }
			catch (const exception& e) {
/////////////////////////////////////////////////////////////////////////////////////////
cerr << ">>>>>>>>>>>  " << o->file_name << ": resync data [" << e.what() << "]" << endl;
/////////////////////////////////////////////////////////////////////////////////////////
				delete[] buf;

				// rescan for sync pattern
				w[p]["i"] = (double)w[p]["i"] - sz - 4;
				w[p]["n"] = (int_4)(SYNC_REPEATS-1);
				w[p]["step"] = "sync";
				continue;
			}
			delete[] buf;

			// prepare for next step
			w[p]["n"] = (int_4)0;
			w[p]["step"] = "sync";

			return;
		}
	}
}

PTOOLS_END_NAMESPACE


