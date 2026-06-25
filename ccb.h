#ifndef __ccb_h__
#define __ccb_h__

#include "base.h"

typedef U8 u8;
typedef S64 felem;

void curve25519_alice(u8 *alice_publickey, const u8 *secret, const u8 *basepoint);

#endif
