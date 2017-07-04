///////////////////////////////////////////////////////////////////////////////
//
// CLASSIFICATION: UNCLASSIFIED
//
// FILE: pipe2tab
//
// PROGRAMMER: mlb
//
// DESCRIPTION: This primitive takes in a type 4000 file and outputs a 
//              tab file. There are three inputs: the type 4000 file (or
//              pipe), the name of the tab file, and the data tag that 
//              will be used to extract the tab from the type 4000 file.
//              
//
///////////////////////////////////////////////////////////////////////////////
#include <primitive.h>
#include <iostream>
#include <sstream>

#include <ocserialize.h>
#include <ocvalreader.h>

#include <t4tab.h>

#if defined(OC_FORCE_NAMESPACE)
using namespace OC;
#endif


#define MODE_ON  1

void mainroutine ()
{
  string filehead;
  string fileroot;
  string fileext;
  string fid;
  string prevfid = "INIT";
  string fname;
  int numtabs = 0;
  int indxtab = 0;
  real_8 it, ft;
  Tab in_tab;

  string filename = m_apick(2);
  m_head(filename, filehead);
  m_root(filename, fileroot);
  m_ext(filename, fileext);
  if (fileext.size() <= 0) {
    fileext = "tab";
  }

  filehead = filehead + fileroot;

  m_lowercase(filehead);
  m_lowercase(fileext);

  // Set up the input pipe/file
  CPHEADER hin;
  initT4Tab(hin, m_apick(1), HCBF_INPUT);

  // Get the data tag
  string data_tag = m_apick(3);
  if (data_tag.size() <= 0) {
    data_tag = DEFAULT_VRBKEY;
  }

  // SWITCHES
  bool doecho = (m_get_switch("ECHO") > 0);

  // CONTROL WIDGETS
  int_4 m_mode, w_mode;
  m_mode  = m_get_switch_def("MODE", MODE_ON);
  w_mode  = m_lwinit("MODE", m_mode, 0, 1, 1); // 1:
  if (m_mode == MODE_ON) {
    m_info("Mode set to ON");
  }
  else {
    m_info("Mode set to OFF");
  }

  m_sync();

  m_now(it, ft);
  m_times2str(it, 0, ' ', fid);
  m_info("Starttime: " + fid);

  while (!Mc->break_ && Mu->okio) {
    m_pause(Mc->pause);

    // Check for changes in the command widgets.
    if (m_lwget(w_mode, m_mode) > 0) {
      if (m_mode == MODE_ON) {
        m_info("Mode set to ON");
      }
      else {
        m_info("Mode set to OFF");
      }
    }

    if (recvT4Tab(hin, in_tab, data_tag)) {
      if (doecho) {
        in_tab.prettyPrint(cerr);
      }

      // Don't write the tabfile.
      if (m_mode != MODE_ON) {
        continue;
      }

      // Write the tabfile.
      // What was requested was <path><filename>.<ext>
      // The output will be <path><filename>_YYMMDD_hhmmss_x.<ext>
      // Where x is a 1 up number per YYMMDD_hhmmss string.
      m_now(it, ft);
      if ((m_times2str(it, 0, ' ', fid)) >= 20) {
        // Format the string to YYMMDD_hhmmss.
        fid =
        fid.substr(2, 2) +
        fid.substr(5, 2) +
        fid.substr(8, 2) + "_" +
        fid.substr(12, 2) +
        fid.substr(15, 2) +
        fid.substr(18, 2);
      }

      if (prevfid != fid) {
        indxtab = 0;
        prevfid = fid;
      }

      ++indxtab;
      ostringstream out;
      out << indxtab;
      Str fext =  out.str();
      fname = filehead + "_" + fid + "_" + fext + "." + fileext;

      try {
        WriteTabToFile(in_tab, fname);
        if (Mu->verbose) {
          m_info("created tab file: " + fname);
        }
        numtabs++;
      } catch (...) {
        m_warning("cound not write tab file: " + fname);
      }
    }
  }

  m_close(hin);
  m_times2str(it, 0, ' ', fid);
  m_info("Stoptime: " + fid);
  m_info("Tab files written: " + Stringize(numtabs));
}

