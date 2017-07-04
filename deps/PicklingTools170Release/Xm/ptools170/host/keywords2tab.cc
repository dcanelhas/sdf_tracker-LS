// Classification:  UNCLASSIFIED
//
// keywords2tab.cc
//
// Description:  Converts keywords attached to an X-Midas file
//   to a Python dictionary (Tab) structure that is written
//   to a text file.
//
#include <primitive.h>
#include <headers.h>
#include <vector>
#include <iomanip>
#include <sstream>
#include <stdio.h>
#include "ocval.h"
#include "ocvalreader.h"

#if defined(OC_FORCE_NAMESPACE)
using namespace OC;
#endif

bool keys2Tab (string inFile, const string scope, Tab &outTab);
bool okeys2Tab (const CPHEADER& inFile, const string scope, Tab &outTab);
bool otab2Keys (CPHEADER& inFile, const string scope, Tab &inTab, int_4 depth=1, const bool numbertags=true);
void type2Tab (Tab &t, string key, void* value, string type, int_4 value_len);

template <class T>
int_4 addKeyToFile (CPHEADER& hcb, string key, T value, int_4 mode)
{
  ostringstream oss;

  // Test the input key to determine output format.
  if ((key.size() > 2) && (key[1] == ':')) {
    switch (key[0]) {
      case 'D':
        oss << setprecision(30) << value;
        break;
      default:
        oss << value;
        break;
    }

    if (((mode & MK_Replace) == MK_Replace)) {
      key = key.substr(2, key.size() - 1);
    }
  } else {
    oss << value;
  }

  // Set and put the data.
  string data = oss.str();

  if (data.size() > 0) {
    return m_put_keydata(hcb, key, &data[0], data.size(), mode, 'A');
  }
  return -1;
} // addKeyToFile


bool okeys2Tab (const CPHEADER& inFile, const string scope, Tab& outTab)
{
  Tab  tScoped[1000];
  int tScopeLevel = 0;

  string key = "**INIT**";
  int_4  mode = MK_Typetag + MK_Next;
  void* keyptr = 0;
  char buftype;
  int ls = m_find_keydata(inFile, key, keyptr, ls, mode, buftype);

  while (ls >= 0) {
    int_4 pos = key.find(":");
    string kstr = key.substr(pos + 1);
    string type = key.substr(0, pos);

    // Check to see if found scoping keyword
    // If we find the scoping keywords we wanted to create
    // a nested Tab file
    if (kstr.find(scope, 0) == 0) {
      // Make a single Tab entry/value.
      Tab t;
      type2Tab(t, kstr, keyptr, type, ls);
      //cerr << kstr << ":" << t[kstr] << endl;

      // Check if this tag already present.
      bool noNewScope = false;
      for (int i = 0; i <= tScopeLevel; ++i) {
        if (tScoped[i].contains(kstr)) {
          // If this is the <scopevalue>END key, don't create a new scope
          // level.  Check for *END and that the current scope value matches
          // the end.
          string curScope = tScoped[i][kstr];
          string endScope = t[kstr];
          int pos = endScope.rfind("END");
          if (pos == (endScope.size() - 3) && endScope.find(curScope) == 0) {
            noNewScope = true;
          }
          for (int j = tScopeLevel; j >= i; --j) {
            string tagName = tScoped[j]["TAG_UNIQUE_NAME____"];
            tScoped[j].remove("TAG_UNIQUE_NAME____");
            string scopeName = tScoped[j][tagName];
            tScoped[j].remove(tagName);
            tScoped[j - 1][scopeName] = tScoped[j];
            tScoped[j] = Tab();
          }
          tScopeLevel = i - 1;
          break;
        }
      }

      if (!noNewScope) {
        ++tScopeLevel;
        tScoped[tScopeLevel]["TAG_UNIQUE_NAME____"] = kstr;
        tScoped[tScopeLevel][kstr] = t[kstr];
      }
    }
    else {
      type2Tab(tScoped[tScopeLevel], kstr, keyptr, type, ls);
    }
    ls = m_find_keydata(inFile, key, keyptr, ls, mode, buftype);
  }

  // Write to 1-up tab and clear the rest and start again.
  for (int i = tScopeLevel; i > 0; --i) {
    string tagName = tScoped[i]["TAG_UNIQUE_NAME____"];
    tScoped[i].remove("TAG_UNIQUE_NAME____");
    string scopeName = tScoped[i][tagName];
    tScoped[i].remove(tagName);
    tScoped[i - 1][scopeName] = tScoped[i];
    tScoped[i] = Tab();
  }
  outTab = tScoped[0];

  return true;
} // okeys2Tab


