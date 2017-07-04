///////////////////////////////////////////////////////////////////////////////
//
//  CLASSIFICATION: UNCLASSIFIED
//
//  FILENAME: tab2xml.cc
//
//  DESCRIPTION:
//    Takes in a Tab file and converts it to an XML table.
//    Will also take in an XML table and output a Tab file.
//
//  SYNTAX:
//    VAL2XML <input>
//
//  REVISION HISTORY:
//    NAME    DATE        DESCRIPTION OF CHANGE
//    ------------------------------------------------------------------------
//    jdg     25Jan08     Initial
//
///////////////////////////////////////////////////////////////////////////////
#include <primitive.h>
#include <iostream>
#include <sstream>
#include <ocval.h>
#include <ocvalreader.h>
#include <occonvert.h>
#include <xmltools.h>

#if defined(OC_FORCE_NAMESPACE)
using namespace OC;
#endif

void mainroutine()
{
  string file_in = m_apick(1);

  m_lowercase(file_in);
  string filehead;
  m_head(file_in, filehead);
  string fileroot;
  m_root(file_in, fileroot);
  string fileext;
  m_ext(file_in, fileext);

  string file_out = m_apick(2);
  m_lowercase(file_out);
  if (file_out == " ") {
    file_out = "";
  }

  Val v;
  Tab t;
  int_4 options = 0;

  bool print_only = (m_get_switch("PRINT") > 0);
  if (Mu->verbose && print_only) {
    m_info("/PRINT switch detected, output to terminal only");
  }

  // Input/Output Flags
  bool val_in   = (m_get_switch("VALIN") > 0);
  bool val_out  = (m_get_switch("VALOUT") > 0);
  bool xml_in   = (m_get_switch("XMLIN") > 0);
  bool xml_out  = (m_get_switch("XMLOUT") > 0);
  bool tab_out  = (m_get_switch("TABOUT") > 0);

  // Load/Dump Flags
  if (m_get_switch("STRICT") > 0) { options |= XML_STRICT_HDR; }

  // Load Flags
  if (m_get_switch_def("UNFOLD", 1) > 0)   { options |= XML_LOAD_UNFOLD_ATTRS; }
  if (m_get_switch_def("NOPRE", 1) > 0)    { options |= XML_LOAD_NO_PREPEND_CHAR; }
  if (m_get_switch_def("DROPATT", 0) > 0)  { options |= XML_LOAD_DROP_ALL_ATTRS; }
  if (m_get_switch_def("ORDER", 0) > 0)    { options |= XML_LOAD_USE_OTABS; }
  if (m_get_switch_def("TRYATT", 0) > 0)   { options |= XML_LOAD_TRY_TO_KEEP_ATTRIBUTES_WHEN_NOT_TABLES; }
  if (m_get_switch_def("DROPTOP", 0) > 0)  { options |= XML_LOAD_DROP_TOP_LEVEL; }
  if (m_get_switch_def("EVAL", 1) > 0)     { options |= XML_LOAD_EVAL_CONTENT; }

  // Dump Flags
  if (m_get_switch_def("PREKEYS", 0) > 0)  { options |= XML_DUMP_PREPEND_KEYS_AS_TAGS; }
  if (m_get_switch_def("TAGATT", 0) > 0)   { options |= XML_DUMP_SIMPLE_TAGS_AS_ATTRIBUTES; }
  if (m_get_switch_def("STRSTR", 0) > 0)   { options |= XML_DUMP_STRINGS_AS_STRINGS; }
  if (m_get_switch_def("STRGUESS", 1) > 0) { options |= XML_DUMP_STRINGS_BEST_GUESS; }
  if (m_get_switch_def("PRETTY", 1) > 0)   { options |= XML_DUMP_PRETTY; }
  if (m_get_switch_def("PODLIST", 0) > 0)  { options |= XML_DUMP_POD_LIST_AS_XML_LIST; }
  if (m_get_switch_def("STREMPTY", 0) > 0) { options |= XML_DUMP_PREFER_EMPTY_STRINGS; }
  if (m_get_switch_def("SORT", 0) > 0)     { options |= XML_DUMP_UNNATURAL_ORDER; }

  string toplevel = "";
  if (m_get_uswitch("TOP", toplevel) <= 0) {
    toplevel = "top";
  }

  // Defaults
  if (!val_in && !xml_in) {
    val_in = true;
  }
  if (!val_out && !xml_out) {
    xml_out = true;
  }

  // Error checking
  if (val_in && xml_in) {
    m_error("/VALIN and /XMLIN switches detected, only one is allowed");
  }
  if (val_out && xml_out) {
    m_error("/VALOUT and /XMLOUT switches detected, only one is allowed");
  }

  // Info stuff
  if (Mu->verbose && val_in && xml_out) {
    m_info("VAL->XML");
  }
  if (Mu->verbose && val_in && val_out) {
    m_info("VAL->VAL");
  }
  if (Mu->verbose && xml_in && xml_out) {
    m_info("XML->XML");
  }
  if (Mu->verbose && xml_in && val_out) {
    m_info("XML->VAL");
  }

  // Read in the Tab or XML Table
  if (val_in) {
    try {
      ReadValFromFile(file_in, v);
    } catch (const exception& e) {
      if (Mu->verbose) cerr << e.what() << endl;
      try {
        ReadValFromFile(m_apick(1), v);
      } catch (const exception& e) {
        if (Mu->verbose) cerr << e.what() << endl;
        m_error("Could not read file as Val: " + file_in);
      }
    }
  }
  else if (xml_in) {
    try {
      ReadValFromXMLFile(file_in, v, options);
      if (tab_out && (v.tag == 'a')) {
        ConvertArrToTab(v, false, false);
      }
    } catch (const exception& e) {
      if (Mu->verbose) cerr << e.what() << endl;
      try {
        ReadValFromXMLFile(m_apick(1), v, options);
        if (tab_out && (v.tag == 'a')) {
          ConvertArrToTab(v, false, false);
        }
        t=v;
      } catch (const exception& e) {
      if (Mu->verbose) cerr << e.what() << endl;
        m_error("Could not read file as XML: " + file_in);
      }
    }
  }
  
  if (Mu->verbose) {
    m_info("Input: " + file_in);
  }

  // Write out the Tab or XML Table
  if (val_out) {
    if (print_only) {
      v.prettyPrint(cerr);
    }
    else {
      if (file_out == "") {
        file_out = filehead + fileroot + "_val.val";
      }
      try {
        WriteValToFile(v, file_out);
      } catch (const exception& e) {
        if (Mu->verbose) cerr << e.what() << endl;
        m_error("Could not write file as Val: " + file_out);
      }
    }
  }
  else if (xml_out) {
    if (print_only) {
      stringstream s;
      WriteValToXMLStream(v, s, toplevel, options);
      cerr << s.str() << endl;
      //prettyPrintXML(t, cerr);
    }
    else {
      if (file_out == "") {
        file_out = filehead + fileroot + "_xml.tbl";
      }
      m_info("Output: " + file_out);
      try {
        WriteValToXMLFile(v, file_out, toplevel, options);
      } catch (const exception& e) {
        if (Mu->verbose) cerr << e.what() << endl;
        m_error("Could not write file as XML: " + file_out);
      }
    }
  }

  if (Mu->verbose && !print_only) {
    m_info("Output: " + file_out);
  }

  m_sync();
}

