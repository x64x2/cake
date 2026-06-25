#ifndef __ccd_h__
#define __ccd_h__

#include "base.h"

extern const U8 BasePoint[32];

static void PointMultiply(OUT U8 *Q, IN const U8 *P, IN const U8 *K, IN int len);
static void CalculateY(OUT U8 *Y, IN const U8 *X);
static void TrimSecretKey(U8 *X);

U8* ReverseByteOrder(OUT U8 *Y, IN const U8 *X);
U32* BytesToWords(OUT U32 *Y, IN const U8 *X);
U8* WordsToBytes(OUT U8 *Y, IN const U32 *X);
void MontMul(OUT U32 *Z, IN const U32 *X, IN const U32 *Y);
void ToMont(OUT U32 *Y, IN const U32 *X);
void FromMont(OUT U32 *Y, IN const U32 *X);
void ExpModBPO(OUT U32 *Y, IN const U32 *X, IN const U8 *E, IN int bytes);
void InvModBPO(OUT U32 *Y, IN const U32 *X);
void MulMod(OUT U32 *Z, IN const U32 *X, IN const U32 *Y);

#endif