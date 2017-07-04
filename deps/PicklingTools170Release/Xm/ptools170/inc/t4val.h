#ifndef T4VAL_H_

#include <primitive.h>
#include <ochashtable.h>
#include <ocval.h>
#include <serialization.h>

#define DEFAULT_VRBKEY "VRBKEY"

PTOOLS_BEGIN_NAMESPACE

void initT4Val(CPHEADER& hcb, string name, int_4 hcbfmode);
void sendT4Val(CPHEADER& hcb, const Val& v, 
	       string vrb_key = string(DEFAULT_VRBKEY),
	       Serialization_e ser=SERIALIZE_OC);
bool recvT4Val(CPHEADER& hcb, Val& v, 
	       string vrb_key = string(DEFAULT_VRBKEY),
	       Serialization_e ser=SERIALIZE_OC);

PTOOLS_END_NAMESPACE

#define T4VAL_H_
#endif // T4VAL_H_

