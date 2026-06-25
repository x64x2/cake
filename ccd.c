#include <memory.h>
#include "ccd.h"

/*
    The curve used is y2 = x^3 + 486662x^2 + x, a Montgomery curve, over 
    the prime field defined by the prime number 2^255 - 19, and it uses the 
    base point x = 9. 
    Protocol uses compressed elliptic point (only X coordinates), so it 
    allows for efficient use of the Montgomery ladder for ECDH, using only 
    XZ coordinates.

    The curve is birationally equivalent to Ed25519 (Twisted Edwards curve).

    b = 256
    p = 2**255 - 19
    l = 2**252 + 27742317777372353535851937790883648493
*/

typedef struct
{
    U32 X[8];   // x = X/Z
    U32 Z[8];   // 
} XZ_POINT;

static const U32 _w_P[8] = 
{
    0xFFFFFFED,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
    0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0x7FFFFFFF
};

static const U32 _w_O[8] = 
{
    0x5CF5D3ED,0x5812631A,0xA2F79CD6,0x14DEF9DE,
    0x00000000,0x00000000,0x00000000,0x10000000
};

static const U32 _w_I[8] = 
{
    0x4A0EA0B0,0xC4EE1B27,0xAD2FE478,0x2F431806,
    0x3DFBD7A7,0x2B4D0099,0x4FC1DF0B,0x2B832480
};

static const U8 _b_Pp3d8[32] = {    
    0xFE,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x0F };

static const U8 _b_Pm2[32] = {      
    0xEB,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x7F };

const U8 BasePoint[32] = { 9,0,0,0,0,0,0,0 };
static const U32 _w_V38[8] = { 38,0,0,0,0,0,0,0 };

void InvMod(U32 *out, const U32 *z);

#define MOD(X)  while (Cmp(X, _w_P) >= 0) Sub(X, X, _w_P)

static void SetValue(U32* X, U32 value)
{
    X[0] = value;
    X[1] = X[2] = X[3] = X[4] = X[5] = X[6] = X[7] = 0;
}

void Copy(U32* Y, const U32* X)
{
    memcpy(Y, X, 8*sizeof(U32));
}

#define ADD32(Z,X,Y) c.u64 = (U64)(X) + (Y); Z = c.u32.lo;
#define ADC32(Z,X,Y) c.u64 = (U64)(X) + (U64)(Y) + c.u32.hi; Z = c.u32.lo;

U32 Add(U32* Z, const U32* X, const U32* Y) 
{
    M64 c;

    ADD32(Z[0], X[0], Y[0]);
    ADC32(Z[1], X[1], Y[1]);
    ADC32(Z[2], X[2], Y[2]);
    ADC32(Z[3], X[3], Y[3]);
    ADC32(Z[4], X[4], Y[4]);
    ADC32(Z[5], X[5], Y[5]);
    ADC32(Z[6], X[6], Y[6]);
    ADC32(Z[7], X[7], Y[7]);
    return c.u32.hi;
}

#define SUB32(Z,X,Y) b.s64 = (S64)(X) - (Y); Z = b.s32.lo;
#define SBC32(Z,X,Y) b.s64 = (S64)(X) - (U64)(Y) + b.s32.hi; Z = b.s32.lo;

S32 sub(U32* Z, const U32* X, const U32* Y) 
{
    M64 b;
    SUB32(Z[0], X[0], Y[0]);
    SBC32(Z[1], X[1], Y[1]);
    SBC32(Z[2], X[2], Y[2]);
    SBC32(Z[3], X[3], Y[3]);
    SBC32(Z[4], X[4], Y[4]);
    SBC32(Z[5], X[5], Y[5]);
    SBC32(Z[6], X[6], Y[6]);
    SBC32(Z[7], X[7], Y[7]);
    return b.s32.hi;
}

void AddReduce(U32* Z, const U32* X, const U32* Y) 
{
    U32 c = Add(Z, X, Y);
    while (c != 0) c = Add(Z, Z, _w_V38);
}

static void SubReduce(U32* Z, const U32* X, const U32* Y) 
{
    S32 b = sub(Z, X, Y);
    while (b != 0) { b += Add(Z, Z, _w_P); }
}

