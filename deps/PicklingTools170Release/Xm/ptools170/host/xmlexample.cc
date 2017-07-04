//////////////////////////////////////////////////////////////////////////////
//  CLASSIFICATION: UNCLASSIFIED
//
//  FILENAME: xmlexample.cc
//
//  DESCRIPTION:
//     Show how to convert between XML and dicts (Python Dictionaries,
//     represented by Vals in C++)
//
//  SWITCHES:
//    See explain page for switch listing.
//
//
//////////////////////////////////////////////////////////////////////////////
#include <primitive.h>
#include <ocval.h>
#include <ocvalreader.h>
#include <xmltools.h>

#if defined(OC_FORCE_NAMESPACE)
using namespace OC;
#endif

inline bool ends_with (const string& main, const string& pattern)
{
  const size_t plen = pattern.length();
  const size_t mlen = main.length();
  if (plen>mlen) {
    return false;
  }
  // Assertion, at least as big as the main string
  const size_t offset = mlen - plen;
  const char* mdata = main.data();
  const char* pdata = pattern.data();
  for (size_t ii=0; ii<plen; ii++) {
    if (mdata[ii+offset]!=pdata[ii]) return false;
  }
  return true;
}

#define DEF_ARRDISP AS_LIST

void mainroutine()
{
  // Parameters
  string filein = m_apick(1);
  bool xml_in = ends_with(filein, ".XML") || ends_with(filein, ".xml");
  string fileout = m_apick(2);
  bool xml_out = ends_with(fileout, ".XML") || ends_with(fileout, ".xml");

  // Switches:
  if (m_get_switch("XML_IN") > 0) xml_in = true;
  if (m_get_switch("XML_OUT") > 0) xml_out = true;
  ArrayDisposition_e array_disposition = 
    ArrayDisposition_e(m_get_switch_def("ARRDISP", DEF_ARRDISP));

  cerr << "XML_IN:" << xml_in << " XML_OUT:" << xml_out 
       << "ARRDISP:" << int(array_disposition) << endl;

  // Convert from XML to dicts or other way!
  Val v;

  // Input, either a dict or XML
  if (xml_in) {
    try {
      ReadValFromXMLFile(filein, v,
			 XML_STRICT_HDR |          // must have hdr
			 XML_LOAD_DROP_TOP_LEVEL | // top-level usually unneeded
			 XML_LOAD_EVAL_CONTENT,    // convert strings to data
			 array_disposition  // How to handle POD data
			 );

    } catch (const exception& e) {
      m_error("Problems reading XML:"+string(e.what())+" from file"+filein);
    }
  } 
  // Must be a dict
  else {
    try {
      ReadValFromFile(filein, v);
    } catch (const exception& e) {
      m_error("Problems reading dict:"+string(e.what())+" from file"+filein);
    }
  } 

  m_sync();

  // Convert dict to XML 
  if (xml_out) {
    try {
      WriteValToXMLFile(v, fileout, "root",
			XML_STRICT_HDR |               // MUST have XML header
			XML_DUMP_STRINGS_BEST_GUESS |  // careful with quotes
			XML_DUMP_PRETTY,               // show nesting
			array_disposition // How to handle POD data
			);
    } catch (const exception& e) {
      m_error("Problems writing XML:"+string(e.what())+" to file"+fileout);
    }
  } 
  // Write as dict
  else {
    try {
      WriteValToFile(v, fileout);
    } catch (const exception& e) {
      m_error("Problems writing dict:"+string(e.what())+" to file"+fileout);
    }
  }
}
