// Compiler for PHP (aka KPHP)
// Copyright (c) 2024 LLC «V Kontakte»
// Distributed under the GPL v3 License, see LICENSE.notice.txt

#ifndef TYPE
  #ifdef __clang__
    #define TYPE(name) // .type not supported
  #else
    #define TYPE(name) .type name, @function;
  #endif
#endif

#define ALIGNARG(log2) 1 << log2

// Define an entry point visible from C/C++.
#define ENTRY_P2ALIGN(name, alignment)                                                                                                                         \
  .globl name;                                                                                                                                                 \
  TYPE(name).align ALIGNARG(alignment);                                                                                                                        \
  name:

// Common entry 16 byte aligns
#define ENTRY(name) ENTRY_P2ALIGN(name, 4)
#define END(name) .size name, .- name;

// Number of general registers
#define NUM_GENERAL_REG 23

// Register size
#define GREG_SIZE 8

// General registers
#define GREG_R8 0
#define GREG_R9 1
#define GREG_R10 2
#define GREG_R11 3
#define GREG_R12 4
#define GREG_R13 5
#define GREG_R14 6
#define GREG_R15 7
#define GREG_RDI 8
#define GREG_RSI 9
#define GREG_RBP 10
#define GREG_RBX 11
#define GREG_RDX 12
#define GREG_RAX 13
#define GREG_RCX 14
#define GREG_RSP 15
#define GREG_RIP 16
#define GREG_EFL 17
#define GREG_CSGSFS 18
#define GREG_ERR 19
#define GREG_TRAPNO 20
#define GREG_OLDMASK 21
#define GREG_CR2 22

// KPHP user context layout

/*  struct kcontext_t {               OFFSET*/
/*    uint64_t     flags                0   */
/*    kcontext_t*  link                 8   */
/*    kstack_t     stack                16  */
/*    struct kmcontext_t mcontext {         */
/*      kgregset_t  gregs               32  */ #define KCONTEXT_MCONTEXT_GREGS (32)
/*      kfpregset_t fpregs              216 */ #define KCONTEXT_MCONTEXT_FPREGS (KCONTEXT_MCONTEXT_GREGS + (GREG_SIZE * NUM_GENERAL_REG))
/*      uint64_t    reserved1[8]        224 */
/*    }                                     */
/*    struct kfpstate fpregs_mem {          */
/*      uint16_t cwd                    288 */ #define KCONTEXT_FPREGS_MEM (KCONTEXT_MCONTEXT_FPREGS + 8 + (8 * 8))
/*      uint16_t swd                    290 */
/*      uint16_t ftw                    292 */
/*      uint16_t fop                    294 */
/*      uint16_t rip                    296 */
/*      uint16_t rdp                    298 */
/*      uint16_t mxcsr                  300 */ #define KCONTEXT_FPREGS_MEM_MXCSR (KCONTEXT_FPREGS_MEM + (2 * 6))
/*      uint16_t mxcr_mask                  */
/*      kfpxreg st[8]                       */
/*      kxmmreg xmm[16]                     */
/*      uint32_t reserved1[24]              */
/*    }                                     */
/*  }                                       */

// Offset of register inside context
#define KCONTEXT_MCONTEXT_GREG_OFFSET(reg) (KCONTEXT_MCONTEXT_GREGS + ((reg) * GREG_SIZE))
#define oRBP KCONTEXT_MCONTEXT_GREG_OFFSET(GREG_RBP)
#define oRSP KCONTEXT_MCONTEXT_GREG_OFFSET(GREG_RSP)
#define oRBX KCONTEXT_MCONTEXT_GREG_OFFSET(GREG_RBX)
#define oR8 KCONTEXT_MCONTEXT_GREG_OFFSET(GREG_R8)
#define oR9 KCONTEXT_MCONTEXT_GREG_OFFSET(GREG_R9)
#define oR10 KCONTEXT_MCONTEXT_GREG_OFFSET(GREG_R10)
#define oR11 KCONTEXT_MCONTEXT_GREG_OFFSET(GREG_R11)
#define oR12 KCONTEXT_MCONTEXT_GREG_OFFSET(GREG_R12)
#define oR13 KCONTEXT_MCONTEXT_GREG_OFFSET(GREG_R13)
#define oR14 KCONTEXT_MCONTEXT_GREG_OFFSET(GREG_R14)
#define oR15 KCONTEXT_MCONTEXT_GREG_OFFSET(GREG_R15)
#define oRDI KCONTEXT_MCONTEXT_GREG_OFFSET(GREG_RDI)
#define oRSI KCONTEXT_MCONTEXT_GREG_OFFSET(GREG_RSI)
#define oRDX KCONTEXT_MCONTEXT_GREG_OFFSET(GREG_RDX)
#define oRAX KCONTEXT_MCONTEXT_GREG_OFFSET(GREG_RAX)
#define oRCX KCONTEXT_MCONTEXT_GREG_OFFSET(GREG_RCX)
#define oRIP KCONTEXT_MCONTEXT_GREG_OFFSET(GREG_RIP)
#define oEFL KCONTEXT_MCONTEXT_GREG_OFFSET(GREG_EFL)
#define oFPREGS KCONTEXT_MCONTEXT_FPREGS
#define oFPREGSMEM KCONTEXT_FPREGS_MEM
#define oMXCSR KCONTEXT_FPREGS_MEM_MXCSR