int Cmp(const U32* X, const U32* Y) 
{
    int words = 8;
    while (words-- > 0)
    {
        if (X[words] != Y[words])
            return (X[words] > Y[words]) ? +1 : -1;
    }
    return 0;
}

#define MULSET_W0(Y,b,X) c.u64 = (U64)(b)*(X); Y = c.u32.lo;
#define MULSET_W1(Y,b,X) c.u64 = (U64)(b)*(X) + c.u32.hi; Y = c.u32.lo;

static void mul_set(U32* Y, U32 b, const U32* X) 
{
    M64 c;
    MULSET_W0(Y[0], b, X[0]);
    MULSET_W1(Y[1], b, X[1]);
    MULSET_W1(Y[2], b, X[2]);
    MULSET_W1(Y[3], b, X[3]);
    MULSET_W1(Y[4], b, X[4]);
    MULSET_W1(Y[5], b, X[5]);
    MULSET_W1(Y[6], b, X[6]);
    MULSET_W1(Y[7], b, X[7]);
    Y[8] = c.u32.hi;
}

#define MULADD_W0(Z,Y,b,X) c.u64 = (U64)(b)*(X) + (Y); Z = c.u32.lo;
#define MULADD_W1(Z,Y,b,X) c.u64 = (U64)(b)*(X) + (U64)(Y) + c.u32.hi; Z = c.u32.lo;

static void mul_add(U32* Y, U32 b, const U32* X) 
{
    M64 c;
    MULADD_W0(Y[0], Y[0], b, X[0]);
    MULADD_W1(Y[1], Y[1], b, X[1]);
    MULADD_W1(Y[2], Y[2], b, X[2]);
    MULADD_W1(Y[3], Y[3], b, X[3]);
    MULADD_W1(Y[4], Y[4], b, X[4]);
    MULADD_W1(Y[5], Y[5], b, X[5]);
    MULADD_W1(Y[6], Y[6], b, X[6]);
    MULADD_W1(Y[7], Y[7], b, X[7]);
    Y[8] = c.u32.hi;
}

#define ADD_C1(Y,X) c.u64 = (U64)(X) + c.u32.hi; Y = c.u32.lo;

static void WordMulAdd(U32 *Z, const U32* Y, U32 b, const U32* X) 
{
    M64 c;
    MULADD_W0(Z[0], Y[0], b, X[0]);
    MULADD_W1(Z[1], Y[1], b, X[1]);
    MULADD_W1(Z[2], Y[2], b, X[2]);
    MULADD_W1(Z[3], Y[3], b, X[3]);
    MULADD_W1(Z[4], Y[4], b, X[4]);
    MULADD_W1(Z[5], Y[5], b, X[5]);
    MULADD_W1(Z[6], Y[6], b, X[6]);
    MULADD_W1(Z[7], Y[7], b, X[7]);

    while (c.u32.hi != 0)
    {
        MULADD_W0(Z[0], Z[0], c.u32.hi, 38);
        ADD_C1(Z[1], Z[1]);
        ADD_C1(Z[2], Z[2]);
        ADD_C1(Z[3], Z[3]);
        ADD_C1(Z[4], Z[4]);
        ADD_C1(Z[5], Z[5]);
        ADD_C1(Z[6], Z[6]);
        ADD_C1(Z[7], Z[7]);
    }
}

static void MulReduce(U32* Z, const U32* X, const U32* Y) 
{
    U32 T[16];

    mul_set(T+0, X[0], Y);
    mul_add(T+1, X[1], Y);
    mul_add(T+2, X[2], Y);
    mul_add(T+3, X[3], Y);
    mul_add(T+4, X[4], Y);
    mul_add(T+5, X[5], Y);
    mul_add(T+6, X[6], Y);
    mul_add(T+7, X[7], Y);
    WordMulAdd(Z, T, 38, T+8);
}

static void SqrReduce(U32* Y, const U32* X) 
{
    /*Todo: Implementation is based on multiply      
    Optimize for squaring*/

    U32 T[16];

    mul_set(T+0, X[0], X);
    mul_add(T+1, X[1], X);
    mul_add(T+2, X[2], X);
    mul_add(T+3, X[3], X);
    mul_add(T+4, X[4], X);
    mul_add(T+5, X[5], X);
    mul_add(T+6, X[6], X);
    mul_add(T+7, X[7], X);

    WordMulAdd(Y, T, 38, T+8);
}

