#include "elf_loader.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc,char **argv) {
    assert(argc==2);
    FILE *file=fopen(argv[1],"rb");
    assert(file);
    assert(fseek(file,0,SEEK_END)==0);
    long length=ftell(file);
    assert(length>0 && (unsigned long)length<=65535ul);
    assert(fseek(file,0,SEEK_SET)==0);
    uint8_t *bytes=(uint8_t *)malloc((size_t)length);
    assert(bytes);
    assert(fread(bytes,1,(size_t)length,file)==(size_t)length);
    assert(fclose(file)==0);

    struct elf32_image image;
    assert(elf32_inspect(bytes,(size_t)length,&image)==0);
    assert(image.entry>=ELF32_USER_START);
    printf("Validated %s entry=0x%08X\n",argv[1],image.entry);
    uint32_t load_count=0;
    int entry_executable=0;
    for(uint16_t i=0;i<image.program_header_count;i++) {
        struct elf32_load_segment segment;
        int result=elf32_load_segment_at(&image,i,&segment);
        if(result==1) continue;
        assert(result==0);
        assert((segment.flags&4u)!=0);
        assert((segment.flags&3u)!=3u);
        printf("  load vaddr=0x%08X mem=%u flags=0x%X\n",segment.virtual_address,segment.memory_size,segment.flags);
        if((segment.flags&1u) && image.entry>=segment.virtual_address &&
           (uint64_t)image.entry<(uint64_t)segment.virtual_address+segment.memory_size)
            entry_executable=1;
        ++load_count;
    }
    assert(load_count>0 && entry_executable);
    free(bytes);
    return 0;
}
