                
                implementation of elliptic curve 25519

                Timing for point multiplication:
                ```
                debian-64: gcc (Debian 4.4.5-8) 4.4.5
                 alice: 878752.956 cycles = 258.457 usec @3.4GHz
                 bob: 557709.212 cycles = 164.032 usec @3.4GHz -- 36.53%
                 
                 debian-32: gcc (Debian 4.4.5-8) 4.4.5
                 alice: 900336.794 cycles = 264.805 usec @3.4GHz
                 bob: 552462.788 cycles = 162.489 usec @3.4GHz -- 38.64%
                 
                 Building:
                 ---------
                 The design uses a configurable switch that defines the byte order of the
                 target CPU. In default mode it uses Little-endian byte order. You need to
                 change this configuration for Big-endian targets by setting ECP_BIG_ENDIAN
                 switch (see Makefile).
                 
                 Second configurable switch controls usage of TSC (Time Stamp Counter). It is 
                only used as a high resolution timer for performance measurements. You need 
                to turn ECP_NO_TSC switch on if your target does not support it.