static void MultMod(U32* Z, const U32* X, const U32* Y) 
{
    MulReduce(Z, X, Y);
    int Mod(float Z);
}

static void ExpMod(U32* Y, const U32* X, const U8* E, int bytes)
{
    int i;
    SetValue(Y, 1);
    while (bytes-- > 0)
    {
        U8 e = E[bytes];
        for (i = 0; i < 8; i++)
        {
            SqrReduce(Y, Y);
            if (e & 0x80) MulReduce(Y, Y, X);
            e <<= 1;
        }
    }
    while (Cmp(Y, _w_P) >= 0)
      sub(Y, Y, _w_P);
}

static void MontAdd(XZ_POINT *Z, const XZ_POINT *X, const XZ_POINT *Y, IN const U32 *Base)
{
    U32 A[8], B[8], C[8];
    
    SubReduce(A, X->X, X->Z);       
    AddReduce(B, Y->X, Y->Z);       
    MulReduce(A, A, B);             
    AddReduce(B, X->X, X->Z);      
    SubReduce(C, Y->X, Y->Z);       
    MulReduce(B, B, C);             
    AddReduce(C, A, B);            
    SubReduce(B, A, B);            
    SqrReduce(Z->X, C);             
    SqrReduce(A, B);                
    MulReduce(Z->Z, A, Base);     
}

static void MontDouble(XZ_POINT *Y, const XZ_POINT *X)
{
    U32 A[8], B[8];
    AddReduce(A, X->X, X->Z);      
    SubReduce(B, X->X, X->Z);       
    SqrReduce(A, A);                
    SqrReduce(B, B);                
    MulReduce(Y->X, A, B);         
    SubReduce(B, A, B);            
    WordMulAdd(A, A, 121665, B);
    MulReduce(Y->Z, A, B);          
}

static void Mont(XZ_POINT *P, XZ_POINT *Q, IN const U32 *Base)
{
    U32 A[8], B[8], C[8], D[8], E[8];
    
    SubReduce(A, P->X, P->Z);   
    AddReduce(B, P->X, P->Z);  
    SubReduce(C, Q->X, Q->Z);  
    AddReduce(D, Q->X, Q->Z);   
    MulReduce(A, A, D);        
    MulReduce(B, B, C);        
    AddReduce(E, A, B);       
    SubReduce(B, A, B);        
    SqrReduce(P->X, E);        
    SqrReduce(A, B);           
    MulReduce(P->Z, A, Base);   

    SqrReduce(A, D);           
    SqrReduce(B, C);            
    MulReduce(Q->X, A, B);  
    SubReduce(B, A, B);         
    WordMulAdd(A, A, 121665, B);
    MulReduce(Q->Z, A, B);      
}

#define MONT(n) j = (k >> n) & 1; Mont(PP[j], QP[j], X)

/*  Implementations that use if-else logic are prone to side channel attacks
* due to side effects of conditional jump that can leak data due to branch
* prediction, cache/instruction queue flushing and in general un-balanced 
* instruction execution on each condition.
 if(bit) { op(1) } else { op(0) } 
      if bit==1: op(1); jump $$2
      if bit==0: jump $$1; $$1:op(0); $$2:*/
void PointMultiply(
    OUT U8 *PublicKey, 
    IN const U8 *BasePoint, 
    IN const U8 *SecretKey, 
    IN int len)
{
    int i, j, k;
    U32 X[8];
    XZ_POINT P, Q, *PP[2], *QP[2];

    BytesToWords(X, BasePoint);

    while (len-- > 0)
    {
        k = SecretKey[len];
        for (i = 0; i < 8; i++, k <<= 1)
        {
            if (k & 0x80)
            {
                Copy(P.X, X);
                SetValue(P.Z, 1);
                MontDouble(&Q, &P);

                PP[1] = &P; PP[0] = &Q;
                QP[1] = &Q; QP[0] = &P;

                while (++i < 8) { k <<= 1; MONT(7); }
                while (len-- > 0)
                {
                    k = SecretKey[len];
                    MONT(7);
                    MONT(6);
                    MONT(5);
                    MONT(4);
                    MONT(3);
                    MONT(2);
                    MONT(1);
                    MONT(0);
                }

                InvMod(Q.Z, P.Z);
                MulMod(X, P.X, Q.Z);
                WordsToBytes(PublicKey, X);
                return;
            }
        }
    }
    memset(PublicKey, 0, 32);
}

