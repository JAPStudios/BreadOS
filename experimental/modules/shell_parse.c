#include "shell_parse.h"
int shell_parse(const char *line,char *storage,unsigned int capacity,char **argv,int max_args) {
 unsigned int n=0;int argc=0;
 if(!line||!storage||!argv||max_args<0)return -1;
 while(*line) {
  while(*line==' '||*line=='\t')line++;
  if(!*line)break;
  if(argc>=max_args)return -2;
  if(n>=capacity)return -3;
  argv[argc++]=storage+n;
  char quote=0;
  while(*line) {
   char c=*line;
   if(!quote&&(c==' '||c=='\t'))break;
   line++;
   if(c=='\\'&&quote!='\'') {
    if(!*line)return -1;
    c=*line++;
   } else if(c=='\''||c=='"') {
    if(!quote){quote=c;continue;}
    if(quote==c){quote=0;continue;}
   }
   if(n>=capacity)return -3;
   storage[n++]=c;
  }
  if(quote)return -1;
  if(n>=capacity)return -3;
  storage[n++]=0;
 }
 return argc;
}
