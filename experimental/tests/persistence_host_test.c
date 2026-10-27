#define COUNT 64
#define NAME 32
#define DATA 1024
typedef struct { int used, directory, parent; char name[NAME], data[DATA]; } Node;
static Node nodes[COUNT];
static int cwd;
static int eq(const char *a,const char *b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
#include "fs_persistence.inc"

#include <assert.h>
#include <string.h>
#include <stdio.h>
static unsigned char image[1024*1024];
int ata_read_sector(uint32_t lba,void *b){if(lba>=2048)return -1;memcpy(b,image+lba*512,512);return 0;}
int ata_write_sector(uint32_t lba,const void *b){if(lba>=2048)return -1;memcpy(image+lba*512,b,512);return 0;}
int ata_flush(void){return 0;}
int main(void){
 nodes[0].used=nodes[0].directory=1;
 nodes[1].used=1;nodes[1].parent=0;
 strcpy(nodes[1].name,"notes.txt");strcpy(nodes[1].data,"saved content");
 assert(fs_load()<0);
 assert(fs_save()==0);
 memset(nodes,0,sizeof(nodes));cwd=7;
 assert(fs_load()==0);assert(cwd==0);
 assert(strcmp(nodes[1].data,"saved content")==0);
 image[520]^=1;
 strcpy(nodes[1].data,"keep live RAM");
 assert(fs_load()<0);
 assert(strcmp(nodes[1].data,"keep live RAM")==0);
 puts("PASS: blank-image rejection, snapshot round trip, directory reset, corruption rejection, preservation of live RAM after failed load.");
 return 0;
}
