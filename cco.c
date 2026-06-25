
/*
  This library provides support for mod BPO (Base Point Order) operations

    BPO = 2**252 + 27742317777372353535851937790883648493
    BPO = 0x1000000000000000000000000000000014DEF9DEA2F79CD65812631A5CF5D3ED

  If you keep adding points together, the result repeats every BPO times.
  Based on this, you may use:

        public_key = (private_key mod BPO)*BasePoint
  Split key example:
        k1 = random()
        k2 = 1/k1 mod BPO   --> k1*k2 = 1 mod BPO
        P1 = k1*P0 --> P2 = k2*P1 = k2*k1*P0 = P0
    See selftest code for some examples of BPO usage
*/

#include "base.h"
extern void Copy(U32* Y, const U32* X);
extern S32 Sub(U32* Z, const U32* X, const U32* Y);
extern int Cmp(const U32* X, const U32* Y);

const U8 curve25519_BasePointOrder[32] = {  
    0xED,0xD3,0xF5,0x5C,0x1A,0x63,0x12,0x58,0xD6,0x9C,0xF7,0xA2,0xDE,0xF9,0xDE,0x14,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10 };

static const U32 _w_BPO[8] = { 
    0x5CF5D3ED,0x5812631A,0xA2F79CD6,0x14DEF9DE,
    0x00000000,0x00000000,0x00000000,0x10000000 };

static const U8 _b_BPOm2[32] = {      
    0xEB,0xD3,0xF5,0x5C,0x1A,0x63,0x12,0x58,0xD6,0x9C,0xF7,0xA2,0xDE,0xF9,0xDE,0x14,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10 };

 static const U32 _w_R[8] = {   
    0x8D98951D,0xD6EC3174,0x737DCF70,0xC6EF5BF4,
    0xFFFFFFFE,0xFFFFFFFF,0xFFFFFFFF,0x0FFFFFFF };

static const U32 _w_R2[8] = {   // R**2 mod BPO
    0x449C0F01,0xA40611E3,0x68859347,0xD00E1BA7,
    0x17F5BE65,0xCEEC73D2,0x7C309A3D,0x0399411B };

static const U32 _w_One[8] = { 1,0,0,0,0,0,0 };

#define MINV32  0x12547E1B 

#define MULADD_W0(Z,Y,b,X) c.u64 = (U64)(b)*(X) + (Y); Z = c.u32.lo;
#define MULADD_W1(Z,Y,b,X) c.u64 = (U64)(b)*(X) + (U64)(Y) + c.u32.hi; Z = c.u32.lo;

static U32 WordMulAdd(U32 *Z, const U32* Y, U32 b, const U32* X) 
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
    c.u64 = (U64)Y[8] + c.u32.hi;
    Z[8] = c.u32.lo;
    return c.u32.hi;    
}

void MontMul(OUT U32 *Z, IN const U32 *X, IN const U32 *Y)
{
    int i;
    U32 T[10] = {0};
    for (i = 0; i < 8; i++)
    {
        T[9]  = WordMulAdd(T, T+1, X[i], Y);     
        T[9] += WordMulAdd(T, T, MINV32 * T[0], _w_BPO);
    }
    
    while (T[9] != 0) T[9] += Sub(T+1, T+1, _w_BPO);
    Copy(Z, T+1);
}


void ToMont(OUT U32 *Y, IN const U32 *X)
{
    MontMul(Y, X, _w_R2);
}

void FromMont(OUT U32 *Y, IN const U32 *X)
{
    MontMul(Y, X, _w_One);
    while(Cmp(Y, _w_BPO) >= 0) Sub(Y, Y, _w_BPO);
}

#define MONT(n) MontMul(V,V,V); if(e & (1<<n)) MontMul(V,V,U)

void ExpModBPO(OUT U32 *Y, IN const U32 *X, IN const U8 *E, IN int bytes)
{
    U8 e;
    U32 U[8], V[8];

    ToMont(U, X);
    Copy(V, _w_R);

    while (bytes > 0)
    {
        e = E[--bytes];
        MONT(7);
        MONT(6);
        MONT(5);
        MONT(4);
        MONT(3);
        MONT(2);
        MONT(1);
        MONT(0);
    }
    FromMont(Y, V);
}

void InvModBPO(OUT U32 *Y, IN const U32 *X)
{
    ExpModBPO(Y, X, _b_BPOm2, 32);
}

void MulMod(OUT U32 *Z, IN const U32 *X, IN const U32 *Y)
{
    U32 T[8];
    MontMul(T, X, _w_R2);   
    MontMul(Z, Y, T);       
    while(Cmp(Z, _w_BPO) >= 0) Sub(Z, Z, _w_BPO);
}