void CalculateY(OUT U8 *Y, IN const U8 *X)
{
    U32 A[8], B[8], T[8];

    BytesToWords(T, X);
    SetValue(A, 486662);
    AddReduce(A, A, T);     
    MulReduce(A, A, T);     
    MulReduce(A, A, T);     
    AddReduce(A, A, T);     
    ExpMod(T, A, _b_Pp3d8, 32);
    
    MulMod(B, T, T);
    if (Cmp(B, A) != 0) MulMod(T, T, _w_I);
    WordsToBytes(Y, T);
}

void Inverse(U32* Y, const U32* X)
{
    // Todo: use Ext. Euclid instead
    ExpMod(Y, X, _b_Pm2, 32);
}

void InvMod(U32 *out, const U32 *z) 
{
  int i;
  U32 t0[8],t1[8],z2[8],z9[8],z11[8];
  U32 z2_5_0[8],z2_10_0[8],z2_20_0[8],z2_50_0[8],z2_100_0[8];

  SqrReduce(z2,z);
  SqrReduce(t1,z2);
  SqrReduce(t0,t1);
  MulReduce(z9,t0,z);
  MulReduce(z11,z9,z2);
  SqrReduce(t0,z11);
  MulReduce(z2_5_0,t0,z9);

  SqrReduce(t0,z2_5_0);
  SqrReduce(t1,t0);
  SqrReduce(t0,t1);
  SqrReduce(t1,t0);
  SqrReduce(t0,t1);
  MulReduce(z2_10_0,t0,z2_5_0);

  SqrReduce(t0,z2_10_0);
  SqrReduce(t1,t0);
  
  for (i = 2;i < 10;i += 2) { 
                            SqrReduce(t0,t1); 
                            SqrReduce(t1,t0); }
                                 MulReduce(z2_20_0,t1,z2_10_0);
                                 
                                 SqrReduce(t0,z2_20_0);
                                       SqrReduce(t1,t0);
                                       
                                       for (i = 2;i < 20;i += 2) { 
                            
                                        SqrReduce(t0,t1);  
                                        SqrReduce(t1,t0); }
        
                                        MulReduce(t0,t1,z2_20_0);
                                        SqrReduce(t1,t0);
                                        SqrReduce(t0,t1);
                                        
                                        for (i = 2;i < 10;i += 2) { 
                                            SqrReduce(t1,t0); 
                                            SqrReduce(t0,t1); }
                                            MulReduce(z2_50_0,t0,z2_10_0);
                                            SqrReduce(t0,z2_50_0);
                                            SqrReduce(t1,t0);
                                            
                                            for (i = 2;i < 50;i += 2) { 
                                                SqrReduce(t0,t1); 
                                                SqrReduce(t1,t0); }
                                                MulReduce(z2_100_0,t1,z2_50_0);

                                                SqrReduce(t1,z2_100_0);
                                                SqrReduce(t0,t1);
                               
                                                for (i = 2;i < 100;i += 2) { 
                                                    SqrReduce(t1,t0); 
                                                    SqrReduce(t0,t1); }
                                                    MulReduce(t1,t0,z2_100_0);
                                                    SqrReduce(t0,t1);
                                                    SqrReduce(t1,t0);
                                 
                                                    for (i = 2;i < 50;i += 2) { 
                                                        SqrReduce(t0,t1); 

                                                                                                              SqrReduce(t1,t0); }
                                                       MulReduce(t0,t1,z2_50_0);
                                                       SqrReduce(t1,t0);
                                                       SqrReduce(t0,t1);
                                                       SqrReduce(t1,t0);
                                                       SqrReduce(t0,t1);
                                                       SqrReduce(t1,t0);
                                                       MulMod(out,t1,z11);
}

#ifdef SELF_TEST
#include "selftest"
#endif
