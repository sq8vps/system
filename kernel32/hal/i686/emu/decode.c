#include "state.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdbit.h>
#include "op.h"
#include "emu.h"
#include "hal/i686/ioport.h"

/**
 * @brief Parameters for instructions to simplify operand parsing
 */
static const uint32_t I686OpcodeParams[] = 
{
    //add/adc/and/sub instructions
    [0x00] = REG_MOD_RM | BYTE_OPERAND, //add r/m8,r8
    [0x01] = REG_MOD_RM, //add r/m16,m16; add r/m32,r32
    [0x02] = REG_MOD_RM | BYTE_OPERAND, //add r8,rm8
    [0x03] = REG_MOD_RM, //add r16,r/m16; add r32,r/m32
    [0x04] = IMM_BYTE | BYTE_OPERAND, //add al,imm8
    [0x05] = IMM_WORD_DWORD, //add ax,imm16; add eax,imm32
    [0x10] = REG_MOD_RM | BYTE_OPERAND, //adc r/m8,r8
    [0x11] = REG_MOD_RM, //adc r/m16,m16; adc r/m32,r32
    [0x12] = REG_MOD_RM | BYTE_OPERAND, //adc r8,rm8
    [0x13] = REG_MOD_RM, //adc r16,r/m16; adc r32,r/m32
    [0x14] = IMM_BYTE | BYTE_OPERAND, //adc al,imm8
    [0x15] = IMM_WORD_DWORD, //adc ax,imm16; adc eax,imm32
    [0x80] = MOD_RM_ONLY | IMM_BYTE | BYTE_OPERAND, //add/adc/and/sub/sbb/or/cmp r/m8,imm8
    [0x81] = MOD_RM_ONLY | IMM_WORD_DWORD, //add/adc/and/sub/sbb/or/cmp r/m16,imm16; add/adc/and/sub/sbb/or/cmp r/m32,imm32
    [0x83] = MOD_RM_ONLY | IMM_BYTE, //add/adc/and/sub/sbb/or/cmp r/m16,imm8; add/adc/and/sub/sbb/or/cmp r/m32,imm8

    //mov instructions
    [0x88] = REG_MOD_RM | BYTE_OPERAND, //mov r/m8,r8
    [0x89] = REG_MOD_RM, //mov r/m16,r16; mov r/m32,r32
    [0x8A] = REG_MOD_RM | BYTE_OPERAND, //mov r8,r/m8
    [0x8B] = REG_MOD_RM, //mov r16,r/m16; mov r32,r/m32
    [0x8C] = REG_MOD_RM | SEG_REG, //mov r/16/r32/m16,Sreg
    [0x8E] = REG_MOD_RM | SEG_REG, //mov Sreg,r/m16
    [0xA0] = MOFFS_BYTE, //mov al,moffs8
    [0xA1] = MOFFS_WORD_DWORD, //mov ax,moffs16; mov eax,moffs32
    [0xA2] = MOFFS_BYTE, //mov moffs8,al
    [0xA3] = MOFFS_WORD_DWORD, //mov moffs16,ax; mov moffs32,eax
    [0xB0 ... 0xB7] = REG_IN_OPCODE | IMM_BYTE | BYTE_OPERAND, //mov r8,imm8
    [0xB8 ... 0xBF] = REG_IN_OPCODE | IMM_WORD_DWORD, //mov r16,im16; mov r32,imm32
    [0xC6] = MOD_RM_ONLY | IMM_BYTE | BYTE_OPERAND | (0 << 29), //mov r/m8,imm8
    [0xC7] = MOD_RM_ONLY | IMM_WORD_DWORD | (0 << 29), //mov r/m16,imm16; mov r/m32,imm32

    //sub
    [0x28] = REG_MOD_RM | BYTE_OPERAND, //add r/m8,r8
    [0x29] = REG_MOD_RM, //add r/m16,m16; add r/m32,r32
    [0x2A] = REG_MOD_RM | BYTE_OPERAND, //add r8,rm8
    [0x2B] = REG_MOD_RM, //add r16,r/m16; add r32,r/m32
    [0x2C] = IMM_BYTE | BYTE_OPERAND, //add al,imm8
    [0x2D] = IMM_WORD_DWORD, //add ax,imm16; add eax,imm32

    //ascii adjust...
    [0x37] = 0, //aaa
    [0xD5] = IMM_BYTE, //aad
    [0xD4] = IMM_BYTE, //aam
    [0x3F] = 0, //aas

    //logical (bitwise) operations
    [0x20] = REG_MOD_RM | BYTE_OPERAND, //and r/m8,r8
    [0x21] = REG_MOD_RM, //and r/m16,m16; and r/m32,r32
    [0x22] = REG_MOD_RM | BYTE_OPERAND, //and r8,rm8
    [0x23] = REG_MOD_RM, //and r16,r/m16; and r32,r/m32
    [0x24] = IMM_BYTE | BYTE_OPERAND, //and al,imm8
    [0x25] = IMM_WORD_DWORD, //and ax,imm16; and eax,imm32

    [0x08] = REG_MOD_RM | BYTE_OPERAND, //or r/m8,r8
    [0x09] = REG_MOD_RM, //or r/m16,r16; or r/m32,r32
    [0x0A] = REG_MOD_RM | BYTE_OPERAND, //or r8,r/m8
    [0x0B] = REG_MOD_RM, //or r16,r/m16; or r32,r/m32
    [0x0C] = IMM_BYTE | BYTE_OPERAND, //or al,imm8
    [0x0D] = IMM_WORD_DWORD, //or ax,imm16; or eax,imm32    

    [0x30] = REG_MOD_RM | BYTE_OPERAND, //xor r/m8,r8
    [0x31] = REG_MOD_RM, //xor r/m16,r16; xor r/m32,r32
    [0x32] = REG_MOD_RM | BYTE_OPERAND, //xor r8,r/m8
    [0x33] = REG_MOD_RM, //xor r16,r/m16; xor r32,r/m32    
    [0x34] = IMM_BYTE | BYTE_OPERAND, //xor al,imm8
    [0x35] = IMM_WORD_DWORD, //xor ax,imm16; xor eax,imm32

    [0xD0] = MOD_RM_ONLY | BYTE_OPERAND, //rcl/rcr/rol/ror/shr/shl/sar/sal r/m8,1
    [0xD2] = MOD_RM_ONLY | BYTE_OPERAND, //rcl/rcr/rol/ror/shr/shl/sar/sal  r/m8,c
    [0xC0] = MOD_RM_ONLY | IMM_BYTE | BYTE_OPERAND, //rcl/rcr/rol/ror/shr/shl/sar/sal  r/m8,imm8
    [0xD1] = MOD_RM_ONLY, //rcl/rcr/rol/ror/shr/shl/sar/sal  r/m16,1; rcl/rcr/rol/ror/shr/shl/sar/sal  r/m32,1
    [0xD3] = MOD_RM_ONLY, //rcl/rcr/rol/ror/shr/shl/sar/sal  r/m16,cl; rcl/rcr/rol/ror/shr/shl/sar/sal  r/m32,cl
    [0xC1] = MOD_RM_ONLY | IMM_BYTE, //rcl/rcr/rol/ror/shr/shl/sar/sal  r/m16,imm8; rcl/rcr/rol/ror/shr/shl/sar/sal  r/m32,imm8

    [0xA8] = IMM_BYTE, //test al,imm8
    [0xA9] = IMM_WORD_DWORD, //test ax,imm16; test eax,imm32
    [0x84] = REG_MOD_RM | BYTE_OPERAND, //test r/m8,r8
    [0x85] = REG_MOD_RM, //test r/m16,r16; test r/m32,r32

    //compare
    [0x38] = REG_MOD_RM | BYTE_OPERAND, //cmp r/m8,r8
    [0x39] = REG_MOD_RM, //cmp r/m16,m16; cmp r/m32,r32
    [0x3A] = REG_MOD_RM | BYTE_OPERAND, //cmp r8,rm8
    [0x3B] = REG_MOD_RM, //cmp r16,r/m16; cmp r32,r/m32
    [0x3C] = IMM_BYTE | BYTE_OPERAND, //cmp al,imm8
    [0x3D] = IMM_WORD_DWORD, //cmp ax,imm16; cmp eax,imm32

    [0x98] = 0, //cbw
    [0xF8] = 0, //clc
    [0xFC] = 0, //cld
    [0xFA] = 0, //cli
    [0xF5] = 0, //cmc
    [0xF9] = 0, //stc
    [0xFD] = 0, //std
    [0xFB] = 0, //sti

    [0x99] = 0, //cwd; cdq

    [0x27] = 0, //daa
    [0x2F] = 0, //das

    //incrementation, decrementation, jumps
    [0xFE] = MOD_RM_ONLY | BYTE_OPERAND, //inc r/m8; dec r/m8
    [0xFF] = MOD_RM_ONLY, //inc r/m16; inc r/m32; dec r/m16; dec r/m32; 
        //near absolute indirect jumps: jmp r/m16; jmp r/m32; far absolute indirect jumps: jmp m16:16; jmp m16:32
        //do not dereference memory, as jumps may require non-standard lengths
        //also equvialent calls
        //also push r/m16; push r/m32
    [0x40 ... 0x47] = REG_IN_OPCODE, //inc r16; inc r32
    [0x48 ... 0x4F] = REG_IN_OPCODE, //dec r16; dec r32

    //multiplication, division, numerical negation, bitwise negation, test
    [0xF6] = MOD_RM_ONLY | BYTE_OPERAND, //(i)div r/m8; (i)mul r/m8; neg r/m8; not r/m8; test r/m8,imm8
    [0xF7] = MOD_RM_ONLY, //(i)div r/m16; (i)div r/m32; (i)mul r/m16; (i)mul r/m32; neg r/m16; neg r/m32; not r/m16; not r/m32; test r/m16,imm16; test r/m32,imm32

    //multiplication
    [0x69] = REG_MOD_RM | IMM_WORD_DWORD, //imul r16,r/m16,imm16; imul r32,r/m32,imm32; 
    [0x6B] = REG_MOD_RM | IMM_BYTE, //imul r16,r/m16,imm8; imul r32,r/m32,imm8; 

    //I/O ports
    [0xE4] = IMM_BYTE | BYTE_OPERAND, //in al,imm8
    [0xE5] = IMM_BYTE | BYTE_OPERAND, //in ax,imm8; in eax,imm8
    [0xEC] = 0, //in al,dx
    [0xED] = 0, //in ax,dx; in eax,dx
    [0xE6] = IMM_BYTE, //out imm8, al
    [0xE7] = IMM_BYTE, //out imm8,ax; out imm8,eax
    [0xEE] = 0, //out dx,al
    [0xEF] = 0, //out dx,ax; out dx,eax
    [0x6C] = 0, //insb
    [0x6D] = 0, //insw; insd
    [0x6E] = 0, //outsb
    [0x6F] = 0, //outsw, outsd


    //interrupts
    [0xCC] = 0, //int3
    [0xCD] = IMM_BYTE, //int imm8
    [0xCE] = 0, //into
    [0xF1] = 0, //int1

    //returns
    [0xCF] = 0, //iret, iretd
    [0xC3] = 0, //ret near
    [0xCB] = 0, //ret far
    [0xC2] = IMM_WORD_DWORD, //ret imm16 (near)
    [0xCA] = IMM_WORD_DWORD, //ret imm16 (far)

    //jumps and loops
    [0xEB] = COFFS_BYTE, //jmp rel8 - short
    [0xE9] = COFFS_WORD_DWORD, //jmp rel16; jmp rel32 - near relative
    [0xEA] = COFFS_DWORD_PWORD, //jmp ptr16:16; jmp ptr16:32 - far absolute
    [0xE3] = COFFS_BYTE, //jcxz rel8; jecxz rel8
    [0x77] = COFFS_BYTE, //ja rel8
    [0x73] = COFFS_BYTE, //jae rel8 = jnb rel8
    [0x72] = COFFS_BYTE, //jb rel8 = jc rel8 = jnae rel8
    [0x76] = COFFS_BYTE, //jbe rel8 = jna rel8
    [0x74] = COFFS_BYTE, //je rel8 = jz rel8
    [0x7F] = COFFS_BYTE, //jg rel8 = jnle rel8
    [0x7D] = COFFS_BYTE, //jge rel8 = jnl rel8
    [0x7C] = COFFS_BYTE, //jl rel8 = jnge rel8
    [0x7E] = COFFS_BYTE, //jle rel8 = jng rel8
    [0x75] = COFFS_BYTE, //jne rel8 = jnz rel8
    [0x71] = COFFS_BYTE, //jno rel8
    [0x7B] = COFFS_BYTE, //jnp rel8 = jpo rel8
    [0x79] = COFFS_BYTE, //jns rel8
    [0x70] = COFFS_BYTE, //jo rel8
    [0x7A] = COFFS_BYTE, //jp rel8 = jpe rel8
    [0x78] = COFFS_BYTE, //js rel8
    [0xE2] = COFFS_BYTE, //loop rel8
    [0xE1] = COFFS_BYTE, //loope rel8
    [0xE0] = COFFS_BYTE, //loopne rel8

    //calls
    [0xE8] = COFFS_WORD_DWORD, //call rel16; call rel32 - call near, relative
    [0x9A] = COFFS_DWORD_PWORD, //call ptr16:16; call ptr16:32, call far, absolute

    //stack
    [0x8F] = MOD_RM_ONLY, //pop r/m16; pop r/m32
    [0x58 ... 0x5F] = REG_IN_OPCODE, //pop r16; pop r32
    [0x1F] = 0, //pop ds
    [0x07] = 0, //pop es
    [0x17] = 0, //pop ss,
    [0x9D] = 0, //popf; popfd
    [0x50 ... 0x57] = REG_IN_OPCODE, //push r16; push r32
    [0x6A] = IMM_BYTE, //push imm8
    [0x68] = IMM_WORD_DWORD, //push imm16; push imm32
    [0x0E] = 0, //push cd
    [0x16] = 0, //push ss
    [0x1E] = 0, //push ds
    [0x06] = 0, //push es
    [0x9C] = 0, //pushf; pushfd
    [0x61] = 0, //popa; popad
    [0x60] = 0, //pusha; pushad

    //load far pointer
    [0xC5] = REG_MOD_RM | NO_DEREFERENCE, //lds r16,m16:16; lds r32,m16:32
    [0xC4] = REG_MOD_RM | NO_DEREFERENCE, //les r16,m16:16; les r32,m16:32

    //load effective address
    [0x8D] = REG_MOD_RM | NO_DEREFERENCE | NO_SEGMENT, //lea r16,m; lea r32,m

    //exchange
    [0x90 ... 0x97] = REG_IN_OPCODE, //xchg ax,r16; xchg eax,r32
    [0x86] = REG_MOD_RM | BYTE_OPERAND, //xchg r/m8,r8
    [0x87] = REG_MOD_RM, //xchg r/m16,r16; xchg r/m32,r32

    //string
    [0xAC] = 0, //lodsb
    [0xAD] = 0, //lodsw; lodsd
    [0xA4] = 0, //movsb
    [0xA5] = 0, //movsw, movsd
    [0xA6] = REPE_REPZ, //cmpsb
    [0xA7] = REPE_REPZ, //cmpsw; cmpsd
    [0xAE] = REPE_REPZ, //scasb
    [0xAF] = REPE_REPZ, //scasw, scasd
    [0xAA] = 0, //stosb
    [0xAB] = 0, //stosw; stosd

    //function handling
    [0xC8] = 0, //enter imm16,imm8
    [0xC9] = 0, //leave

    //other
    [0xF4] = 0, //hlt
    [0x9F] = 0, //lahf
    [0x9E] = 0, //sahf
    [0xD7] = 0, //xlatb
    [0x62] = REG_MOD_RM | NO_DEREFERENCE, //bound r16,m32&32; bound r32,m32&32
};

