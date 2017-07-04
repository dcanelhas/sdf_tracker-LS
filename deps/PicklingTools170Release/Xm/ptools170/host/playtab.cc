///////////////////////////////////////////////////////////////////////////////
//
//  CLASSIFICATION: UNCLASSIFIED
//
//  FILENAME: playtab.cc
//
//  DESCRIPTION:
//    Loads a tabfile to a T4000 pipe.
//
//  REVISION HISTORY:
//    NAME    DATE        DESCRIPTION OF CHANGE
//  -------------------------------------------------------------------------
//    jdg     27Feb07     Initial
//
///////////////////////////////////////////////////////////////////////////////
#include <primitive.h>
#include <istream>
#include <ocval.h>
#include <ocvalreader.h>
#include <t4tab.h>

#if defined(OC_FORCE_NAMESPACE)
using namespace OC;
#endif

#define cerr cerr << "playtab.cc> "

string gl_vname;

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: sendFromLogFile
//
// DESCRIPTION:
//   Open the file and searches for contents contained in {} brackets.
//   When a full string starting with '{' and ending with '}' is located,
//   the string is converted to a Tab and sent out the T4000 pipe.
//   Minimal error checking is done.
//
///////////////////////////////////////////////////////////////////////////////
void sendFromLogFile(CPHEADER &hout, const string fname, const real_8 wait)
{
	char c          = ' ';
	char prevc      = ' ';
	string s        = "";
	string brackets = "";
	bool inString   = false;
	bool gotStart   = false;

	ifstream f(fname.c_str());
	if (f.is_open()) {
		if (Mu->verbose) {
			m_info("[sendFromLogFile] sending: " + fname);
		}

		while (!f.eof() && !Mc->break_) {
			c = f.get();

			// Check for in string.
			if ((c == '\'') || (c == '"')) {
				inString = !inString;
			}

			// Check for errors on input.
			if (!inString) {
				if ((prevc == ',') && ((c == '}') || (c == ']'))) {
					cerr << s << endl << " next->" << c << endl;
					m_warning("[sendFromLogFile] bad placing of ,: " + fname);
					return;
				}
				else if (c == '{') {
					gotStart = true;
					brackets += c;
				}
				else if (c == '[') {
					brackets += c;
				}
				else if (c == '}') {
					if (brackets[brackets.length()-1] != '{') {
						cerr << s << endl << " next->" << c << endl;
						m_warning("[sendFromLogFile] non-matching {}: " + fname);
						return;
					}
					else {
						brackets.erase(brackets.length()-1, 1);
					}
				}
				else if (c == ']') {
					if (brackets[brackets.length()-1] != '[') {
						cerr << s << endl << " next->" << c << endl;
						m_warning("[sendFromLogFile] non-matching []: " + fname);
						return;
					}
					else {
						brackets.erase(brackets.length()-1, 1);
					}
				}
			}

			if (gotStart) {
				s += c;
			}

			// Remove whitespace.
			if (!inString &&
				(c != ' ') && (c != '\n') && (c != '\r') && (c != '\t')) {
				prevc = c;
			}

			if (!gotStart) {
				prevc = ' ';
			}

			// Check for Tab.
			if ((brackets.length() == 0) && gotStart) {
				try {
					Tab t = s;
					if (Mu->verbose) {
						t.prettyPrint(cerr);
					}
					sendT4Tab(hout, t, gl_vname);
					s = "";
					prevc = ' ';
					gotStart = false;
					m_pause(wait);
				} catch (const exception &e) {
					cerr << s << endl;
					m_warning("Problem:" + Stringize(e.what()));
				}
			}
		}
		f.close();
	}
	else {
		m_warning("[sendFromLogFile] unable to open file: " + fname);
		return;
	}
}

void mainroutine()
{
	CPHEADER hout;
	string tabfile;
	Tab tout;

	// Input tabfile
	tabfile = m_apick(1);
	m_lowercase(tabfile);

	// Output file/pipe
	initT4Tab(hout, m_apick(2), HCBF_OUTPUT);
	
	// Switches
	bool entries = (m_get_switch("ENTRIES") > 0);
	bool logpp   = (m_get_switch("LOGPP") > 0);
	real_8 wait = m_get_dswitch_def("WAIT", Mc->pause);
	if (m_get_uswitch("VNAME", gl_vname) <= 0) {
		gl_vname = DEFAULT_VRBKEY;
	}
	if (entries && logpp) {
		m_warning("/LOGPP set, ignoring /ENTRIES");
		entries = false;
	}

	if (!logpp) {
		try {
			// Load the tabfile to be outputted.
			ReadTabFromFile(tabfile, tout);
		} catch (const exception &e) {
			m_warning("Problem: " + Stringize(e.what()));
			m_error("Could not load file: " + tabfile);
		}
	}

	m_sync();

	// Normal Mode: send the file as an entire Tab.
	if (!entries && !logpp) {
		if (Mu->verbose) {
			tout.prettyPrint(cerr);
		}
		sendT4Tab(hout, tout, gl_vname);
	}
	// Log Mode: input file was created from the output
	//           of prettyPrint, send each entry one at a
	//           time.
	// CAUTION: This must come before the 'entries' check
	//          since this flag will not open the Tab file.
	else if (logpp) {
		sendFromLogFile(hout, tabfile, wait);
	}
	// Entries Mode: Tries to load the Tab on a per entry
	//               basis.
	else if (entries) {
		for (It I(tout); I(); ) {
			if (Mc->break_ || !Mu->okio) {
				break;
			}
			try {
				Tab &tr = tout[I.key()];
				if (Mu->verbose) {
					tr.prettyPrint(cerr);
				}
				sendT4Tab(hout, tr, gl_vname);
			} catch (const exception &e) {
				m_warning(Stringize(I.key()) + ":" + Stringize(e.what()));
			}
			m_pause(wait);
		}
	}
	else {
		m_warning("Did nothing");
	}

	if (hout.open) m_close(hout);
}

// EOF vim: ts=2 sw=2
