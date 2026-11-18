#include "ata.h"
#include "pfs.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_SECTOR_SIZE 512u
#define IMAGE_SECTOR_COUNT 32768u
#define APP_COUNT 5u

struct app_image {
    const char *name;
    uint8_t *bytes;
    size_t size;
};

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
    static const char *const names[APP_COUNT]={"hello","guard","victim","attacker","wxguard"};
    struct app_image apps[APP_COUNT];
    for(uint32_t i=0;i<APP_COUNT;i++) {
        apps[i].name=names[i];
        apps[i].bytes=0;
        apps[i].size=0;
    }
    if(argc!=(int)(APP_COUNT+2u)) {
        fprintf(stderr,"Usage: %s OUTPUT_DISK_IMAGE HELLO_ELF GUARD_ELF VICTIM_ELF ATTACKER_ELF WXGUARD_ELF\n",argv[0]);
        return 2;
    }
    for(uint32_t i=0;i<APP_COUNT;i++) {
        if(load_file(argv[i+2u],&apps[i].bytes,&apps[i].size)!=0) {
            fprintf(stderr,"Could not read user ELF: %s\n",argv[i+2u]);
            for(uint32_t j=0;j<APP_COUNT;j++) free(apps[j].bytes);
            return 1;
        }
    }

    image_file=fopen(argv[1],"w+b");
    if(!image_file) {
        perror("Could not create disk image");
        for(uint32_t i=0;i<APP_COUNT;i++) free(apps[i].bytes);
        return 1;
    }
    if(fseek(image_file,(long)IMAGE_SECTOR_COUNT*IMAGE_SECTOR_SIZE-1L,SEEK_SET)!=0 ||
       fputc(0,image_file)==EOF || fflush(image_file)!=0) {
        fprintf(stderr,"Could not size the BreadFS disk image.\n");
        fclose(image_file);
        for(uint32_t i=0;i<APP_COUNT;i++) free(apps[i].bytes);
        return 1;
    }

    int formatted=pfs_init();
    if(formatted!=2 || !pfs_ready()) {
        fprintf(stderr,"BreadFS did not format the new image (status %d).\n",formatted);
        fclose(image_file);
        for(uint32_t i=0;i<APP_COUNT;i++) free(apps[i].bytes);
        return 1;
    }
    for(uint32_t i=0;i<APP_COUNT;i++) {
        if(pfs_write_binary(apps[i].name,apps[i].bytes,apps[i].size)!=(int)apps[i].size) {
            fprintf(stderr,"Could not install /%s into BreadFS.\n",apps[i].name);
            fclose(image_file);
            for(uint32_t j=0;j<APP_COUNT;j++) free(apps[j].bytes);
            return 1;
        }
    }
    if(pfs_init()!=1 || !pfs_ready()) {
        fprintf(stderr,"BreadFS failed to remount after application installation.\n");
        fclose(image_file);
        for(uint32_t i=0;i<APP_COUNT;i++) free(apps[i].bytes);
        return 1;
    }
    for(uint32_t i=0;i<APP_COUNT;i++) {
        if(verify_binary(apps[i].name,apps[i].bytes,apps[i].size)!=0) {
            fprintf(stderr,"BreadFS read-back verification failed for /%s.\n",apps[i].name);
            fclose(image_file);
            for(uint32_t j=0;j<APP_COUNT;j++) free(apps[j].bytes);
            return 1;
        }
    }

    for(uint32_t i=0;i<APP_COUNT;i++) free(apps[i].bytes);
    if(fclose(image_file)!=0) {
        fprintf(stderr,"Could not finalize disk image.\n");
        return 1;
    }
    image_file=0;
    printf("Created 16 MiB BreadFS image with five verified ELF32 applications.\n");
    return 0;
}