/**
 * @brief Parameters for extended instructions to simplify operand parsing
 */
static const uint32_t I686ExtendedOpcodeParams[] = 
{
    //multiplication
    [0xAF] = REG_MOD_RM, //imul r16,r/m16; imul r32,r/m32 

    //conditional jumps
    [0x87] = COFFS_WORD_DWORD, //ja rel16/13
    [0x83] = COFFS_WORD_DWORD, //jae rel16/13 = jnb rel16/13
    [0x82] = COFFS_WORD_DWORD, //jb rel16/13 = jc rel16/13 = jnae rel16/13
    [0x86] = COFFS_WORD_DWORD, //jbe rel16/13 = jna rel16/13
    [0x84] = COFFS_WORD_DWORD, //je rel16/13 = jz rel16/13
    [0x8F] = COFFS_WORD_DWORD, //jg rel16/13 = jnle rel16/13
    [0x8D] = COFFS_WORD_DWORD, //jge rel16/13 = jnl rel16/13
    [0x8C] = COFFS_WORD_DWORD, //jl rel16/13 = jnge rel16/13
    [0x8E] = COFFS_WORD_DWORD, //jle rel16/13 = jng rel16/13
    [0x85] = COFFS_WORD_DWORD, //jne rel16/13 = jnz rel16/13
    [0x81] = COFFS_WORD_DWORD, //jno rel16/13
    [0x8B] = COFFS_WORD_DWORD, //jnp rel16/13 = jpo rel16/13
    [0x89] = COFFS_WORD_DWORD, //jns rel16/13
    [0x80] = COFFS_WORD_DWORD, //jo rel16/13
    [0x8A] = COFFS_WORD_DWORD, //jp rel16/13 = jpe rel16/13
    [0x88] = COFFS_WORD_DWORD, //js rel16/13

    //load far pointer
    [0xB2] = REG_MOD_RM | NO_DEREFERENCE, //lss r16,m16:16; lss r32,m16:32
    [0xB4] = REG_MOD_RM | NO_DEREFERENCE, //lfs r16,m16:16; lfs r32,m16:32
    [0xB5] = REG_MOD_RM | NO_DEREFERENCE, //lgs r16,m16:16; lgs r32,m16:32

    //stack
    [0xA1] = 0, //pop fs
    [0xA9] = 0, //pop gs
    [0xA0] = 0, //push fs
    [0xA8] = 0, //push gs

    //bit test
    [0xA3] = REG_MOD_RM, //bt r/m16,r16; bt r/m32,r32
    [0xAB] = REG_MOD_RM, //bts r/m16,r16; bts r/m32,r32
    [0xB3] = REG_MOD_RM, //btr r/m16,r16; btr r/m32,r32
    [0xBB] = REG_MOD_RM, //btc r/m16,r16; btc r/m32,r32
    [0xBA] = MOD_RM_ONLY | IMM_BYTE, //bt/bts/btr/btc r/m16,imm8; bt/bts/btr/btc r/m32,imm8

    //bit scan
    [0xBC] = REG_MOD_RM, //bsf r16,r/m16; bsf r32,r/m32
    [0xBD] = REG_MOD_RM, //bsr r16,r/m16; bsr r32,r/m32

    //double shift
    [0xA4] = REG_MOD_RM | IMM_BYTE, //shld r/m16,r16,imm8; shld r/m32,r32,imm8
    [0xA5] = REG_MOD_RM, //shld r/m16,r16,cl; shld r/m32,r32,cl
    [0xAC] = REG_MOD_RM | IMM_BYTE, //shrd r/m16,r16,imm8; shrd r/m32,r32,imm8
    [0xAD] = REG_MOD_RM, //shrd r/m16,r16,cl; shrd r/m32,r32,cl

    //move with zero/sign extension
    [0xB6] = REG_MOD_RM, //movzx r16,r/m8; movzx r32,r/m8
    [0xB7] = REG_MOD_RM, //movzx r32,r/m16
    [0xBE] = REG_MOD_RM, //movsx r16,r/m8; movsx r32,r/m8
    [0xBF] = REG_MOD_RM, //movsx r32,r/m16

    //conditional move
    [0x47] = REG_MOD_RM, //cmova 
    [0x43] = REG_MOD_RM, //cmovae  = cmovnb 
    [0x42] = REG_MOD_RM, //cmovb  = cmovc  = cmovnae 
    [0x46] = REG_MOD_RM, //cmovbe  = cmovna 
    [0x44] = REG_MOD_RM, //cmove  = cmovz 
    [0x4F] = REG_MOD_RM, //cmovg  = cmovnle 
    [0x4D] = REG_MOD_RM, //cmovge  = cmovnl 
    [0x4C] = REG_MOD_RM, //cmovl  = cmovnge 
    [0x4E] = REG_MOD_RM, //cmovle  = cmovng 
    [0x45] = REG_MOD_RM, //cmovne  = cmovnz 
    [0x41] = REG_MOD_RM, //cmovno 
    [0x4B] = REG_MOD_RM, //cmovnp  = cmovpo 
    [0x49] = REG_MOD_RM, //cmovns 
    [0x40] = REG_MOD_RM, //cmovo 
    [0x4A] = REG_MOD_RM, //cmovp  = cmovpe 
    [0x48] = REG_MOD_RM, //cmovs

    //conditional set
    [0x97] = REG_MOD_RM | BYTE_OPERAND, //seta 
    [0x93] = REG_MOD_RM | BYTE_OPERAND, //setae  = setnb 
    [0x92] = REG_MOD_RM | BYTE_OPERAND, //setb  = setc  = setnae 
    [0x96] = REG_MOD_RM | BYTE_OPERAND, //setbe  = setna 
    [0x94] = REG_MOD_RM | BYTE_OPERAND, //sete  = setz 
    [0x9F] = REG_MOD_RM | BYTE_OPERAND, //setg  = setnle 
    [0x9D] = REG_MOD_RM | BYTE_OPERAND, //setge  = setnl 
    [0x9C] = REG_MOD_RM | BYTE_OPERAND, //setl  = setnge 
    [0x9E] = REG_MOD_RM | BYTE_OPERAND, //setle  = setng 
    [0x95] = REG_MOD_RM | BYTE_OPERAND, //setne  = setnz 
    [0x91] = REG_MOD_RM | BYTE_OPERAND, //setno 
    [0x9B] = REG_MOD_RM | BYTE_OPERAND, //setnp  = setpo 
    [0x99] = REG_MOD_RM | BYTE_OPERAND, //setns 
    [0x90] = REG_MOD_RM | BYTE_OPERAND, //seto 
    [0x9A] = REG_MOD_RM | BYTE_OPERAND, //setp  = setpe 
    [0x98] = REG_MOD_RM | BYTE_OPERAND, //sets

    [0xC8] = REG_IN_OPCODE, //bswap r32

    [0xB0] = REG_MOD_RM | BYTE_OPERAND, //cmpxchg r/m8,r8
    [0xB1] = REG_MOD_RM, //cmpxchg r/m16,r16; cmpxchg r/m32,r32
    [0xC7] = MOD_RM_ONLY | NO_DEREFERENCE, //cmpxchg8b m64

    [0xC0] = REG_MOD_RM | BYTE_OPERAND, //xadd r/m8,r8
    [0xC1] = REG_MOD_RM, //xadd r/m16,r16; xadd r/m32,r32
};

/**
 * @brief Get/set instruction operands
 * @param *state Emulator state. EIP must point to the instruction opcode
 * @param *params Instruction parameters and prefixes
 * @param *regOp Operand encoded in REG field in ModR/M or in the opcode
 * @param *modrmOp Register or memory operand encoded in ModR/M
 * @param *imm Immediate operand or memory offset
 * @param *coffs Code offset
 * @param set True to set operands, false to get operands
 * @return Next EIP value after the instruction or 0xFFFFFFFF on memory violation
 */
static uint32_t I686GetSetOperands(struct I686EmuState *state, const struct I686InstructionParams *params, uint32_t *regOp, uint32_t *modrmOp, uint32_t *imm, uint64_t *coffs, bool set);

/**
 * @brief Read 1, 2, 4, 6 or 8 bytes
 * @param *state Emulator state
 * @param address Address to read from
 * @param size Number of bytes to read
 * @param *data Destination buffer
 * @return False on success, true on failure
 */
static bool I686EmuReadMemory(const struct I686EmuState *state, uint32_t address, uint8_t size, void *data)
{
    if(((address + size) < address) //address wrapping around zero
        || ((address + size) >= I686_EMU_REAL_MODE_SPACE_SIZE)) //or crossing the real mode space boundary
        return true; //results in memory violation

    if(1 == size)
        *((uint8_t*)data) = state->code[address];
    else if(2 == size)
        *((uint16_t*)data) = (uint16_t)state->code[address] | ((uint16_t)state->code[address + 1] << 8);
    else if(4 == size)
        *((uint32_t*)data) = (uint32_t)state->code[address] | ((uint32_t)state->code[address + 1] << 8) 
            | ((uint32_t)state->code[address + 2] << 16) | ((uint32_t)state->code[address + 3] << 24);
    else if(6 == size)
        *((uint64_t*)data) = (uint64_t)state->code[address] | ((uint64_t)state->code[address + 1] << 8) 
            | ((uint64_t)state->code[address + 2] << 16) | ((uint64_t)state->code[address + 3] << 24)
            | ((uint64_t)state->code[address + 4] << 32) | ((uint64_t)state->code[address + 5] << 40);
    else if(8 == size)
        *((uint64_t*)data) = (uint64_t)state->code[address] | ((uint64_t)state->code[address + 1] << 8) 
            | ((uint64_t)state->code[address + 2] << 16) | ((uint64_t)state->code[address + 3] << 24)
            | ((uint64_t)state->code[address + 4] << 32) | ((uint64_t)state->code[address + 5] << 40)
            | ((uint64_t)state->code[address + 6] << 48) | ((uint64_t)state->code[address + 7] << 56);
    else
        return true;
    
    return false;
}