bool keys2Tab (string inFile, const string scope, Tab &outTab)
{
  bool status = false;
  CPHEADER hkey;

  // Check to make sure hkey exists
  hkey.file_name = inFile;
  m_init (hkey, hkey.file_name, " ", " ", 0);
  m_existence (hkey, -1);

  if (hkey.exist) {
    m_inok (hkey);

  }
  else {
    return false;
  }

  status = okeys2Tab(hkey, scope, outTab);
  m_close (hkey);

  return status;
} // keys2Tab


bool otab2Keys (CPHEADER& hin, string scope, Tab& inTab, int_4 depth, const bool numbertag)
{
  int_4 mode = MK_Normal | MK_Typetag;

  string scope_key = scope;
  if ((depth != 1) && (numbertag)) {
    scope_key += Stringize(depth - 1);
  }
  string scope_value = "";

  for (It I(inTab); I(); ) {
    // Grab the next keyword/value pair.
    string k = string(I.key());
    Val v = I.value();

    switch (v.tag) {
      case 's': // L:
      case 'S': // L:
        {
          int_u1 x = v;
          addKeyToFile<int_u1>(hin, "L:" + k, x, mode);
        }
        break;
      case 'i': // L:
      case 'I': // L:
        {
          int_2  x = v;
          addKeyToFile<int_2>(hin, "L:" + k, x, mode);
        }
        break;
      case 'l': // L:
      case 'L': // L:
      case 'x': // L:
      case 'X': // L:
      case 'b': // L:
        {
          int_4  x = v;
          addKeyToFile<int_4>(hin, "L:" + k, x, mode);
        }
        break;
      case 'f': // D:
        {
          real_4 x = v;
          addKeyToFile<real_4>(hin, "D:" + k, x, mode);
        }
        break;
      case 'd': // D:
        {
          real_8 x = v;
          addKeyToFile<real_8>(hin, "D:" + k, x, mode);
        }
        break;
      case 'a': // A:
        {
          string x = v;
          addKeyToFile<string>(hin, "A:" + k, x, mode);
        }
        break;
      case 't': // SCOPE:
        {
          int pos = k.rfind("END");
          if (pos == (k.size() - 3)) {
            break;
          }
          Tab &t = v;
          addKeyToFile<string>(hin, "A:" + scope_key, k, mode);
          scope_value = k;
          otab2Keys(hin, scope, t, depth + 1);
          addKeyToFile<string>(hin, "A:" + scope_key, scope_value + "END", mode);
        }
        break;
      case 'F':
      case 'D':
      case 'n':
      case 'Z':
        break;
    }
  }

  return true;
} // otab2Keys


void type2Tab (Tab &t, string key, void* value, string type, int_4 value_len)
{
  if (type=="L") {
    int_4 lval = *reinterpret_cast<int_4*>(value);
    t[key] = lval;
  }
  else if (type=="D") {
    real_8 dval = *reinterpret_cast<real_8*>(value);
    t[key] = dval;
  }
  else if (type=="A") {
    string keystr(reinterpret_cast<char*>(value), value_len);
    t[key] = keystr;
  }
} //  type2Tab


void mainroutine()
{
  CPHEADER hin;         // X-Midas file input
  Tab out_tab;           // Tab to be written to output file
  string scope_tag;

  // Get command line parameters
  m_init(hin, m_apick(1), " ", " ", 0);
  string tabfile = m_apick(2);

  // SWITCHES
  if (m_get_uswitch("KTAG", scope_tag) <= 0) {
    scope_tag = "TAG";
  }

  const bool dotab2keys = (m_get_switch("FPUT") >= 1);

  if (Mu->verbose) {
    m_info("/KTAG switch set, scope=" + scope_tag);
  }

  m_lowercase(tabfile);

  m_existence(hin, -1);

  if (hin.exist) {
    m_open(hin, HCBF_HCBMOD);
  }

  if (!dotab2keys) {
    if (Mu->verbose) {
      cerr << "[Keywords2Tab]::KEYS->TAB" << endl;
    }
    bool kstat = okeys2Tab(hin, scope_tag, out_tab);
    if (Mu->verbose) {
      cerr << "[Keywords2Tab]::converted: " << kstat << endl;
    }
    if (kstat) {
      try {
        WriteTabToFile(out_tab, tabfile);
      } catch (const exception &e) {
        cerr << "[Keywords2Tab]::Advisory: " << e.what() << endl;
      }
    }
    else {
      m_warning("Unable to convert Keywords to Tab");
    }
  }
  else {
    if (Mu->verbose) {
      cerr << "[Keywords2Tab]::TAB->KEYS" << endl;
    }
    Tab in_tab;
    try {
      ReadTabFromFile(tabfile, in_tab);
      if (!otab2Keys(hin, scope_tag, in_tab)) {
        m_warning("Unable to convert Tab to Keywords");
      }
    } catch (const exception &e) {
      cerr << "[Keywords2Tab]::Advisory: " << e.what() << endl;
    }
  }

  if (Mu->verbose) {
    cerr << "[Keywords2Tab]::complete" << endl;
  }

  m_close(hin);
}

