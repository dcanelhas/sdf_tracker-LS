#ifndef M2TABLEIZE_H_
#define M2TABLEIZE_H_

#include "m2opaltable.h"
#include "m2opallink.h"
#include "m2opalheader.h"

// At the bottom of the file are a bunch of conversion from
// "non standard" types (TimePacket, OpalLink, EventData) to
// "standard" types (Tables, Strings, Lists).  We "mod" the Tablize
// routines in the m2opaltable.cc file slightly so they are more informative

// NOTE:::: These SHOULD be in a .h instead of labguishing in
// m2opaltable.cc (How did these ever get called from M2k, if they
// ever did???)  JUST IN CASE these are going to conflict with
// m2opaltable.cc, we change the names from Tablize to Tableize, but
// the first part of this file is (sorta) just cut and paste from M2k
// m2opaltable.cc

// ///////////////////////////////////////////// Tableize Methods

inline OpalTable* Tableize (const MultiVector& mv)
{
  PotHolder<OpalTable> tbl(new OpalTable);
  for (Index i = 0; i < mv.length(); i++) {
    tbl->append(Opalize(mv[i]));
  }
  return tbl.abandon();
}					// Tableize (MultiVector)


inline OpalTable* Tableize (const EventData& ed)
{
  PotHolder<OpalTable> tbl(new OpalTable);
  tbl->put("StartTime", Opalize(ed.startTime()));
  if (ed.knownEndTime()) {
    tbl->put("EndTime", Opalize(ed.endTime()));
  }
  OpalTable tracks;
  for (Index i = 0; i < ed.length(); i++) {
    tracks.append(Opalize(ed[i]));
  }
  tbl->put("TRACKS", Opalize(tracks));
  return tbl.abandon();
}					// Tableize (EventData)



inline OpalTable* Tableize (const IndexedTimeStamp& stamp)
{
  PotHolder<OpalTable> tbl(new OpalTable);

  tbl->put("RepresentsPTime", Opalize(stamp.representsPrecisionTime()));
  tbl->put("FSO", Opalize(stamp.fso()));
  tbl->put("VSO", Opalize(stamp.vso()));

  if (stamp.representsPrecisionTime()) {
    // If we are looking at a time stamp that represents precision
    // time, then there are additional fields to store.
    tbl->put("Time", Opalize(stamp.absoluteTime()));
    tbl->put("AccumDecimation", Opalize(stamp.accumDecimation()));
    tbl->put("TrackNumber", Opalize(stamp.track()));
    
    if (stamp.initializedPolynomial()) {
      Vector pi(DOUBLE, 0);
      stamp.getPolynomial(pi);
      tbl->put("pi", Opalize(pi()));
    }       

  } else {
    // Things specific to nominal time stamps
    tbl->put("Time", Opalize(stamp.relativeTime()));
  }

  return tbl.abandon();
}					// Tableize (IndexedTimeStamp)



inline OpalTable* Tableize (const TimePacket& tp)
{
  PotHolder<OpalTable> tbl(new OpalTable);
  tbl->put("Valid", Opalize(tp.valid()));
  
  // "stamps" is an opal table of opal tables, each of which
  // represents a time stamp in this time packet

  OpalTable stamps;
  { // Precision
    OpalTable p_stamps;
    StingyArray<IndexedTimeStamp> s = tp.precisionTimeStamps();
    Index len = s.length();

    for (Index ii = 0 ; ii < len; ii++) {
      p_stamps.put(ii, OpalValue(Tableize(s[ii])));
    }
    stamps.put("Precision", Opalize(p_stamps));
  }

  { // Nominal
    OpalTable n_stamps;
    StingyArray<IndexedTimeStamp> s = tp.nominalTimeStamps();
    Index len = s.length();

    for (Index ii = 0 ; ii < len; ii++) {
      n_stamps.put(ii, OpalValue(Tableize(s[ii])));
    }
    stamps.put("Nominal", Opalize(n_stamps));
  }
  
  // Put OpalTable of IndexedTimeStamps in OpalTable representing the
  // TimePacket
  tbl->put("TimeStamps", Opalize(stamps));
  
  return tbl.abandon();
}					// Tableize (TimePacket)


// Helper routine to convert EventData to a Table.  To be consistent
// with what protocol 0 does, we do it the same way, even though there
// might be a better way ...
inline OpalTable EventDataToTable (const OpalValue& ov)
{
  OpalValue ret;

  EventData ev = UnOpalize(ov, EventData);
  OpalTable ot;
  ot.put("startTime", ev.startTime());
  ot.put("endTime", ev.endTime());
  OpalTable l;
  for (int ii=0; ii<ev.length(); ii++) {
    l.append(OpalValue(ev[ii])); // Put the event data
  }
  ot.put("DATA", l);
  return ot;
  
  // TODO: Would we rather do this?
  //EventData ed = UnOpalize(ov, EventData);
  //OpalTable* otp = Tableize(ed);
  //OpalValue t(otp);
}

// helper function to turn time packets into Tables for
// serialization purposes
inline OpalTable TimePacketToTable (const OpalValue& ov)
{
  TimePacket tp = UnOpalize(ov, TimePacket);
  OpalTable* otp = Tableize(tp);
  OpalValue t(otp);
  return *otp;
}

inline string OpalValueToString (const OpalValue& ov)
{
  string s = ov.stringValue();
  return s;
}

inline string OpalLinkToString (const OpalValue& ov)
{
  // Serialize as string.  TODO: As Proxy?
  OpalLink& ol = dynamic_cast(OpalLink&, *(ov.data()));
  string path = ol.path();
  return path;
}

inline OpalTable OpalHeaderToTable (const OpalValue& ov)
{
  // Serialize as table
  OpalHeader oh = UnOpalize(ov, OpalHeader);
  OpalTable  ot = oh.table();
  return ot;
}

inline OpalTable MultiVectorToList (const OpalValue& ov)
{
  OpalTable l(OpalTableImpl_GRAPHARRAY);
  MultiVector mv = UnOpalize(ov, MultiVector);
  for (int ii=0; ii<mv.length(); ii++) {
    l.append(OpalValue(mv[ii]));
  }
  return l;
}

#endif // M2TABLEIZE_H_

