// Based on the https://cr.yp.to/siphash/siphash-20120918.pdf paper

#include "siphash.h"
#include "siphash_test_prg.h"


__data uint8_t v[32];


__code uint8_t v_init[32] = "\x73\x6f\x6d\x65\x70\x73\x65\x75\x64\x6f\x72\x61\x6e\x64\x6f\x6d\x6c\x79\x67\x65\x6e\x65\x72\x61\x74\x65\x64\x62\x79\x74\x65\x73";

void siphash_init(uint8_t * __xdata k[16]) {
    uint8_t idx = 16;
    do {
        v[idx] = k[idx];
    } while(--idx);
    idx = 16;
    do {
        v[idx+16] = k[idx];
    } while(--idx);

    idx = 32;
    do {
        v[idx] ^= v_init[idx];
    } while(--idx);
}

