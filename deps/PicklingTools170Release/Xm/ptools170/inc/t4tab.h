#ifndef T4TAB_H_

#include <primitive.h>
#include <ocval.h>
#include <t4val.h>

PTOOLS_BEGIN_NAMESPACE

void initT4Tab(CPHEADER& hcb, string name, int_4 hcbfmode);
void sendT4Tab(CPHEADER& hcb, const Tab& v, 
	       string vrb_key = string(DEFAULT_VRBKEY), 
	       Serialization_e ser=SERIALIZE_OC);
bool recvT4Tab(CPHEADER& hcb, Tab& v, 
	       string vrb_key = string(DEFAULT_VRBKEY), 
	       Serialization_e ser=SERIALIZE_OC);

PTOOLS_END_NAMESPACE

#define T4TAB_H_
#endif // T4TAB_H_

