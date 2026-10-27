#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "shell_parse.h"
static char buffer[256];static char *args[16];
static int parse(const char *s){return shell_parse(s,buffer,sizeof(buffer),args,16);}
int main(void) {
 assert(parse("")==0);
 assert(parse(" \t ")==0);
 assert(parse("mv /notes.txt /Documents/")==3);
 assert(strcmp(args[1],"/notes.txt")==0);
 assert(parse("mv \"/my notes.txt\" \"/My Documents/\"")==3);
 assert(strcmp(args[1],"/my notes.txt")==0);
 assert(strcmp(args[2],"/My Documents/")==0);
 assert(parse("mv '/my notes.txt' /")==3);
 assert(strcmp(args[1],"/my notes.txt")==0);
 assert(parse("touch /my\\ notes.txt")==2);
 assert(strcmp(args[1],"/my notes.txt")==0);
 assert(parse("echo \"\" ''")==3);
 assert(!args[1][0]&&!args[2][0]);
 assert(parse("echo ab\" c\"d")==2);
 assert(strcmp(args[1],"ab cd")==0);
 assert(parse("echo 'a\\b'")==2);
 assert(strcmp(args[1],"a\\b")==0);
 assert(parse("echo \"unfinished")<0);
 assert(parse("echo unfinished\\")<0);
 char tiny[2];char *one[1];
 assert(shell_parse("abc",tiny,sizeof(tiny),one,1)==-3);
 assert(shell_parse("a b",buffer,sizeof(buffer),one,1)==-2);
 assert(shell_parse("a",buffer,0,one,1)==-3);
 char edge[256];memset(edge,'a',255);edge[255]=0;
 assert(parse(edge)==1);assert(strlen(args[0])==255);
 puts("PASS: whitespace, normal moves, quoted paths, single quotes, escapes, empty arguments, concatenated quotes, malformed input and capacity boundaries.");
}
