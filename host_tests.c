#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "kernel.c"
#include "kernel/pmm.h"

unsigned char __kernel_start[1];
unsigned char __kernel_end[1];
#include "heap.h"
int main(void){
 memset(nodes,0,sizeof nodes);nodes[0].used=nodes[0].dir=1;
 assert(create("/My Documents",1)>0);
 int i=create("/my notes.txt",0);assert(i>0);
 assert(textcopy(nodes[i].data,"Hello",CONTENT)==0);
 assert(movefile("/my notes.txt","/My Documents/",0)==0);
 assert(resolve("/my notes.txt")<0);
 assert(resolve("/My Documents/my notes.txt")==i);
 assert(eq(nodes[i].data,"Hello"));
 assert(movefile("/My Documents/my notes.txt","/",0)==0);
 assert(movefile("/my notes.txt","/My Documents/copy.txt",1)==0);
 assert(movefile("/my notes.txt","/My Documents/copy.txt",0)<0);
 assert(create("/My Documents/sub",1)>0);
 assert(movefile("/My Documents","/My Documents/sub",0)<0);
 assert(resolve("/my notes.txt/")<0);
 assert(resolve("/my notes.txt/../My Documents")<0);
 char b[256];char *a[16];
 assert(parse("mv \"/my notes.txt\" \"/My Documents/\"",b,a)==3);
 assert(eq(a[1],"/my notes.txt"));
 assert(parse("echo \"unterminated",b,a)<0);
 memcpy(candidate,nodes,sizeof nodes);assert(valid());
 candidate[i].parent=(unsigned)i;assert(!valid());
 heap_init();
 size_t initial_heap=heap_available();assert(initial_heap>60000);
 unsigned char *first=(unsigned char *)heap_alloc(17);
 unsigned char *second=(unsigned char *)heap_alloc(300);
 assert(first&&second);
 assert(((uintptr_t)first%16u)==0&&((uintptr_t)second%16u)==0);
 for(unsigned j=0;j<17;j++)first[j]=(unsigned char)(j+1);
 for(unsigned j=0;j<17;j++)assert(first[j]==(unsigned char)(j+1));
 unsigned char *zeroed=(unsigned char *)heap_calloc(32,1);assert(zeroed);
 for(unsigned j=0;j<32;j++)assert(zeroed[j]==0);
 assert(heap_free(first)==0);
 assert(heap_free(second)==0);
 assert(heap_free(zeroed)==0);
 assert(heap_free(zeroed)<0);
 assert(heap_free((void *)(uintptr_t)1)<0);
 assert(heap_available()==initial_heap);
 struct TestMmapEntry {
  uint32_t size;
  uint64_t address;
  uint64_t length;
  uint32_t type;
 } __attribute__((packed));
 const struct TestMmapEntry fake_map[]={
  {20,0,8u*1024u*1024u,1},
  {20,8u*1024u*1024u,1u*1024u*1024u,2},
  {20,9u*1024u*1024u,7u*1024u*1024u,1}
 };
 assert(pmm_init_from_memory_map(fake_map,sizeof fake_map,0x00100000u,0x00200000u)==0);
 assert(pmm_free_pages()==3328u);
 uintptr_t page=pmm_alloc_page();
 assert(page==0x00200000u);
 assert((page&4095u)==0);
 assert(page<0x00800000u||page>=0x00900000u);
 assert(pmm_free_pages()==3327u);
 assert(pmm_free_page(page)==0);
 assert(pmm_free_pages()==3328u);
 assert(pmm_free_page(page)<0);
 assert(pmm_reserve_range(0x00200000u,4096u)==0);
 assert(pmm_free_pages()==3327u);
 assert(pmm_free_page(0x00200000u)<0);
 assert(pmm_free_page(0x00800000u)<0);
 puts("PASS: filesystem path/move/copy/snapshot; heap allocation/coalescing; PMM map parsing, reserved ranges, allocation and release.");
}