/**
 * @brief Write 1, 2, 4, 6 or 8 bytes
 * @param *state Emulator state
 * @param address Address to write to
 * @param size Number of bytes to write
 * @param *data Source buffer
 * @return False on success, true on failure
 */
static bool I686EmuWriteMemory(const struct I686EmuState *state, uint32_t address, uint8_t size, const void *data)
{
    if(((address + size) < address) //address wrapping around zero
        || ((address + size) >= I686_EMU_REAL_MODE_SPACE_SIZE)) //or crossing the real mode space boundary
        return true; //results in memory violation

    const uint8_t *d = data;
    if(1 == size)
        state->code[address] = d[0];
    else if(2 == size)
    {
        state->code[address] = d[0];
        state->code[address + 1] = d[1];
    }
    else if(4 == size)
    {
        state->code[address] = d[0];
        state->code[address + 1] = d[1];
        state->code[address + 2] = d[2];
        state->code[address + 3] = d[3];
    }
    else if(6 == size)
    {
        state->code[address] = d[0];
        state->code[address + 1] = d[1];
        state->code[address + 2] = d[2];
        state->code[address + 3] = d[3];
        state->code[address + 4] = d[4];
        state->code[address + 5] = d[5];
    }
    else if(8 == size)
    {
        state->code[address] = d[0];
        state->code[address + 1] = d[1];
        state->code[address + 2] = d[2];
        state->code[address + 3] = d[3];
        state->code[address + 4] = d[4];
        state->code[address + 5] = d[5];
        state->code[address + 6] = d[6];
        state->code[address + 7] = d[7];
    }
    else
        return true;
    
    return false;
}

/**
 * @brief Push value on the stack
 * @param *state Emulator state
 * @param *params Instruction params
 * @param pushSize Size of the push = stack pointer decrement value
 * @param valueSize Size of the element
 * @param *value Value to push
 * @return False on success, true on memory violation
 */
static bool I686EmuPush(struct I686EmuState *state, const struct I686InstructionParams *params, uint8_t pushSize, uint8_t valueSize, void *value)
{
    if(params->override.address)
        state->registers.esp -= pushSize;
    else
        state->registers.sp -= pushSize;
    return I686EmuWriteMemory(state, 
        EMU_FAR_POINTER_TO_LINEAR(state->registers.ss, params->override.address ? state->registers.esp : (uint16_t)state->registers.sp), valueSize, value);
}

/**
 * @brief Pop value from the stack
 * @param *state Emulator state 
 * @param *params Instruction params
 * @param pushSize Size of the pop = stack pointer increment value
 * @param valueSize Size of the element
 * @param *value Memory to store the popped value
 * @return False on success, true on memory violation
 */
static bool I686EmuPop(struct I686EmuState *state, const struct I686InstructionParams *params, uint8_t popSize, uint8_t valueSize, void *value)
{
    bool ret = I686EmuReadMemory(state, 
        EMU_FAR_POINTER_TO_LINEAR(state->registers.ss, params->override.address ? state->registers.esp : (uint16_t)state->registers.sp), valueSize, value);
    if(params->override.address)
        state->registers.esp += popSize;
    else
        state->registers.sp += popSize;
    return ret;
}

enum I686EmulatorState I686EmulatorRun(struct I686EmuState *state)
{
    struct I686InstructionParams params = {};
    bool expansion = false;
    bool skipStore = false;
    struct I686Registers *reg = &(state->registers);
    uint8_t *code = (uint8_t*)EMU_FAR_POINTER_TO_LINEAR(reg->cs, state->code);
    uint32_t originalIp = reg->eip;
    uint32_t next;

    repeat:
    reg->eip = originalIp;

    //decode params
    //there may be up to 4 parameters
    for(uint8_t i = 0; i < 4; i++)
    {
        switch(code[reg->eip])
        {
            case 0xF2: //repne/repnz
                params.repne = 1;
                break;
            case 0xF3: //rep or repe/repz
                params.rep = 1;
                break;
            //segment overrides
            case 0x2E:
                params.override.cs = 1;
                break;
            case 0x36:
                params.override.ss = 1;
                break;
            case 0x3E:
                params.override.ds = 1;
                break;
            case 0x26:
                params.override.es = 1;
                break;
            case 0x64:
                params.override.fs = 1;
                break;
            case 0x65:
                params.override.gs = 1;
                break;
            case 0x66: //operand size override
                params.override.operand = 1;
                break;
            case 0x67: //address size override
                params.override.address = 1;
                break;
            case 0xF0: //lock
                break;
            default:
                //non-params byte, end params analysis
                goto endparams;
        }
        ++reg->eip;
    }
    endparams:

    //check for opcode expansion
    if(0x0F == code[reg->eip])
    {
        expansion = true;
        ++reg->eip;
    }

    int32_t regOp = 0, modrmOp = 0, imm = 0;
    uint64_t coffs = 0;
    uint8_t opcode = code[reg->eip];
    uint8_t variant = 0; //instruction variant encoded in REG field
    uint8_t bits = 16; //operand size
    uint32_t seg = 0; //segment register value
    uint32_t fail = 0; //memory operation failure indicator
    union
    {
        uint64_t u64;
        int64_t i64;
        uint32_t u32;
        int32_t i32;
        uint16_t u16;
        int16_t i16;
        uint8_t u8;
        int8_t i8;
    } v1, v2; //additional values

    params.params = expansion ? I686ExtendedOpcodeParams[opcode] : I686OpcodeParams[opcode];

    if(params.params & MOD_RM_ONLY)
        variant = (code[reg->eip + 1] >> 3) & 0x7;
    
    if(params.params & BYTE_OPERAND)
        bits = 8;
    else if(params.override.operand)
        bits = 32;
    else
        bits = 16;

    
    if(params.override.cs)
        seg = reg->cs;
    else if(params.override.ss)
        seg = reg->ss;
    else if(params.override.es)
        seg = reg->es;
    else if(params.override.fs)
        seg = reg->fs;       
    else if(params.override.gs)
        seg = reg->gs;
    else if(params.override.ds)
        seg = reg->ds;
    else if(params.params & SEGMENT_CS)
        seg = reg->cs;
    else if(params.params & SEGMENT_SS)
        seg = reg->ss;
    else
        seg = reg->ds;

    //handle far jumps and calls with 0xFF opcode, which require non-standard operand size (segment + offset)
    if((0xFF == opcode) && ((0x3 == variant) || (0x5 == variant)))
        params.params |= NO_DEREFERENCE; //do not dereference memory pointer, the opcode decoder will do it

    next = I686GetSetOperands(state, &params, (uint32_t*)&regOp, (uint32_t*)&modrmOp, (uint32_t*)&imm, &coffs, false);   
    if(0xFFFFFFFF == next)
        return EMU_MEMORY_VIOLATION;

    int32_t *target = &regOp; //target operation operand pointer for instruction handling simplification

