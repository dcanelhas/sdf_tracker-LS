///////////////////////////////////////////////////////////////////////////////
//
//  CLASSIFICATION: UNCLASSIFIED
//
//  FILENAME: fanintab.cc
//
//  DESCRIPTION:
//    Takes in many input Tab pipes and outputs a single Tab pipe.
//
//  SYNTAX:
//    FANINTAB <in1|in2|...|inN> <out>
//
//  REVISION HISTORY:
//    NAME    DATE        DESCRIPTION OF CHANGE
//    ------------------------------------------------------------------------
//    jdg     14Sep07     Initial
//
///////////////////////////////////////////////////////////////////////////////
#include <primitive.h>
#include <ocval.h>
#include <t4tab.h>
#include <vector>

#if defined(OC_FORCE_NAMESPACE)
using namespace OC;
#endif

// Stucture used to store CPHEADERs in a vector.
typedef struct {
  CPHEADER *hcb;
} HCB_STRUCT;

//////////////////////////////////////////////////////////////////////////////
// FUNCTION: barStripper
//
// DESCRIPTION:
//   This function attempts to convert a '|' delimited string into a simple
//   Tab containing filenames.
//
//////////////////////////////////////////////////////////////////////////////
void barStripper(Tab& t, string s)
{
  // Start of string for Tab.
  string filename = "";

  // Extract all filenames from the command line.
  for (int i = 0; i < int(s.size()); ++i) {
    if (s[i] == '|') {
      if (filename.size() > 0) {
        t[t.entries()] = filename;
      }
      filename = "";
    }
    else if (s[i] != ' ') {
      filename += s[i];
    }
  }

  // Grab the last filename.
  if (filename.size() > 0) {
    t[t.entries()] = filename;
  }
}

void mainroutine()
{
  vector<HCB_STRUCT> vhin;
  CPHEADER hout;
  Tab tfiles;
  string vrbkey;

  // Extract all filenames from command line.
  barStripper(tfiles, m_upick(1));

  // Need at least one file to fanin.
  if (tfiles.entries() <= 0) {
    m_error("At least one file is needed to fanin.");
  }

  // Open the output file.
  initT4Tab(hout, m_apick(2), HCBF_OUTPUT);

  // Open all the input files.
  for (int i = 0; i < int(tfiles.entries()); ++i) {
    HCB_STRUCT hin;
    hin.hcb = new CPHEADER();
    string fname = string(tfiles[i]);
    initT4Tab(*(hin.hcb), fname, HCBF_INPUT);
    vhin.push_back(hin);
  }

  // The key for the VRB structure passed by t4tab.
  if (m_get_uswitch("VRBKEY", vrbkey) <= 0) {
    vrbkey = DEFAULT_VRBKEY;
  }

  m_sync();

   while (!Mc->break_) {

     for (int i = 0; i < int(vhin.size()); ++i) {
      Tab tin;
      if (recvT4Tab(*(vhin[i].hcb), tin, vrbkey)) {
        sendT4Tab(hout, tin, vrbkey);
      }
    }

    // Don't hog the CPU.
    m_pause(Mc->pause);
  }

  // Close the output file.
  m_close(hout);

  // Close the input files.
  for (int i = 0; i < int(vhin.size()); ++i) {
    m_close(*(vhin[i].hcb));
    delete vhin[i].hcb;
  }

  if (Mu->verbose) {
    m_info("Terminated");
  }
}

