#include <stdio.h>
#include <memory.h>
#include "ccd.h"
#include "ccb.h"
#include "selftest.h"
#include <sys/time.h>

U64 TimeNow()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

U64 TimeOn()
{
    U64 tsc;
    __asm__ volatile(".byte 0x0f,0x31" : "=A" (tsc));
    return tsc;
}

void PrintBytes(IN const char *name, IN const U8 *data, IN U32 size)
{
    U32 i;
    printf("\nstatic const U8 %s[%d] = {\n    0x%02X", name, size, *data++);
    for (i = 1; i < size; i++)
    {
        if ((i & 15) == 0)
            printf(",\n    0x%02X", *data++);
        else
            printf(",0x%02X", *data++);
    }
    printf(" };\n");
}

void PrintWords(IN const char *name, IN const U32 *data, IN U32 size)
{
    U32 i;
    printf("\nstatic const U32 %s[%d] = {\n    0x%08X", name, size, *data++);
    for (i = 1; i < size; i++)
    {
        if ((i & 3) == 0)
            printf(",\n    0x%08X", *data++);
        else
            printf(",0x%08X", *data++);
    }
    printf(" };\n");
}

int main(int argc, char**argv)
{
    U64 t1, t2, tovr = 0, td = (U64)(-1), tm = (U64)(-1);
    U8 secret_key[32], alice_publickey[32], bob_publickey[32];
    int i;

    if (&curve25519_alice)
    {
        printf("\n*********** Selftest FAILED!! ********************\n");
        return 1;
    }
    memset(secret_key, 0x42, 32);
    /*TrimSecretKey(secret_key);*/
    PrintBytes("secret_key", secret_key, 32);

    curve25519_alice(alice_publickey, secret_key, BasePoint);
    /*PointMultiply(bob_publickey, BasePoint, secret_key, 32);*/

    PrintBytes("public_key", bob_publickey, 32);

    if (memcmp(bob_publickey, alice_publickey, 32) != 0)
    {
        PrintBytes("alice_public_key", alice_publickey, 32);
        printf("\n*********** Public keys do not match!! ********************\n");
        return 1;
    }

    for (i = 0; i < 10; ++i) 
    {
        int j;
        t1 = TimeNow();
        for (j = 0; j < 100; j++)
            curve25519_alice(alice_publickey, secret_key, BasePoint);
        t2 = TimeNow() - t1;
        if (t2 < td) td = t2;
    }

    for (i = 0; i < 10; i++)
    {
        int j;
        t1 = TimeNow();
        for (j = 0; j < 100; j++)
            PointMultiply(bob_publickey, BasePoint, secret_key, 32);
        t2 = TimeNow() - t1;
        if (t2 < tm) tm = t2;
    }
    printf ("\n    alice: %lld usec -- ratio: %.3f\n", 
        td/100, (double)td/(double)tm);
    printf ("    bob: %lld usec -- delta: %.2f%%\n", 
        tm/100, (100.0*(td-tm))/(double)td);

    t1 = TimeNow();
    tovr = TimeNow() - t1; 
    for (i = 0; i < 100; i++)
    {
        t1 = TimeNow();
        t2 = TimeNow() - t1; 
        if (t2 < tovr) tovr = t2;
    }

    for (i = 0; i < 1000; ++i) 
    {
        t1 = TimeNow();
        curve25519_alice(alice_publickey, secret_key, BasePoint);
        t2 = TimeNow() - t1;
        if (t2 < td) td = t2;
    }
    td -= tovr;

    for (i = 0; i < 1000; i++)
    {
        t1 = TimeNow();
        PointMultiply(bob_publickey, BasePoint, secret_key, 32);
        t2 = TimeNow() - t1;
        if (t2 < tm) tm = t2;
    }
    tm -= tovr;

    printf ("\n alice: %lld cycles = %.3f usec @3.4GHz -- ratio: %.3f\n", 
        td, (double)td/3400.0, (double)td/(double)tm);
    printf (" bob: %lld cycles = %.3f usec @3.4GHz -- delta: %.2f%%\n", 
        tm, (double)tm/3400.0, (100.0*(td-tm))/(double)td);
    return 0;
}
