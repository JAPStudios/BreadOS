#include "elf_loader.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint8_t image_bytes[8192];
static void put16(size_t o,uint16_t v){image_bytes[o]=(uint8_t)v;image_bytes[o+1]=(uint8_t)(v>>8);}
static void put32(size_t o,uint32_t v){for(unsigned i=0;i<4;i++)image_bytes[o+i]=(uint8_t)(v>>(8*i));}
static void make_image(void) {
    memset(image_bytes,0,sizeof(image_bytes));
    image_bytes[0]=0x7F;image_bytes[1]='E';image_bytes[2]='L';image_bytes[3]='F';
    image_bytes[4]=1;image_bytes[5]=1;image_bytes[6]=1;
    put16(16,2);put16(18,3);put32(20,1);put32(24,ELF32_USER_START);
    put32(28,52);put16(40,52);put16(42,32);put16(44,1);
    put32(52,1);put32(56,4096);put32(60,ELF32_USER_START);
    put32(68,4);put32(72,4096);put32(76,5);put32(80,4096);
    image_bytes[4096]=0x90;image_bytes[4097]=0x90;
    image_bytes[4098]=0x90;image_bytes[4099]=0xC3;
}
int main(void) {
    struct elf32_image parsed;
    struct elf32_load_segment segment;
    make_image();
    assert(elf32_inspect(image_bytes,sizeof(image_bytes),&parsed)==0);
    assert(parsed.entry==ELF32_USER_START);
    uint32_t code_limit=0;
    assert(elf32_user_code_limit(&parsed,&code_limit)==0);
    assert(code_limit==ELF32_USER_START+4095u);
    assert(elf32_load_segment_at(&parsed,0,&segment)==0);
    assert(segment.offset==4096 && segment.file_size==4 && segment.memory_size==4096);
    assert(elf32_load_segment_at(&parsed,1,&segment)==-1);

    assert(elf32_inspect(image_bytes,100,&parsed)==-1);
    make_image();image_bytes[0]=0;assert(elf32_inspect(image_bytes,sizeof(image_bytes),&parsed)==-1);
    make_image();put16(18,0x3E);assert(elf32_inspect(image_bytes,sizeof(image_bytes),&parsed)==-1);
    make_image();put32(68,9000);assert(elf32_inspect(image_bytes,sizeof(image_bytes),&parsed)==-1);
    make_image();put32(72,0x403FF000);put32(80,8192);assert(elf32_inspect(image_bytes,sizeof(image_bytes),&parsed)==-1);
    make_image();put32(76,7);assert(elf32_inspect(image_bytes,sizeof(image_bytes),&parsed)==-1);
    make_image();put32(24,ELF32_USER_START+0x1000);assert(elf32_inspect(image_bytes,sizeof(image_bytes),&parsed)==-1);
    make_image();put32(28,0xFFFFFFF0);assert(elf32_inspect(image_bytes,sizeof(image_bytes),&parsed)==-1);
    /* A writable segment may start at the page immediately after executable pages. */
    make_image();put16(44,2);
    put32(84,1);put32(88,8192);put32(92,ELF32_USER_START+4096u);
    put32(96,ELF32_USER_START+4096u);put32(100,0);put32(104,4);
    put32(108,6);put32(112,4096);
    assert(elf32_inspect(image_bytes,sizeof(image_bytes),&parsed)==0);
    assert(elf32_user_code_limit(&parsed,&code_limit)==0);
    assert(code_limit==ELF32_USER_START+4095u);

    /* Writable memory below the executable range cannot be covered by user CS. */
    make_image();put16(44,2);put32(24,ELF32_USER_START+4096u);
    put32(76,6);
    put32(84,1);put32(88,8192);put32(92,ELF32_USER_START+4096u);
    put32(96,ELF32_USER_START+4096u);put32(100,4);put32(104,4096);
    put32(108,5);put32(112,4096);
    assert(elf32_inspect(image_bytes,sizeof(image_bytes),&parsed)==-1);

    /* Program headers must not alias the ELF file header. */
    make_image();put32(28,40);assert(elf32_inspect(image_bytes,sizeof(image_bytes),&parsed)==-1);
    /* p_align must be a power of two and satisfy p_vaddr/p_offset congruence. */
    make_image();put32(80,3);assert(elf32_inspect(image_bytes,sizeof(image_bytes),&parsed)==-1);
    make_image();put32(80,8192);assert(elf32_inspect(image_bytes,sizeof(image_bytes),&parsed)==-1);
    /* Dynamic linking and interpreter segments are deliberately unsupported. */
    make_image();put32(52,2);assert(elf32_inspect(image_bytes,sizeof(image_bytes),&parsed)==-1);
    make_image();put32(52,3);assert(elf32_inspect(image_bytes,sizeof(image_bytes),&parsed)==-1);
    /* Two load segments may not map overlapping user virtual ranges. */
    make_image();put16(44,2);
    put32(84,1);put32(88,4096);put32(92,ELF32_USER_START);
    put32(100,1);put32(104,4096);put32(108,4);put32(112,4096);
    assert(elf32_inspect(image_bytes,sizeof(image_bytes),&parsed)==-1);
    /* Reject empty memory ranges and file data larger than the mapped range. */
    make_image();put32(72,0);assert(elf32_inspect(image_bytes,sizeof(image_bytes),&parsed)==-1);
    make_image();put32(68,4097);assert(elf32_inspect(image_bytes,sizeof(image_bytes),&parsed)==-1);
    /* Reject writable-and-executable pages, unknown flags and unreadable loads. */
    make_image();put32(76,7);assert(elf32_inspect(image_bytes,sizeof(image_bytes),&parsed)==-1);
    make_image();put32(76,13);assert(elf32_inspect(image_bytes,sizeof(image_bytes),&parsed)==-1);
    make_image();put32(76,1);assert(elf32_inspect(image_bytes,sizeof(image_bytes),&parsed)==-1);
    /* The page below the user stack is a guard and must remain unmapped. */
    make_image();put32(24,ELF32_USER_STACK_GUARD_PAGE);
    put32(60,ELF32_USER_STACK_GUARD_PAGE);
    assert(elf32_inspect(image_bytes,sizeof(image_bytes),&parsed)==-1);
    /* A segment may end exactly at the guard boundary without touching it. */
    make_image();put32(24,ELF32_USER_STACK_GUARD_PAGE-4096u);
    put32(60,ELF32_USER_STACK_GUARD_PAGE-4096u);
    assert(elf32_inspect(image_bytes,sizeof(image_bytes),&parsed)==0);
    puts("ELF loader validation tests passed");
    return 0;
}