    if(!expansion)
    {
        //get opcode
        switch(opcode)
        {
            //add/adc/sub/sbb/cmp instructions
            case 0x00: //add r/m8,r8
            case 0x10: //adc r/m8,r8
            case 0x28: //sub r/m8,r8
            case 0x18: //sbb r/m8,r8
            case 0x38: //cmp r/m8,r8
            case 0x01: //add r/m16,r16, add r/m32,r32
            case 0x11: //adc r/m16,r16, adc r/m32,r32
            case 0x29: //sub r/m16,r16, sub r/m32,r32
            case 0x19: //sbb r/m16,r16, sbb r/m32,r32
            case 0x39: //cmp r/m16,r16, cmp r/m32,r32
                target = &modrmOp;
                [[fallthrough]];
            case 0x02: //add r8,r/m8
            case 0x12: //adc r8,r/m8
            case 0x2A: //sub r8,r/m8
            case 0x1A: //sbb r8,r/m8
            case 0x3A: //cmp r8,r/m8
            case 0x03: //add r16,r/m16, add r32,r/m32
            case 0x13: //adc r16,r/m16, add r32,r/m32
            case 0x2B: //sub r16,r/m16; sub r32,r/m32
            case 0x1B: //sbb r16,r/m16; sbb r32,r/m32
            case 0x3B: //cmp r16,r/m16; cmp r32,r/m32
                if(0x30 != (opcode & 0x30)) //non-cmp
                    *target = I686EmuAddSub(reg, modrmOp, regOp, bits, !(opcode & 0x8), (opcode & 0x10));
                else //cmp, just subtract without carry and do not store the result
                    I686EmuSub(reg, modrmOp, regOp, bits, false);
                break;
            case 0x04: //add al,imm8
            case 0x14: //adc al,imm8
            case 0x2C: //sub al,imm8
            case 0x1C: //sbb al,imm8
            case 0x3C: //cmp al,imm8
                if(0x30 != (opcode & 0x30)) //non-cmp
                    reg->al = I686EmuAddSub(reg, reg->al, imm, 8, !(opcode & 0x8), (opcode & 0x10));
                else //cmp
                    I686EmuSub(reg, reg->al, imm, 8, false);
                break;
            case 0x05: //add ax,imm16, add eax,imm32
            case 0x15: //adc ax,imm16, adc eax,imm32
            case 0x2D: //sub ax,imm16; sub eax,imm32
            case 0x1D: //sbb ax,imm16; sbb eax,imm32
                if(32 == bits)
                    reg->eax = I686EmuAddSub(reg, reg->eax, imm, 32, !(opcode & 0x8), (opcode & 0x10));
                else
                    reg->ax = I686EmuAddSub(reg, reg->ax, imm, 16, !(opcode & 0x8), (opcode & 0x10));
                break;
            case 0x3D: //cmp ax,imm16; cmp eax,imm32
                I686EmuSub(reg, (32 == bits) ? reg->eax : (uint16_t)reg->ax, imm, bits, false);
                break;
            case 0x83: //add/adc/and/sub/sbb/cmp/or/xor r/m16,imm8; add/adc/and/sub/sbb/cmp/or/xor r/m32, imm8 - sign extension
                imm = (int32_t)((int8_t)imm);
                [[fallthrough]];
            case 0x80: //add/adc/and/sub/sbb/cmp/or/xor r/m8,imm8
            case 0x81: //add/adc/and/sub/sbb/cmp/or/xor r/m16,imm16; add/adc/and/sub/sbb/cmp/or/xor r/m32, imm32
                switch(variant)
                {
                    case 0x0: //add
                    case 0x2: //adc
                        modrmOp = I686EmuAdd(reg, modrmOp, imm, bits, (0x2 == variant));
                        break;
                    case 0x5: //sub
                    case 0x3: //sbb
                        modrmOp = I686EmuSub(reg, modrmOp, imm, bits, (0x3 == variant));
                        break;
                    case 0x4: //and
                        modrmOp = I686EmuAnd(reg, modrmOp, imm, bits);
                        break;
                    case 0x7: //cmp
                        I686EmuSub(reg, modrmOp, imm, bits, false);
                        break;
                    case 0x1: //or
                        modrmOp = I686EmuOr(reg, modrmOp, imm, bits);
                        break;
                    case 0x6: //xor
                        modrmOp = I686EmuXor(reg, modrmOp, imm, bits);
                        break;
                }   
                break;

            //mov instructions
            case 0x88: //mov r/m8,r8
            case 0x89: //mov r/m16,r16; mov r/m32,r32
            case 0x8C: //mov r/m16,Sreg
                modrmOp = regOp;
                break;
            case 0x8A: //mov r8,r/m8
            case 0x8B: //mov r16,r/m16; mov r32,r/m32
            case 0x8E: //mov Sreg,r/m16
                regOp = modrmOp;
                break;
            case 0xB0 ... 0xBF: //mov r8,imm8, mov r16,imm16, mov r32,imm32
                regOp = imm;
                break;
            case 0xC6: //mov r/m8,imm8; mov r/m16,imm16; mov r/m32,imm32
            case 0xC7: 
                modrmOp = imm;
                break;
            case 0xA0: //mov al,moffs8
                reg->al = imm;
                break;
            case 0xA1: //mov ax,moffs16; mov eax,moffs32
                if(32 == bits)
                    reg->eax = imm;
                else
                    reg->ax = imm;
                break;
            case 0xA2: //mov moffs8,al
                imm = reg->al;
                break;
            case 0xA3: //mov moffs16,ax; mov moffs32,eax
                if(params.override.operand)
                    imm = reg->eax;
                else
                    imm = reg->ax;
                break;
            
            //ascii adjust...
            case 0x37: //aaa
                if(((reg->al & 0xF) > 9) || (reg->flags & FLAG_AF))
                {
                    reg->ax += 0x106;
                    reg->flags |= FLAG_AF | FLAG_CF;
                }
                else
                    reg->flags &= ~(FLAG_AF | FLAG_CF);
                reg->al &= 0xF;
                break;
            case 0xD5: //aad
                reg->al += (reg->ah * (int8_t)imm);
                reg->ah = 0;
                break;
            case 0xD4: //aam
                reg->ah = reg->al / (int8_t)imm;
                reg->al %= (int8_t)imm;
                break;
            case 0x3F: //aas
                if(((reg->al & 0xF) > 9) || (reg->flags & FLAG_AF))
                {
                    reg->ax -= 6;
                    reg->ah--;
                    reg->al &= 0xF;
                    reg->flags |= FLAG_AF | FLAG_CF;
                }
                else
                {
                    reg->flags &= ~(FLAG_AF | FLAG_CF);
                    reg->al &= 0xF;
                }
                break; 
            
            //logical operations
            case 0x20: //and r/m8,r8
            case 0x21: //and r/m16,r16, and r/m32,r32
            case 0x08: //or r/m8,r8
            case 0x09: //or r/m16,r16, or r/m32,r32
            case 0x30: //xor r/m8,r8
            case 0x31: //xor r/m16,r16; xor r/m32,r32
                target = &modrmOp;
                [[fallthrough]];
            case 0x22: //and r8,r/m8
            case 0x23: //and r16,r/m16, and r32,r/m32
            case 0x0A: //or r8,r/m8
            case 0x0B: //or r16,r/m16, or r32,r/m32
            case 0x32: //xor r8,r/m8
            case 0x33: //xor r16,r/m16; xor r32,r/m32
                switch(opcode & 0x38)
                {
                    case 0x8:
                        v1.u8 = EMU_OR;
                        break;
                    case 0x20:
                        v1.u8 = EMU_AND;
                        break;
                    case 0x30:
                        v1.u8 = EMU_XOR;
                        break;
                }
                *target = I686EmuBitwise(v1.u8, reg, modrmOp, regOp, bits);
                break;
            case 0x24: //and al,imm8
            case 0x0C: //or al,imm8
            case 0x34: //xor al,imm8
                switch(opcode & 0x38)
                {
                    case 0x8:
                        v1.u8 = EMU_OR;
                        break;
                    case 0x20:
                        v1.u8 = EMU_AND;
                        break;
                    case 0x30:
                        v1.u8 = EMU_XOR;
                        break;
                }
                reg->al = I686EmuBitwise(v1.u8, reg, reg->al, imm, 8);
                break;
            case 0x25: //and ax,imm16, and eax,imm32
            case 0x0D: //or ax,imm16, or eax,imm32
            case 0x35: //xor ax,imm16; xor eax,imm32
                switch(opcode & 0x38)
                {
                    case 0x8:
                        v1.u8 = EMU_OR;
                        break;
                    case 0x20:
                        v1.u8 = EMU_AND;
                        break;
                    case 0x30:
                        v1.u8 = EMU_XOR;
                        break;
                }
                if(32 == bits)
                    reg->eax = I686EmuBitwise(v1.u8, reg, reg->eax, imm, 32);
                else
                    reg->ax = I686EmuBitwise(v1.u8, reg, reg->ax, imm, 16);
                break;
            
            case 0xA8: //test al,imm8
                I686EmuAnd(reg, reg->al, imm, 8);
                break;
            case 0xA9: //test ax,imm16; test eax,imm32
                I686EmuAnd(reg, params.override.operand ? reg->eax : reg->ax, imm, params.override.operand ? 32 : 16);
                break;
            case 0x84: //test r/m8,r8
                I686EmuAnd(reg, regOp, modrmOp, 8);
                break;
            case 0x85: //test r/m16,r16; test r/m32,r32
                I686EmuAnd(reg, regOp, modrmOp, params.override.operand ? 32 : 16);
                break;

            case 0xD0: //rcl/rcr/rol/ror/sal/sar/shl/shr r/m8,1
            case 0xD1: //rcl/rcr/rol/ror/sal/sar/shl/shr r/m16,1; rcl/rcr/rol/ror/sal/sar/shl/shr r/m32,1
                modrmOp = (variant < 0x4) ? I686EmuRotate(reg, modrmOp, 1, (0xD0 == opcode) ? 8 : (params.override.operand ? 32 : 16), (variant & 0x1), (variant & 0x2))
                    : I686EmuShift(reg, modrmOp, 1, (0xD0 == opcode) ? 8 : (params.override.operand ? 32 : 16), (variant & 0x1), (0x7 == variant));
                break;
            case 0xD2: //rcl/rcr/rol/ror/sal/sar/shl/shr r/m8,cl
            case 0xD3: //rcl/rcr/rol/ror/sal/sar/shl/shr r/m16,cl; rcl/rcr/rol/ror/sal/sar/shl/shr r/m32,cl
                modrmOp = (variant < 0x4) ? I686EmuRotate(reg, modrmOp, reg->cl, (0xD2 == opcode) ? 8 : (params.override.operand ? 32 : 16), (variant & 0x1), (variant & 0x2))
                    : I686EmuShift(reg, modrmOp, reg->cl, (0xD0 == opcode) ? 8 : (params.override.operand ? 32 : 16), (variant & 0x1), (0x7 == variant));
                break;
            case 0xC0: //rcl/rcr/rol/ror/sal/sar/shl/shr r/m8,imm8
            case 0xC1: //rcl/rcr/rol/ror/sal/sar/shl/shr r/m16,imm8; rcl/rcr/rol/ror/sal/sar/shl/shr r/m32,imm8
                modrmOp = (variant < 0x4) ? I686EmuRotate(reg, modrmOp, imm, (0xC0 == opcode) ? 8 : (params.override.operand ? 32 : 16), (variant & 0x1), (variant & 0x2))
                    : I686EmuShift(reg, modrmOp, imm, (0xD0 == opcode) ? 8 : (params.override.operand ? 32 : 16), (variant & 0x1), (0x7 == variant));
                break;

            case 0x98: //cbw
                if(32 == bits)
                    reg->eax = (int32_t)reg->ax;
                else
                    reg->ax = (int16_t)reg->al;
                break;
            case 0xF8: //clc
                reg->flags &= ~FLAG_CF;
                break;
            case 0xFC: //cld
                reg->flags &= ~FLAG_DF;
                break;
            case 0xFA: //cli
                reg->flags &= ~FLAG_IF;
                break;
            case 0xF5: //cmc
                reg->flags ^= FLAG_CF;
                break;
            case 0xF9: //stc
                reg->flags |= FLAG_CF;
                break;
            case 0xFD: //std
                reg->flags |= FLAG_DF;
                break;
            case 0xFB:
                reg->flags |= FLAG_IF;
                break;

            case 0x99: //cwd; cdq
                if(params.override.operand)
                {
                    int64_t t = (int64_t)reg->eax;
                    reg->edx = ((uint64_t)t >> 32);
                    reg->eax = t & 0xFFFFFFFF;
                }
                else
                {
                    int32_t t = (int32_t)reg->ax;
                    reg->dx = ((uint32_t)t >> 16);
                    reg->ax = t & 0xFFFF;
                }
                break;

            //decimal adjust...
            case 0x27: //daa
                v1.u8 = reg->al;
                v2.u16 = reg->flags & FLAG_CF;
                if(((reg->al & 0xF) > 9) || (reg->flags & FLAG_AF))
                {
                    reg->al += 6;
                    reg->flags |= FLAG_AF;
                }
                else
                    reg->flags &= ~FLAG_AF;
                if((v1.u8 > 0x99) || v2.u16)
                {
                    reg->al += 0x60;
                    reg->flags |= FLAG_CF;
                }
                else
                    reg->flags &= ~FLAG_CF;
                break;
            case 0x2F: //das
                v1.u8 = reg->al;
                v2.u16 = reg->flags & FLAG_CF;
                if(((reg->al & 0xF) > 9) || (reg->flags & FLAG_AF))
                {
                    reg->al -= 6;
                    reg->flags |= FLAG_AF;
                    if((uint8_t)reg->al > v1.u8)
                        reg->flags |= FLAG_CF;
                }
                else
                    reg->flags &= ~FLAG_AF;
                if((v1.u8 > 0x99) || v2.u16)
                {
                    reg->al -= 0x60;
                    reg->flags |= FLAG_CF;
                }
                break;
            
            //incrementation, decrementation, also jumps and calls
            case 0xFF: //inc r/m16; inc r/m32; dec r/m16; dec r/m32; 
                //also absolute near indirect jumps: jmp r/m16; jmp r/m32; absolute far indirect jumps: jmp m16:16; jmp m16:32
                //also absolute near indirect calls: call r/m16; call r/m32; absolute far indirect calls: call m16:16; call m16:32
                //also push r/m16; push r/m32
                if((0xFF == opcode) && (0x6 == variant)) //handle push
                {
                    params.override.operand ? I686EmuPush(state, &params, 4, 4, &modrmOp) : I686EmuPush(state, &params, 2, 2, &modrmOp);
                    break;
                }
                else if((0xFF == opcode) && ((0x2 == variant) || (0x3 == variant) || (0x4 == variant) || (0x5 == variant))) //handle jumps and calls
                {
                    if(0x3 == variant) //far call, store segment
                        fail |= I686EmuPush(state, &params, params.override.operand ? 4 : 2, 2, &reg->cs);
                    if((0x2 == variant) || (0x3 == variant)) //calls, store return address
                    {
                        v1.u32 = reg->eip + 2;
                        fail |= I686EmuPush(state, &params, params.override.operand ? 4 : 2, params.override.operand ? 4 : 2, &v1);
                    }
                    if((0x4 == variant) || (0x2 == variant)) //near absolute indirect jump or call
                    {
                        reg->eip = (uint32_t)modrmOp;
                        if(!params.override.operand)
                            reg->eip &= 0xFFFF;
                    }
                    else //far absolute jump or call
                    {
                        uint64_t t = 0;
                        fail |= I686EmuReadMemory(state, (uint32_t)modrmOp, params.override.operand ? 6 : 4, &t);
                        if(params.override.operand)
                        {
                            reg->cs = t >> 32;
                            reg->eip = t & 0xFFFFFFFF;
                        }
                        else
                        {
                            reg->cs = t >> 16;
                            reg->eip = t & 0xFFFF;
                        }
                    }
                    skipStore = true;
                    break;
                }
                [[fallthrough]];
            case 0xFE: //inc r/m8; dec r/m8
                target = &modrmOp;
                [[fallthrough]];
            case 0x40 ... 0x47: //inc r16; inc r32
            case 0x48 ... 0x4F: //dec r16; dec r32
                v1.u16 = reg->flags & FLAG_CF;
                if((((0xFE == opcode) || (0xFF == opcode)) && (0x0 == variant))
                    || (opcode <= 0x47))
                    *target = I686EmuAdd(reg, *target, 1, (0xFE == opcode) ? 8 : (params.override.operand ? 32 : 16), false);
                else
                    *target = I686EmuSub(reg, *target, 1, (0xFE == opcode) ? 8 : (params.override.operand ? 32 : 16), false);
                reg->flags &= ~FLAG_CF;
                reg->flags |= v1.u16;
                break;
            
            //multiplication, division, negation, test
            case 0xF6: //(i)div r/m8, (i)mul r/m8; neg r/m8; not r/m8; test r/m8,imm8
                if(0x3 == variant) //neg
                {
                    modrmOp = (int8_t)I686EmuSub(reg, 0, (int8_t)modrmOp, 8, false);
                    break;
                }
                else if(0x2 == variant) //not
                {
                    modrmOp = (uint8_t)(~((uint8_t)modrmOp));
                    break;
                }
                else if(0x0 == variant) //test
                {
                    ++next;
                    I686EmuAnd(reg, modrmOp, code[next - 1], 8);
                    break;
                }
                switch(variant)
                {
                    case 0x4: //mul
                    case 0x5: //imul
                        I686EmuMul(reg, reg->al, modrmOp, (uint32_t*)&v2, (uint32_t*)&v1, 8, (0x5 == variant));
                        break;
                    case 0x6: //div
                        v1.u8 = (uint8_t)reg->ax / (uint8_t)modrmOp;
                        v2.u8 = (uint8_t)reg->ax % (uint8_t)modrmOp;
                        break;
                    case 0x7: //idiv
                        v1.i8 = reg->ax / (int8_t)modrmOp;
                        v2.i8 = reg->ax % (int8_t)modrmOp;
                        break;
                }
                reg->al = v1.i8;
                reg->ah = v2.i8;
                break;
            case 0xF7: //(i)div r/m16; (i)div r/m32; (i)mul r/m16; (i)mul r/m32; neg r/m16; neg r/m32; not r/m16; not r/m32; test r/m16,imm16; test r/m32,imm32
                if(params.override.operand) //32-bit
                {
                    if(0x3 == variant) //neg
                    {
                        if(params.override.operand)
                            modrmOp = (int32_t)I686EmuSub(reg, 0, (int32_t)modrmOp, 32, false);
                        else
                            modrmOp = (int16_t)I686EmuSub(reg, 0, (int16_t)modrmOp, 16, false);
                        break;
                    }
                    else if(0x2 == variant) //not
                    {
                        modrmOp = (uint32_t)(~((uint32_t)modrmOp));
                        break;
                    }
                    else if(0x0 == variant) //test
                    {
                        //test instruction differs from others, because it additionally has an immediate, which additionally complicates decoding, 
                        //as if x86 was not already overcomplicated
                        if(params.override.operand)
                        {
                            next += 4;
                            uint32_t t = (uint32_t)code[next - 4] | ((uint32_t)code[next - 3] << 8) | ((uint32_t)code[next - 2] << 16) | ((uint32_t)code[next - 1] << 24);
                            I686EmuAnd(reg, modrmOp, t, 32);
                        }
                        else
                        {
                            next += 2;
                            uint16_t t = (uint16_t)code[next - 2] | ((uint16_t)code[next - 1] << 8);
                            I686EmuAnd(reg, modrmOp, t, 16);
                        }
                        skipStore = true;
                        break;
                    }
                    int64_t t = 0;
                    switch(variant)
                    {
                        case 0x4: //mul
                        case 0x5: //imul
                            I686EmuMul(reg, reg->eax, modrmOp, (uint32_t*)&reg->edx, (uint32_t*)&reg->eax, 32, (0x5 == variant));
                            break;
                        case 0x6: //div
                            t = (uint64_t)reg->eax | ((uint64_t)reg->edx << 32);
                            reg->eax = (uint64_t)t / (uint64_t)modrmOp;
                            reg->edx = (uint64_t)t % (uint64_t)modrmOp;
                            break;
                        case 0x7: //idiv
                            t = (uint64_t)reg->eax | ((uint64_t)reg->edx << 32);
                            reg->eax = t / (int64_t)modrmOp;
                            reg->edx = t % (int64_t)modrmOp;
                            break;
                    }
                }
                else //16-bit
                {
                    if(0x3 == variant) //neg
                    {
                        modrmOp = -((int16_t)modrmOp);
                        if(0 == modrmOp)
                            reg->flags &= ~FLAG_CF;
                        else
                            reg->flags |= FLAG_CF;
                        break;
                    }
                    else if(0x02 == variant) //not
                    {
                        modrmOp = (uint16_t)(~((uint16_t)modrmOp));
                        break;
                    }
                    int32_t t = 0;
                    switch(variant)
                    {
                        case 0x4: //mul
                        case 0x5: //imul
                            I686EmuMul(reg, (uint16_t)reg->ax, (uint16_t)modrmOp, (uint32_t*)&v2, (uint32_t*)&v1, 16, (0x5 == variant));
                            reg->dx = v2.i16;
                            reg->ax = v1.i16;
                            break;
                        case 0x6: //div
                            t = (uint32_t)reg->ax | ((uint32_t)reg->dx << 16);
                            reg->ax = (uint32_t)t / (uint32_t)modrmOp;
                            reg->dx = (uint32_t)t % (uint32_t)modrmOp;
                            break;
                        case 0x7: //idiv
                            t = (uint32_t)reg->ax | ((uint32_t)reg->dx << 16);
                            reg->eax = t / (int32_t)modrmOp;
                            reg->edx = t % (int32_t)modrmOp;
                            break;
                    }
                }
                break;
                
                case 0x6B: //imul r16,r/m16,imm8; imul r32,r/m32,imm8 - sign extension
                    imm = (int32_t)((int8_t)(imm)); 
                    [[fallthrough]];
                case 0x69: //imul r16,r/m16,imm16; imul r32,r/m32,imm32 
                    I686EmuMul(reg, (uint32_t)modrmOp, (uint32_t)imm, NULL, (uint32_t*)&regOp, params.override.operand ? 32 : 16, true);
                    params.noModRmWrite = 1;
                    break;

                //I/O ports
                case 0xEC: //in al,dx
                    imm = reg->dx;
                    [[fallthrough]];
                case 0xE4: //in al,imm8
                    reg->al = IoPortReadByte(imm);
                    break;
                case 0xED: //in ax,dx; in eax,dx
                    imm = reg->dx;
                    [[fallthrough]];
                case 0xE5: //in ax,imm8; in eax,imm8
                    if(params.override.operand)
                        reg->eax = IoPortReadDWord(imm);
                    else
                        reg->ax = IoPortReadWord(imm);
                    break;
                case 0x6C: //insb (ins m8,dx)
                    v1.u8 = IoPortReadByte(reg->dx);
                    fail |= I686EmuWriteMemory(state, EMU_FAR_POINTER_TO_LINEAR(reg->es, (params.override.address ? reg->edi : reg->di)), 1, &v1);
                    break;
                case 0x6D: //insw; insd (ins m16/m32, dx)
                    v1.u32 = params.override.operand ? IoPortReadDWord(reg->dx) : IoPortReadWord(reg->dx);
                    fail |= I686EmuWriteMemory(state, EMU_FAR_POINTER_TO_LINEAR(reg->es, (params.override.address ? reg->edi : reg->di)), params.override.operand ? 4 : 2, &v1);
                    break;

                case 0xEE: //out dx,al
                    imm = reg->dx;
                    [[fallthrough]];
                case 0xE6: //out imm8,al
                    IoPortWriteByte(imm, reg->al);
                    break;
                case 0xEF: //out dx,ax; out dx,eax
                    imm = reg->dx;
                    [[fallthrough]];
                case 0xE7: //out imm8,ax; out imm8,eax
                    if(params.override.operand)
                        IoPortWriteDWord(imm, reg->eax);
                    else
                        IoPortWriteWord(imm, reg->ax);
                    break;
                case 0x6E: //outsb (outs dx,m8)
                    v1.u64 = 0;
                    fail |= I686EmuReadMemory(state, seg + (params.override.address ? reg->esi : reg->si), 1, &v1);
                    if(!fail)
                        IoPortWriteByte(reg->dx, v1.u8);
                    break;
                case 0x6F: //outsw; outsd (outs dx, m16/m32)
                    v1.u64 = 0;
                    fail |= I686EmuReadMemory(state, seg + (params.override.address ? reg->esi : reg->si), params.override.operand ? 4 : 2, &v1);
                    if(!fail)
                        params.override.operand ? IoPortWriteDWord(reg->dx, v1.u32) : IoPortWriteWord(reg->dx, v1.u16);
                    break;
                
                //interrupts
                case 0xCE: //into
                    if(!(reg->flags & FLAG_OF))
                        break;
                    [[fallthrough]];
                case 0xCC: //int3
                case 0xF1: //int1
                case 0xCD: //int imm8
                    if(0xCE == opcode)
                        imm = 4;
                    else if(0xCC == opcode)
                        imm = 3;
                    else if(0xF1 == opcode)
                        imm = 1;
                    fail |= I686EmuPush(state, &params, 2, 2, &reg->flags);
                    reg->flags &= ~(FLAG_IF | FLAG_TF);
                    fail |= I686EmuPush(state, &params, 2, 2, &reg->cs);
                    fail |= I686EmuPush(state, &params, 2, 2, &reg->ip);
                    reg->cs = state->ivt[imm] >> 16;
                    reg->eip = state->ivt[imm] & 0xFFFF;
                    skipStore = true;
                    break;

                //returns
                case 0xCF: //iret, iretd
                    if(EMU_FAR_POINTER_TO_LINEAR(reg->ss, reg->esp) == I686_EMULATOR_STACK_TOP)
                        return EMU_OK;
                    reg->eip = 0;
                    fail |= params.override.operand ? I686EmuPop(state, &params, 4, 4, &reg->eip) : I686EmuPop(state, &params, 2, 2, &reg->ip);
                    fail |= I686EmuPop(state, &params, params.override.operand ? 4 : 2, 2, &reg->cs);
                    if(params.override.operand)
                    {
                        fail |= I686EmuPop(state, &params, 4, 4, &v1);
                        reg->eflags = (reg->eflags & 0x1A0000) | (v1.u32 & 0x257FD5);
                    }
                    else
                        fail |= I686EmuPop(state, &params, 2, 2, &reg->flags);
                    skipStore = true;
                    break;
                
                case 0xC3: //ret near
                case 0xC2: //ret imm16 (near)
                    if(EMU_FAR_POINTER_TO_LINEAR(reg->ss, reg->esp) == I686_EMULATOR_STACK_TOP)
                        return EMU_OK;
                    reg->eip = 0;
                    fail |= params.override.operand ? I686EmuPop(state, &params, 4, 4, &reg->eip) : I686EmuPop(state, &params, 2, 2, &reg->ip);
                    if(0xC2 == opcode)
                    {
                        imm = (int16_t)imm;
                        params.override.address ? (reg->esp += imm) : (reg->sp += imm);
                    }
                    skipStore = true;
                    break;
                case 0xCB: //ret far
                case 0xCA: //ret imm16 (far)
                    if(EMU_FAR_POINTER_TO_LINEAR(reg->ss, reg->esp) == I686_EMULATOR_STACK_TOP)
                        return EMU_OK;
                    reg->eip = 0;
                    fail |= params.override.operand ? I686EmuPop(state, &params, 4, 4, &reg->eip) : I686EmuPop(state, &params, 2, 2, &reg->ip);
                    fail |= I686EmuPop(state, &params, params.override.operand ? 4 : 2, 2, &reg->cs);
                    if(0xCA == opcode)
                    {
                        imm = (int16_t)imm;
                        params.override.address ? (reg->esp += imm) : (reg->sp += imm);
                    }
                    skipStore = true;
                    break;
                
                //jumps, calls
                case 0xEB: //jmp rel8
                    reg->eip += 2 + (int8_t)coffs;
                    if(!params.override.operand)
                        reg->eip &= 0xFFFF;
                    skipStore = true;
                    break;
                case 0xE8: //call rel16; call rel32
                    if(params.override.operand)
                    {
                        v1.u32 = reg->eip + 5;
                        fail |= I686EmuPush(state, &params, 4, 4, &v1);
                    }
                    else
                    {
                        v1.u16 = reg->ip + 3;
                        fail |= I686EmuPush(state, &params, 2, 2, &v1);
                    }
                    [[fallthrough]];
                case 0xE9: //jmp rel16; jmp rel32
                    if(params.override.operand)
                        reg->eip += 5 + (int32_t)coffs;
                    else
                    {
                        reg->eip += 3 + (int16_t)coffs;
                        reg->eip &= 0xFFFF;
                    }
                    skipStore = true;
                    break;
                case 0x9A: //call ptr16:16; call ptr16:32
                    fail |= I686EmuPush(state, &params, params.override.operand ? 4 : 2, 2, &reg->cs);
                    if(params.override.operand)
                    {
                        v1.u32 = reg->eip + 7;
                        fail |= I686EmuPush(state, &params, 4, 4, &v1);
                    }
                    else
                    {
                        v1.u16 = reg->ip + 5;
                        fail |= I686EmuPush(state, &params, 2, 2, &v1);
                    }
                    [[fallthrough]];
                case 0xEA: //jmp ptr16:16; jmp ptr16:32
                    if(params.override.operand)
                    {
                        reg->cs = coffs >> 32;
                        reg->eip = coffs & 0xFFFFFFFF;
                    }
                    else
                    {
                        reg->cs = coffs >> 16;
                        reg->eip = coffs & 0xFFFF;
                    }
                    skipStore = true;
                    break;
                //conditional jumps
                case 0xE3: //jcxz rel8; jecxz rel8
                    if((params.override.operand && (0 == reg->ecx))
                    || (!params.override.operand && (0 == reg->cx)))
                        reg->eip += 2 + (int8_t)coffs;
                    else
                        reg->eip += 2;
                    skipStore = true;
                    break;
                case 0x77:
                case 0x73:
                case 0x72:
                case 0x76:
                case 0x74:
                case 0x7F:
                case 0x7D:
                case 0x7C:
                case 0x7E:
                case 0x75:
                case 0x71:
                case 0x7B:
                case 0x79:
                case 0x70:
                case 0x7A:
                case 0x78:
                    reg->eip = I686EmuJump(reg, &params, opcode, false, coffs);
                    skipStore = true;
                    break;
                
                //load far pointer
                case 0xC5: //lds r16,m16:16; lds r32,m16:32
                case 0xC4: //les r16,m16:16; les r32,m16:32
                    v1.u64 = 0;
                    fail |= I686EmuReadMemory(state, (uint32_t)modrmOp, params.override.operand ? 6 : 4, &v1);
                    if(0xC5 == opcode)
                        reg->ds = v1.u64 >> (params.override.operand ? 32 : 16);
                    else
                        reg->es = v1.u64 >> (params.override.operand ? 32 : 16);
                    regOp = v1.u64 & (params.override.operand ? 0xFFFFFFFF : 0xFFFF);
                    params.noModRmWrite = 1;
                    break;
                
                //load effective address
                case 0x8D: //lea r16,m; lea r32,m
                    regOp = modrmOp;
                    params.noModRmWrite = 1;
                    break;
                
                //strings

                //cmpsb, cmpsw, cmpsd
                case 0xA6: //cmpsb
                case 0xA7:  //cmpsw, cmpsd
                    seg *= 16;
                    v1.u32 = 0;
                    v2.u32 = 0;
                    
                    //segment might be overriden only for the 1st operand (ds:esi)
                    fail |= I686EmuReadMemory(state, seg + params.override.address ? reg->esi : reg->si,
                        (0xA6 == opcode) ? 1 : (params.override.operand ? 4 : 2), &v1);
                    fail |= I686EmuReadMemory(state, EMU_FAR_POINTER_TO_LINEAR(reg->es, params.override.address ? reg->edi : reg->di),
                        (0xA6 == opcode) ? 1 : (params.override.operand ? 4 : 2), &v2);
                    
                    I686EmuSub(reg, v1.u32, v2.u32, (0xA6 == opcode) ? 8 : (params.override.operand ? 32 : 16), false);
                    
                    if(params.override.address)
                    {
                        int8_t shift = (0xA6 == opcode) ? 1 : (params.override.operand ? 4 : 2);
                        if(reg->flags & FLAG_DF)
                            shift = -shift;
                        reg->esi += shift;
                        reg->edi += shift;
                    }
                    else
                    {
                        int8_t shift = (0xA6 == opcode) ? 1 : (params.override.operand ? 4 : 2);
                        if(reg->flags & FLAG_DF)
                            shift = -shift;
                        reg->si += shift;
                        reg->di += shift;
                    }
                    break;

                case 0xAC: //lodsb
                case 0xAD: //lodsw; lodsd
                    seg *= 16;
                    if(0xAC == opcode)
                        fail |= I686EmuReadMemory(state, seg + (params.override.address ? reg->esi : reg->si), 1, &reg->al);
                    else if(params.override.operand)
                        fail |= I686EmuReadMemory(state, seg + (params.override.address ? reg->esi : reg->si), 4, &reg->eax);
                    else
                        fail |= I686EmuReadMemory(state, seg + (params.override.address ? reg->esi : reg->si), 2, &reg->ax);
                    
                    if(params.override.address)
                    {
                        int8_t shift = (0xAC == opcode) ? 1 : (params.override.operand ? 4 : 2);
                        reg->esi += (reg->flags & FLAG_DF) ? -shift : shift;
                    }
                    else
                    {
                        int8_t shift = (0xAC == opcode) ? 1 : (params.override.operand ? 4 : 2);
                        reg->si += (reg->flags & FLAG_DF) ? -shift : shift;
                    }
                    break;

                case 0xA4: //movsb
                case 0xA5: //movsw; movsd
                    seg *= 16;
                    v1.u32 = 0;
                    
                    //segment might be overriden only for the 1st operand (ds:esi)
                    fail |= I686EmuReadMemory(state, seg + (params.override.address ? reg->esi : reg->si),
                        (0xA4 == opcode) ? 1 : (params.override.operand ? 4 : 2), &v1);
                    
                    fail |= I686EmuWriteMemory(state, EMU_FAR_POINTER_TO_LINEAR(reg->es, params.override.address ? reg->edi : reg->di),
                        (0xA4 == opcode) ? 1 : (params.override.operand ? 4 : 2), &v1);

                    if(params.override.address)
                    {
                        int8_t shift = (0xA4 == opcode) ? 1 : (params.override.operand ? 4 : 2);
                        if(reg->flags & FLAG_DF)
                            shift = -shift;
                        reg->esi += shift;
                        reg->edi += shift;
                    }
                    else
                    {
                        int8_t shift = (0xA4 == opcode) ? 1 : (params.override.operand ? 4 : 2);
                        if(reg->flags & FLAG_DF)
                            shift = -shift;
                        reg->si += shift;
                        reg->di += shift;
                    }
                    break;
                
                case 0xAE: //scasb
                case 0xAF: //scasw; scasd
                    v1.u32 = 0;
                    fail |= I686EmuReadMemory(state, EMU_FAR_POINTER_TO_LINEAR(reg->es, params.override.address ? reg->edi : reg->di), (0xAE == opcode) ? 1 : (params.override.operand ? 4 : 2), &v1);
                    
                    I686EmuSub(reg, (0xAE == opcode) ? reg->al : (params.override.operand ? reg->eax : reg->ax), v1.u8,
                        (0xAE == opcode) ? 8 : (params.override.operand ? 32 : 16), false);
                    
                    if(params.override.address)
                    {
                        int8_t shift = (0xAE == opcode) ? 1 : (params.override.operand ? 4 : 2);
                        reg->edi += (reg->flags & FLAG_DF) ? -shift : shift;
                    }
                    else
                    {
                        int8_t shift = (0xAE == opcode) ? 1 : (params.override.operand ? 4 : 2);
                        reg->di += (reg->flags & FLAG_DF) ? -shift : shift;
                    }
                    break;

                case 0xAA: //stsob
                case 0xAB: //stosw; stosd
                    if(0xAA == opcode)
                        fail |= I686EmuWriteMemory(state, EMU_FAR_POINTER_TO_LINEAR(reg->es, params.override.address ? reg->edi : reg->di), 1, &reg->al);
                    else if(params.override.operand)
                        fail |= I686EmuWriteMemory(state, EMU_FAR_POINTER_TO_LINEAR(reg->es, params.override.address ? reg->edi : reg->di), 4, &reg->eax);
                    else
                        fail |= I686EmuWriteMemory(state, EMU_FAR_POINTER_TO_LINEAR(reg->es, params.override.address ? reg->edi : reg->di), 2, &reg->ax);
                    
                    if(params.override.address)
                    {
                        int8_t shift = (0xAA == opcode) ? 1 : (params.override.operand ? 4 : 2);
                        reg->edi += (reg->flags & FLAG_DF) ? -shift : shift;
                    }
                    else
                    {
                        int8_t shift = (0xAA == opcode) ? 1 : (params.override.operand ? 4 : 2);
                        reg->di += (reg->flags & FLAG_DF) ? -shift : shift;
                    }
                    break;
                
                //exchange
                case 0x90 ... 0x97: //xchg ax,r16; xchg eax,r32
                    if(params.override.operand)
                    {
                        uint32_t t = reg->eax;
                        reg->eax = regOp;
                        regOp = t;
                    }
                    else
                    {
                        uint16_t t = reg->ax;
                        reg->ax = regOp;
                        regOp = t;
                    }
                    break;
                case 0x86: //xchg r/m8,r8
                case 0x87: //xchg r/m16,r16; xchg r/m32,r32
                    v1.u32 = regOp;
                    regOp = modrmOp;
                    modrmOp = v1.u32;
                    break;
                
                //stack
                case 0x8F: //pop r/m16; pop r/m32
                    target = &modrmOp;
                    [[fallthrough]];
                case 0x58 ... 0x5F: //pop r16; pop r32
                    fail |= I686EmuPop(state, &params, params.override.operand ? 4 : 2, params.override.operand ? 4 : 2, target);
                    break;
                case 0x1F: //pop ds
                    fail |= I686EmuPop(state, &params, 2, 2, &reg->ds);
                    break;
                case 0x07: //pop es
                    fail |= I686EmuPop(state, &params, 2, 2, &reg->es);
                    break;    
                case 0x17: //pop ss
                    fail |= I686EmuPop(state, &params, 2, 2, &reg->ss);
                    break;   
                case 0x9D: //popf; popfd
                    fail |= params.override.operand ? I686EmuPop(state, &params, 4, 4, &reg->eflags) : I686EmuPop(state, &params, 2, 2, &reg->flags);
                    break;
                case 0x50 ... 0x57: //push r16; push r32
                    fail |= params.override.operand ? I686EmuPush(state, &params, 4, 4, &regOp) : I686EmuPush(state, &params, 2, 2, &regOp);
                    break;
                case 0x6A: //push imm8
                    fail |= I686EmuPush(state, &params, params.override.operand ? 4 : 2, 1, &imm);
                    break;
                case 0x68: //push imm16; push imm32
                    fail |= params.override.operand ? I686EmuPush(state, &params, 4, 4, &imm) : I686EmuPush(state, &params, 2, 2, &imm);
                    break;
                case 0x0E: //push cs
                    fail |= I686EmuPush(state, &params, 2, 2, &reg->cs);
                    break;
                case 0x16: //push ss
                    fail |= I686EmuPush(state, &params, 2, 2, &reg->ss);
                    break;
                case 0x1E: //push ds
                    fail |= I686EmuPush(state, &params, 2, 2, &reg->ds);
                    break;
                case 0x06: //push es
                    fail |= I686EmuPush(state, &params, 2, 2, &reg->es);
                    break;
                case 0x9C: //pushf; pushfd
                    fail |= params.override.operand ? I686EmuPush(state, &params, 4, 4, &reg->eflags) : I686EmuPush(state, &params, 2, 2, &reg->flags);
                    break;
                case 0x61: //popa; popad
                    fail |= params.override.operand ? I686EmuPop(state, &params, 4, 4, &reg->edi) : I686EmuPop(state, &params, 2, 2, &reg->di);
                    fail |= params.override.operand ? I686EmuPop(state, &params, 4, 4, &reg->esi) : I686EmuPop(state, &params, 2, 2, &reg->si);
                    fail |= params.override.operand ? I686EmuPop(state, &params, 4, 4, &reg->ebp) : I686EmuPop(state, &params, 2, 2, &reg->bp);
                    reg->esp += (params.override.operand ? 4 : 2);
                    fail |= params.override.operand ? I686EmuPop(state, &params, 4, 4, &reg->ebx) : I686EmuPop(state, &params, 2, 2, &reg->bx);
                    fail |= params.override.operand ? I686EmuPop(state, &params, 4, 4, &reg->edx) : I686EmuPop(state, &params, 2, 2, &reg->dx);
                    fail |= params.override.operand ? I686EmuPop(state, &params, 4, 4, &reg->ecx) : I686EmuPop(state, &params, 2, 2, &reg->cx);
                    fail |= params.override.operand ? I686EmuPop(state, &params, 4, 4, &reg->eax) : I686EmuPop(state, &params, 2, 2, &reg->ax);
                    break;
                case 0x60: //pusha; pushad
                    v1.u32 = params.override.operand ? reg->esp : reg->sp;
                    fail |= params.override.operand ? I686EmuPush(state, &params, 4, 4, &reg->eax) : I686EmuPush(state, &params, 2, 2, &reg->ax);
                    fail |= params.override.operand ? I686EmuPush(state, &params, 4, 4, &reg->ecx) : I686EmuPush(state, &params, 2, 2, &reg->cx);
                    fail |= params.override.operand ? I686EmuPush(state, &params, 4, 4, &reg->edx) : I686EmuPush(state, &params, 2, 2, &reg->dx);
                    fail |= params.override.operand ? I686EmuPush(state, &params, 4, 4, &reg->ebx) : I686EmuPush(state, &params, 2, 2, &reg->bx);
                    fail |= params.override.operand ? I686EmuPush(state, &params, 4, 4, &v1) : I686EmuPush(state, &params, 2, 2, &v1);
                    fail |= params.override.operand ? I686EmuPush(state, &params, 4, 4, &reg->ebp) : I686EmuPush(state, &params, 2, 2, &reg->bp);
                    fail |= params.override.operand ? I686EmuPush(state, &params, 4, 4, &reg->esi) : I686EmuPush(state, &params, 2, 2, &reg->si);
                    fail |= params.override.operand ? I686EmuPush(state, &params, 4, 4, &reg->edi) : I686EmuPush(state, &params, 2, 2, &reg->di);
                    break;

                //function handling
                case 0xC8: //enter imm16,imm8
                    v2.u8 = code[reg->eip + 3]; //nesting level

                    fail |= params.override.operand ? I686EmuPush(state, &params, 4, 4, &reg->ebp) : I686EmuPush(state, &params, 2, 2, &reg->bp);
                    v1.u32 = params.override.operand ? (uint32_t)reg->esp : (uint32_t)reg->sp;

                    if(v2.u8 >= 1) //nesting level >= 1
                    {
                        if(v2.u8 > 1) //nesting level > 1
                        {
                            for(uint8_t i = 0; i < (v2.u8 - 1); i++)
                            {
                                if(params.override.address)
                                {
                                    reg->ebp -= (params.override.operand ? 4 : 2);
                                    fail |= I686EmuPush(state, &params, (params.override.operand ? 4 : 2), 4, &reg->ebp);
                                }
                                else
                                {
                                    reg->bp -= (params.override.operand ? 4 : 2);
                                    fail |= I686EmuPush(state, &params, (params.override.operand ? 4 : 2), 2, &reg->bp);
                                }
                            }
                        }
                        fail |= params.override.operand ? I686EmuPush(state, &params, 4, 4, &v1) : I686EmuPush(state, &params, 2, 2, &v1);
                    }

                    v1.u16 = (uint16_t)code[reg->eip + 1] | ((uint16_t)code[reg->eip + 2] << 8); //alloc size
                    if(params.override.operand)
                    {
                        reg->ebp = v2.u32;
                        reg->esp -= v1.u16;
                    }
                    else
                    {
                        reg->bp = v2.u32 & 0xFFFF;
                        reg->sp -= v1.u16;
                    }
                    reg->eip += 4;
                    skipStore = true;
                    break;
                
                case 0xC9: //leave
                    if(params.override.address)
                        reg->esp = reg->ebp;
                    else
                        reg->sp = reg->bp;

                    fail |= params.override.operand ? I686EmuPop(state, &params, 4, 4, &reg->ebp) : I686EmuPop(state, &params, 2, 2, &reg->bp);
                    break;
                
                //other
                case 0xF4: //hlt
                    break; //TODO: what to do with it?
                
                case 0x9F: //lahf
                    reg->ah = (reg->flags & (FLAG_CF | FLAG_PF | FLAG_AF | FLAG_ZF | FLAG_SF)) | 0b10;
                    break;
                case 0x9E: //sahf
                    reg->flags = (reg->ah & (FLAG_CF | FLAG_PF | FLAG_AF | FLAG_ZF | FLAG_SF)) | 0b10;
                    break;
                
                case 0xD7: //xlatb
                    fail |= I686EmuReadMemory(state, EMU_FAR_POINTER_TO_LINEAR(seg, params.override.address ? reg->ebx : reg->bx) + (uint8_t)reg->al, 1, &reg->al);
                    break;
                
                case 0x62: //bound r16,m16&14; bound r32,m32&32
                    params.noModRmWrite = 1;
                    v1.u64 = 0;
                    fail |= I686EmuReadMemory(state, modrmOp, params.override.operand ? 8 : 4, &v1);
                    if(params.override.operand
                        ? (((int32_t)regOp < (int32_t)(v1.u32)) || ((int32_t)regOp > (int32_t)(v1.u64 >> 32)))
                        : (((int16_t)regOp < (int16_t)(v1.u16)) || ((int16_t)regOp > (int16_t)(v1.u32 >> 16))))
                    {
                        fail |= I686EmuPush(state, &params, 2, 2, &reg->flags);
                        reg->flags &= ~(FLAG_IF | FLAG_TF);
                        fail |= I686EmuPush(state, &params, 2, 2, &reg->cs);
                        fail |= I686EmuPush(state, &params, 2, 2, &reg->ip);
                        reg->cs = state->ivt[5] >> 16;
                        reg->eip = state->ivt[5] & 0xFFFF;
                    }
                    skipStore = true;
                    break;
                
                default:
                    return EMU_UNDEFINED_OPCODE;
                
        }
    }
    else //extended opcode
    {
        switch(opcode)
        {
            case 0xAF: //imul r16,r/m16; imul r32,r/m32
                I686EmuMul(reg, regOp, modrmOp, NULL, (uint32_t*)&regOp, params.override.operand ? 32 : 16, true);
                break;
            case 0x87: //conditional jumps
            case 0x83:
            case 0x82:
            case 0x86:
            case 0x84:
            case 0x8F:
            case 0x8D:
            case 0x8C:
            case 0x8E:
            case 0x85:
            case 0x81:
            case 0x8B:
            case 0x89:
            case 0x80:
            case 0x8A:
            case 0x88:
                reg->eip = I686EmuJump(reg, &params, opcode, true, coffs);
                skipStore = true;
                break;

            //load far pointer
            case 0xB2: //lss r16,m16:16; lss r32,m16:32
            case 0xB4: //lfs r16,m16:16; lfs r32,m16:32
            case 0xB5: //lgs r16,m16:16; lgs r32,m16:32
                v1.u64 = 0;
                I686EmuReadMemory(state, (uint32_t)modrmOp, params.override.operand ? 6 : 4, &v1);
                if(0xB2 == opcode)
                    reg->ss = v1.u64 >> (params.override.operand ? 32 : 16);
                else if(0xB4 == opcode)
                    reg->fs = v1.u64 >> (params.override.operand ? 32 : 16);
                else
                    reg->gs = v1.u64 >> (params.override.operand ? 32 : 16);
                regOp = v1.u64 & (params.override.operand ? 0xFFFFFFFF : 0xFFFF);
                params.noModRmWrite = 1;
                break;
            
            //stack
            case 0xA1: //pop fs
                fail |= I686EmuPop(state, &params, params.override.operand ? 4 : 2, 2, &reg->fs);
                break;
            case 0xA9: //pop gs
                fail |= I686EmuPop(state, &params, params.override.operand ? 4 : 2, 2, &reg->gs);
                break;
            case 0xA0: //push fs
                fail |= I686EmuPush(state, &params, params.override.operand ? 4 : 2, 2, &reg->fs);
                break;
            case 0xA8: //push gs
                fail |= I686EmuPush(state, &params, params.override.operand ? 4 : 2, 2, &reg->gs);
                break;
            
            //bit test
            case 0xA3: //bt r/m16,r16; bt r/m32,r32
            case 0xAB: //bts r/m16,r16; bts r/m32,r32
            case 0xB3: //btr r/m16,r16; btr r/m32,r32
            case 0xBB: //btc r/m16,r16; btc r/m32,r32
                if(modrmOp & (1 << regOp))
                    reg->flags |= FLAG_CF;
                else
                    reg->flags &= ~FLAG_CF;
                if(0xAB == opcode) //bts
                    modrmOp |= (1 << regOp);
                else if(0xB3 == opcode) //btr
                    modrmOp &= ~(1 << regOp);
                else if(0xBB == opcode) //btc
                    modrmOp ^= (1 << regOp);
                break;
            case 0xBA: //bt/bts/btr/btc r/m16,imm8; bt/bts/btr/btc r/m32,imm8
                if(modrmOp & (1 << imm))
                    reg->flags |= FLAG_CF;
                else
                    reg->flags &= ~FLAG_CF;
                
                if(0x5 == variant) //bts
                    modrmOp |= (1 << imm);
                else if(0x6 == variant) //btr
                    modrmOp &= ~(1 << imm);
                else if(0x7 == variant) //btc
                    modrmOp ^= (1 << imm);
                break; 
            
            //bit scan
            case 0xBC: //bsf r16,r/m16; bsf r32,/rm32
                if(0 == modrmOp)
                    reg->flags |= FLAG_ZF;
                else
                    regOp = stdc_trailing_zeros((uint32_t)modrmOp);
                break;
            case 0xBD: //bsr r16,r/m16; bsr r32,/rm32
                if(0 == modrmOp)
                    reg->flags |= FLAG_ZF;
                else
                    regOp = 31 - stdc_leading_zeros((uint32_t)modrmOp);
                break;

            //double shift
            case 0xA5: //shld r/m16,r16,cl; shld r/m32,r32,cl
            case 0xAD: //shrd r/m16,r16,cl; shrd r/m32,r32,cl
                imm = reg->cl;
                [[fallthrough]];
            case 0xA4: //shld r/m16,r16,imm8; shld r/m32,r32,imm8
            case 0xAC: //shrd r/m16,r16,imm8; shrd r/m32,r32,imm8
                if(0 == imm)
                    break;
                reg->flags &= ~(FLAG_SF | FLAG_ZF | FLAG_PF | FLAG_CF);
                v1.u32 = modrmOp & (1 << (params.override.operand ? 31 : 15)); //store original sign bit

                if((0xA5 == opcode) || (0xA4 == opcode)) //left shift
                {
                    if(modrmOp & (1 << ((params.override.operand ? 31 : 15) - imm)))
                        reg->flags |= FLAG_CF;

                    modrmOp = (modrmOp << imm) | (regOp >> ((params.override.operand ? 32 : 16) - imm));
                }
                else //right sift
                {
                    if(modrmOp & (1 << (imm - 1)))
                        reg->flags |= FLAG_CF;

                    modrmOp = (modrmOp >> imm) | (regOp << ((params.override.operand ? 32 : 16) - imm));
                }
                if(!params.override.operand)
                    modrmOp &= 0xFFFF;
                v2.u32 = modrmOp & (1 << (params.override.operand ? 31 : 15)); //current sign bit

                if(v2.u32)
                    reg->flags |= FLAG_SF;
                if(v2.u32 != v1.u32)
                    reg->flags |= FLAG_OF;
                if(0 == modrmOp)
                    reg->flags |= FLAG_ZF;
                if(0 == (stdc_count_ones(modrmOp & 0xFF) & 1))
                    reg->flags |= FLAG_PF;
                break;
            
            //mov with zero/sign extension
            case 0xB6: //movzx r16,r/m8; movzx r32,r/m8
                regOp = params.override.operand ? (uint32_t)((uint8_t)modrmOp) : (uint16_t)((uint8_t)modrmOp);
                break;
            case 0xB7: //movzx r32,r/m16
                regOp = (uint32_t)((uint16_t)modrmOp);
                break;
            case 0xBE: //movsx r16,r/m8; movsx r32,r/m8
                regOp = params.override.operand ? (int32_t)((int8_t)modrmOp) : (int16_t)((int8_t)modrmOp);
                break;
            case 0xBF: //movsx r32,r/m16
                regOp = (int32_t)((int16_t)modrmOp);
                break;

            //conditional move
            case 0x47: //cmova 
            case 0x43: //cmovae  = cmovnb 
            case 0x42: //cmovb  = cmovc  = cmovnae 
            case 0x46: //cmovbe  = cmovna 
            case 0x44: //cmove  = cmovz 
            case 0x4F: //cmovg  = cmovnle 
            case 0x4D: //cmovge  = cmovnl 
            case 0x4C: //cmovl  = cmovnge 
            case 0x4E: //cmovle  = cmovng 
            case 0x45: //cmovne  = cmovnz 
            case 0x41: //cmovno 
            case 0x4B: //cmovnp  = cmovpo 
            case 0x49: //cmovns 
            case 0x40: //cmovo 
            case 0x4A: //cmovp  = cmovpe 
            case 0x48: //cmovs
                if(I686EmuCheckCondition(reg, opcode + 0x30))
                    regOp = modrmOp;
                break;

            //conditional set
            case 0x97: //seta 
            case 0x93: //setae  = setnb 
            case 0x92: //setb  = setc  = setnae 
            case 0x96: //setbe  = setna 
            case 0x94: //sete  = setz 
            case 0x9F: //setg  = setnle 
            case 0x9D: //setge  = setnl 
            case 0x9C: //setl  = setnge 
            case 0x9E: //setle  = setng 
            case 0x95: //setne  = setnz 
            case 0x91: //setno 
            case 0x9B: //setnp  = setpo 
            case 0x99: //setns 
            case 0x90: //seto 
            case 0x9A: //setp  = setpe 
            case 0x98: //sets
                modrmOp = I686EmuCheckCondition(reg, opcode - 0x20) ? 1 : 0;
                break;
            
            case 0xC8: //bswap r32
                regOp = BSWAP(regOp);
                break;

            case 0xB0: //cmpxchg r/m8,r8
                if(0 == I686EmuSub(reg, reg->al, modrmOp, 8, false))
                    modrmOp = regOp;
                else
                    reg->al = modrmOp;
                break;
            case 0xB1: //cmpxchg r/m16,r16; cmpxchg r/m32,r32
                if(0 == I686EmuSub(reg, reg->al, modrmOp, params.override.operand ? 32 : 16, false))
                    modrmOp = regOp;
                else if(params.override.operand)
                    reg->eax = modrmOp;
                else
                    reg->ax = modrmOp;
                break;
            case 0xC7: //cmpxchg8b m64
                v1.u64 = 0;
                fail |= I686EmuReadMemory(state, modrmOp, 8, &v1);
                v2.u64 = (uint64_t)reg->eax | ((uint64_t)reg->edx << 32);
                if(v1.u64 == v2.u64)
                {
                    reg->flags |= FLAG_ZF;
                    v1.u64 = (uint64_t)reg->ebx | ((uint64_t)reg->ecx << 32);
                    fail |= I686EmuWriteMemory(state, modrmOp, 8, &v1);
                }
                else
                {
                    reg->flags &= ~FLAG_ZF;
                    reg->eax = v1.u64 & 0xFFFFFFFF;
                    reg->edx = v1.u64 >> 32;
                }
                break;
            
            case 0xC0: //xadd r/m8,r8
            case 0xC1: //xadd r/m16,r16; xadd r/m32,r32
                v1.i32 = regOp;
                regOp = modrmOp;
                modrmOp = v1.i32;
                modrmOp = I686EmuAdd(reg, modrmOp, regOp, (0xC0 == opcode) ? 8 : (params.override.operand ? 32 : 16), false);
                break;

            default:
                return EMU_UNDEFINED_OPCODE;
        }
    }

