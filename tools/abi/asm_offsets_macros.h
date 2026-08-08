#ifndef BHARAT_ASM_OFFSETS_MACROS_H
#define BHARAT_ASM_OFFSETS_MACROS_H

#define DEFINE(sym, val) \
    asm volatile("\n.ascii \"-> " #sym " %0 " #val "\"\n" : : "i" (val))

#define BLANK() \
    asm volatile("\n.ascii \"->\"\n" : : )

#endif
