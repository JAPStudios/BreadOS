#include "ata.h"
#include "pfs.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_SECTOR_SIZE 512u
#define IMAGE_SECTOR_COUNT 32768u

static FILE *image_file;

int ata_init(void) {
    return image_file ? 1 : 0;
}

uint32_t ata_sector_count(void) {
    return IMAGE_SECTOR_COUNT;
}

int ata_read_sector(uint32_t lba,void *buffer) {
    if(!image_file || !buffer || lba>=IMAGE_SECTOR_COUNT ||
       fseek(image_file,(long)lba*IMAGE_SECTOR_SIZE,SEEK_SET)!=0)
        return -1;
    return fread(buffer,IMAGE_SECTOR_SIZE,1,image_file)==1 ? 0 : -1;
}

int ata_write_sector(uint32_t lba,const void *buffer) {
    if(!image_file || !buffer || lba>=IMAGE_SECTOR_COUNT ||
       fseek(image_file,(long)lba*IMAGE_SECTOR_SIZE,SEEK_SET)!=0)
        return -1;
    if(fwrite(buffer,IMAGE_SECTOR_SIZE,1,image_file)!=1 || fflush(image_file)!=0)
        return -1;
    return 0;
}

static int load_file(const char *path,uint8_t **bytes,size_t *size) {
    FILE *file=fopen(path,"rb");
    if(!file) return -1;
    if(fseek(file,0,SEEK_END)!=0) { fclose(file); return -1; }
    long length=ftell(file);
    if(length<=0 || length>65535L || fseek(file,0,SEEK_SET)!=0) {
        fclose(file);
        return -1;
    }
    uint8_t *data=(uint8_t *)malloc((size_t)length);
    if(!data) { fclose(file); return -1; }
    size_t read_count=fread(data,1,(size_t)length,file);
    int close_status=fclose(file);
    if(read_count!=(size_t)length || close_status!=0) {
        free(data);
        return -1;
    }
    *bytes=data;
    *size=(size_t)length;
    return 0;
}

static int verify_binary(const char *path,const uint8_t *expected,size_t size) {
    uint8_t *actual=(uint8_t *)malloc(size);
    if(!actual) return -1;
    int count=pfs_read_binary(path,actual,size);
    int valid=count==(int)size && memcmp(actual,expected,size)==0;
    free(actual);
    return valid?0:-1;
}

int main(int argc,char **argv) {
    if(argc!=4) {
        fprintf(stderr,"Usage: %s OUTPUT_DISK_IMAGE HELLO_ELF GUARD_ELF\n",argv[0]);
        return 2;
    }

    uint8_t *hello=0,*guard=0;
    size_t hello_size=0,guard_size=0;
    if(load_file(argv[2],&hello,&hello_size)!=0 ||
       load_file(argv[3],&guard,&guard_size)!=0) {
        fprintf(stderr,"Could not read both user ELF files (maximum 65535 bytes each).\n");
        free(hello);
        free(guard);
        return 1;
    }

    image_file=fopen(argv[1],"w+b");
    if(!image_file) {
        perror("Could not create disk image");
        free(hello);
        free(guard);
        return 1;
    }
    if(fseek(image_file,(long)IMAGE_SECTOR_COUNT*IMAGE_SECTOR_SIZE-1L,SEEK_SET)!=0 ||
       fputc(0,image_file)==EOF || fflush(image_file)!=0) {
        fprintf(stderr,"Could not size the BreadFS disk image.\n");
        fclose(image_file);
        free(hello);
        free(guard);
        return 1;
    }

    int formatted=pfs_init();
    if(formatted!=2 || !pfs_ready()) {
        fprintf(stderr,"BreadFS did not format the new image (status %d).\n",formatted);
        fclose(image_file);
        free(hello);
        free(guard);
        return 1;
    }
    if(pfs_write_binary("hello",hello,hello_size)!=(int)hello_size ||
       pfs_write_binary("guard",guard,guard_size)!=(int)guard_size ||
       pfs_init()!=1 || !pfs_ready() ||
       verify_binary("hello",hello,hello_size)!=0 ||
       verify_binary("guard",guard,guard_size)!=0) {
        fprintf(stderr,"BreadFS application install or read-back verification failed.\n");
        fclose(image_file);
        free(hello);
        free(guard);
        return 1;
    }

    free(hello);
    free(guard);
    if(fclose(image_file)!=0) {
        fprintf(stderr,"Could not finalize disk image.\n");
        return 1;
    }
    image_file=0;
    printf("Created 16 MiB BreadFS image with /hello (%lu bytes) and /guard (%lu bytes).\n",
           (unsigned long)hello_size,(unsigned long)guard_size);
    return 0;
}