    if(fail)
        return EMU_MEMORY_VIOLATION;

    if(0xFFFFFFFF == next)
        return EMU_MEMORY_VIOLATION;

    if(!skipStore)
    {
        I686GetSetOperands(state, &params, (uint32_t*)&regOp, (uint32_t*)&modrmOp, (uint32_t*)&imm, &coffs, true);
        reg->eip = next;
    }

    if(EMU_FAR_POINTER_TO_LINEAR(reg->cs, reg->eip)  >= I686_EMU_REAL_MODE_SPACE_SIZE)
        return EMU_MEMORY_VIOLATION;

    if(params.repne)
    {
        if((0 != (params.override.address ? --reg->ecx : --reg->cx)) && !(reg->flags & FLAG_ZF))
            goto repeat;
    }
    else if(params.rep)
    {
        if(0 != (params.override.address ? --reg->ecx : --reg->cx))
        {
            if(!(params.params & REPE_REPZ) || ((params.params & REPE_REPZ) && (reg->flags & FLAG_ZF)))
                goto repeat;
        }
    }

    return EMU_CONTINUE;
}

static uint32_t I686GetSetOperands(struct I686EmuState *state, const struct I686InstructionParams *params, uint32_t *regOp, uint32_t *modrmOp, uint32_t *imm, uint64_t *coffs, bool set)
{
    struct I686Registers *reg = &(state->registers);
    uint8_t *code = state->code;
    uint32_t segmentShift = EMU_FAR_POINTER_TO_LINEAR(reg->cs, 0);
    uint32_t ip = segmentShift + reg->eip + 1; //IP is at the byte following the opcode
    uint32_t fail = 0;

    if(NULL != regOp)
    {
        if(!set)
        {
            if(params->params & REG_MOD_RM)
                *regOp = I686EmuGetRegisterValue(reg, params, (code[ip] >> 3) & 0x7, (params->params & SEG_REG), false);
            else if(params->params & REG_IN_OPCODE)
                *regOp = I686EmuGetRegisterValue(reg, params, code[ip - 1] & 0x7, (params->params & SEG_REG), false); //FIXME: can opcode-encoded register be a segment register?
        }
        else
        {
            if(params->params & REG_MOD_RM)
                I686EmuSetRegisterValue(reg, params, (code[ip] >> 3) & 0x7, *regOp, (params->params & SEG_REG));
            else if(params->params & REG_IN_OPCODE)
                I686EmuSetRegisterValue(reg, params, code[ip - 1] & 0x7, *regOp, (params->params & SEG_REG)); //FIXME: can opcode-encoded register be a segment register?
        }
    }


    if(params->params & (MOD_RM_ONLY | REG_MOD_RM))
    {
        //decode MOD field
        uint8_t mod = code[ip] >> 6;
        uint8_t rm = code[ip] & 0x7;
        if(0x3 == mod) //when MOD=0b11, then R/M represents the register just like REG
        {
            if(!set)
                *modrmOp = I686EmuGetRegisterValue(reg, params, rm, (params->params & SEG_RM), false);
            else if(!params->noModRmWrite)
                I686EmuSetRegisterValue(reg, params, rm, *modrmOp, (params->params & SEG_RM));
        
            ++ip; //advance IP to the byte following ModR/M
        }
        else //for other MOD values, things are much more complicated
        {
            uint32_t address = 0;
            bool ebpBase = false;

            if(params->override.address) //32-bit addressing
            {
                if(0x4 == rm) //in r/m=4 (corresponding to SP) the SIB byte is used
                {
                    ++ip; //advance IP to point at SIB
                    uint32_t base = 0;
                    uint32_t index = 0;
                    //if mod=0 and bp is used as base, then base is assumed to be zero and an additional 32-bit displacement is used
                    //in any other case use provided base register
                    if((0x5 == (code[ip] & 0x7)) && (0x0 == mod))
                    {
                        mod = 0x2; //if BP is selected, then a 32-bit displacement is added. Lie to the following code, that MOD is actually 2
                        ebpBase = true;
                    }
                    else
                        base = I686EmuGetRegisterValue(reg, params, code[ip] & 0x7, false, true); //get base register

                    //index field=4 (corresponding to SP) is equivalent to index=0
                    //get index register for any other index field value
                    if(0x4 != ((code[ip] >> 3) & 0x7)) 
                    {
                        index = I686EmuGetRegisterValue(reg, params, (code[ip] >> 3) & 0x7, false, true) //get index register value
                            << (code[ip] >> 6); //and scale it
                    }
                    address = base + index;
                }
                else if((0x5 == rm) && (0x0 == mod)) //if r/m=5 (BP) and mod=0, then no register is used, but instead a 32-bit displacement is used
                {
                    mod = 0x2; //lie to the following code that mod=2, which means there is a 32-bit displacement
                }
                else //in any other case, use register and displacement
                {
                    address = I686EmuGetRegisterValue(reg, params, rm, false, true);
                    if(0x5 == rm)
                        ebpBase = true;
                }
            }
            else //16 bit addressing
            {
                if((0 == rm) || (1 == rm) || (7 == rm))
                    address += (uint32_t)((uint16_t)reg->bx); //BX is the common term
                if((0 == rm) || (2 == rm) || (4 == rm))
                    address += (uint32_t)((uint16_t)reg->si); //SI is the common term
                if((1 == rm) || (3 == rm) || (5 == rm))
                    address += (uint32_t)((uint16_t)reg->di); //DI is the common term
                if((2 == rm) || (3 == rm) || ((6 == rm) && (0 != mod)))
                {
                    address += (uint32_t)((uint16_t)reg->bp); //BP is the common term
                    ebpBase = true;
                }
                //for rm=6 and mod=0 no register is used, but a 16-bit displacement
                if((6 == rm) && (0 == mod))
                    mod = 0x2; //lie that mod=0x2, so we get a 16-bit displacement
            }

            ++ip; //advance byte to point to the byte following the ModR/M or SIB

            if(0x1 == mod) //MOD=1, 8-bit displacement
            {
                if(params->override.address)
                    address += (uint32_t)((int32_t)((int8_t)code[ip++]));
                else
                    address += (uint16_t)((int16_t)((int8_t)code[ip++]));
            }
            else if(0x2 == mod) //MOD=0x2, 32-bit or 16-bit displacement
            {
                if(params->override.address)
                {
                    address += ((uint32_t)code[ip] | (((uint32_t)code[ip + 1]) << 8) | (((uint32_t)code[ip + 2]) << 16) | (((uint32_t)code[ip + 3]) << 24));
                    ip += 4;
                }
                else
                {
                    address += (uint16_t)((int16_t)((uint16_t)code[ip] | (uint16_t)((code[ip + 1]) << 8)));
                    ip += 2;
                }
            }
            //no displacement in 0x00 mode, except when in 32-bit mode and r/m=5, or there is a SIB and SIB base=5,
            //or r/m=6 in 16-bit mode, but this case is handled by explicitly setting mod=0x2 earlier

            if(!params->override.address)
                address &= 0xFFFF;

            //select segment register
            if(!(params->params & NO_SEGMENT))
            {
                if(ebpBase) //(E)BP base implies SS segment register
                    address += (uint32_t)(reg->ss) * 16;
                else if(params->override.cs)
                    address += (uint32_t)(reg->cs) * 16;
                else if(params->override.es)
                    address += (uint32_t)(reg->es) * 16;
                else if(params->override.fs)
                    address += (uint32_t)(reg->fs) * 16;
                else if(params->override.gs)
                    address += (uint32_t)(reg->gs) * 16;
                else if(params->override.ss)
                    address += (uint32_t)(reg->ss) * 16;
                else if(params->override.ds)
                    address += (uint32_t)(reg->ds) * 16;
                else if(params->params & SEGMENT_CS)
                    address += (uint32_t)(reg->cs) * 16;
                else if(params->params & SEGMENT_SS)
                    address += (uint32_t)(reg->ss) * 16;
                else
                    address += (uint32_t)(reg->ds) * 16;
            }

            if(!set)
            {
                if(params->params & NO_DEREFERENCE)
                    *modrmOp = address;
                else if(params->params & BYTE_OPERAND)
                    fail |= I686EmuReadMemory(state, address, 1, modrmOp);
                else
                    fail |= I686EmuReadMemory(state, address, params->override.operand ? 4 : 2, modrmOp);
            }
            else if(!params->noModRmWrite)
            {
                if(params->params & BYTE_OPERAND)
                    fail |= I686EmuWriteMemory(state, address, 1, modrmOp);
                else
                    fail |= I686EmuWriteMemory(state, address, params->override.operand ? 4 : 2, modrmOp);
            }
        }
    }

    if(params->params & (IMM_BYTE | IMM_WORD_DWORD))
    {
        if(!set)
        {
            if(params->params & IMM_BYTE)
            {
                *imm = (uint32_t)(code[ip]);
                ip += 1;
            }
            else if(params->override.operand)
            {
                *imm = (uint32_t)(code[ip]) | ((uint32_t)(code[ip + 1]) << 8) | ((uint32_t)(code[ip + 2]) << 16) | ((uint32_t)(code[ip + 3]) << 24);
                ip += 4;
            }
            else
            {
                *imm = (uint32_t)(code[ip]) | ((uint32_t)(code[ip + 1]) << 8);
                ip += 2;
            }
        }
        else
        {
            if(params->params & IMM_BYTE)
                ip += 1;
            else if(params->override.operand)
                ip += 4;
            else
                ip += 2;
        }
    }
    else if(params->params & (MOFFS_BYTE | MOFFS_WORD_DWORD))
    {
        uint32_t address = 0;

        if(params->override.cs)
            address += (uint32_t)(reg->cs) * 16;
        else if(params->override.es)
            address += (uint32_t)(reg->es) * 16;
        else if(params->override.fs)
            address += (uint32_t)(reg->fs) * 16;
        else if(params->override.gs)
            address += (uint32_t)(reg->gs) * 16;
        else if(params->override.ss)
            address += (uint32_t)(reg->ss) * 16;
        else
            address += (uint32_t)(reg->ds) * 16;

        if(params->override.address)
        {
            address += (uint32_t)(code[ip]) | ((uint32_t)(code[ip + 1]) << 8) | ((uint32_t)(code[ip + 2]) << 16) | ((uint32_t)(code[ip + 3]) << 24);
            ip += 4;
        }
        else
        {
            address += (uint32_t)(code[ip]) | ((uint32_t)(code[ip + 1]) << 8);
            ip += 2;
        }

        if(params->params & NO_DEREFERENCE)
        {
            if(!set)
                *imm = address;
        }
        else
        {
            if(!set)
            {
                if(params->params & MOFFS_BYTE)
                    fail |= I686EmuReadMemory(state, address, 1, imm);
                else
                    fail |= I686EmuReadMemory(state, address, params->override.operand ? 4 : 2, imm);
            }
            else
            {
                if(params->params & MOFFS_BYTE)
                    fail |= I686EmuWriteMemory(state, address, 1, imm);
                else
                    fail |= I686EmuWriteMemory(state, address, params->override.operand ? 4 : 2, imm);
            }
        }
    }
    else if(params->params & (COFFS_BYTE | COFFS_WORD | COFFS_DWORD | COFFS_PWORD | COFFS_WORD_DWORD | COFFS_DWORD_PWORD))
    {
        if(!set)
        {
            if(params->params & COFFS_BYTE)
            {
                *coffs = (uint64_t)(code[ip]);
                ip += 1;
            }
            else if((params->params & COFFS_WORD) || ((params->params & COFFS_WORD_DWORD) && !params->override.operand))
            {
                *coffs = (uint64_t)(code[ip]) | ((uint64_t)(code[ip + 1]) << 8);
                ip += 2;
            }
            else if((params->params & COFFS_DWORD) 
                || ((params->params & COFFS_DWORD_PWORD) && !params->override.operand)
                || ((params->params & COFFS_WORD_DWORD) && params->override.operand))
            {
                *coffs = (uint64_t)(code[ip]) | ((uint64_t)(code[ip + 1]) << 8) | ((uint64_t)(code[ip + 2]) << 16) | ((uint64_t)(code[ip + 3]) << 24);
                ip += 4;
            }
            else if((params->params & COFFS_PWORD) || ((params->params & COFFS_DWORD_PWORD) && params->override.operand))
            {
                *coffs = (uint64_t)(code[ip]) | ((uint64_t)(code[ip + 1]) << 8) | ((uint64_t)(code[ip + 2]) << 16) | ((uint64_t)(code[ip + 3]) << 24)
                | ((uint64_t)(code[ip + 4]) << 32) | ((uint64_t)(code[ip + 5]) << 40);
                ip += 6;
            }
        }
        else
        {
            if(params->params & COFFS_BYTE)
                ip += 1;
            else if(params->params & COFFS_WORD)
                ip += 2;
            else if((params->params & COFFS_DWORD) || ((params->params & COFFS_DWORD_PWORD) && !params->override.operand))
                ip += 4;
            else if((params->params & COFFS_PWORD) || ((params->params & COFFS_DWORD_PWORD) && params->override.operand))
                ip += 6;
        }
    }
    if(!fail)
        return ip - segmentShift;
    else
        return 0xFFFFFFFF;
}