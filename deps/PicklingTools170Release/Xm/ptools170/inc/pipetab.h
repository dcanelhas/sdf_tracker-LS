#ifndef OCPIPETAB_H_

#include <primitive.h>
#include <ocval.h>
#include <ocserialize.h>  // needed here?

PTOOLS_BEGIN_NAMESPACE

void initT4TabPipe(CPHEADER* o, string name, bool input);
void initT1TabPipe(CPHEADER* o, string name, bool input);
void sendT4Tab(CPHEADER* o, const Val& v, string vrb_key = string("TAB"));
void sendT1Tab(CPHEADER* o, const Val& v, const int_4 xfer = 0);
bool recvT4Tab(CPHEADER* o, Tab& t, string vrb_key = string("TAB"));
void recvT1Tab(CPHEADER* o, Val& v);

PTOOLS_END_NAMESPACE

#define SYNC_PATTERN 0x11  // 17 in decimal
#define SYNC_REPEATS 3
#define MAX_MSG_SIZE 10240

#define OCPIPETAB_H_
#endif // OCPIPETAB_H_

