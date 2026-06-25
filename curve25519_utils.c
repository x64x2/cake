#include "base.h"
#include "ccd.h"
  
void TrimSecretKey(U8 *X)
{
    X[0] &= 0xf8;
    X[31] = (X[31] | 0x40) & 0x7f;
}

U8* ReverseByteOrder(OUT U8 *Y, IN const U8 *X)
{
    int i;
    for (i = 0; i < 32; i++) Y[i] = X[31-i];
    return Y;
}

U32* BytesToWords(OUT U32 *Y, IN const U8 *X)
{
    int i, j;
    for (i = j = 0; j < 8; i += 4, j++)
    {
        Y[j] = ((U32)X[i+0]      ) |
               ((U32)X[i+1] <<  8) |
               ((U32)X[i+2] << 16) |
               ((U32)X[i+3] << 24);
    }
    return Y;
}

U8* WordsToBytes(OUT U8 *Y, IN const U32 *X)
{
    int i, j;
    for (i = j = 0; j < 8; j++)
    {
        Y[i++] = (U8)(X[j]      );
        Y[i++] = (U8)(X[j] >>  8);
        Y[i++] = (U8)(X[j] >> 16);
        Y[i++] = (U8)(X[j] >> 24);
    }
    return Y;
}
