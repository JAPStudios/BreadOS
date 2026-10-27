#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "kernel.c"
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
 puts("PASS: create, text preservation, move in/out, copy, collisions, directory-cycle rejection, invalid paths, quoted parsing and snapshot validation.");
}
