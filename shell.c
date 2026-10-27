#include <stddef.h>
#include "terminal.h"
#include "keyboard.h"
#include "shell.h"
#include "fs.h"
#include "shell_parse.h"
static int equal(const char *a,const char *b) {
 while(*a&&*a==*b){a++;b++;}return *a==*b;
}
static void result(int code) {
 if(code<0)terminal_write("Failed: check paths, type, destination collision, or filesystem limits.\n");
}
static int count(int argc,int expected) {
 if(argc==expected)return 1;
 terminal_write("Wrong number of arguments. Type help.\n");return 0;
}
static void print_words(int argc,char **argv,int first) {
 for(int i=first;i<argc;i++){
  if(i>first)terminal_putc(' ');
  terminal_write(argv[i]);
 }
 terminal_putc('\n');
}
void shell_run(void) {
 char line[256],storage[256],text[256];char *argv[16];
 fs_init();
 for(;;) {
  size_t n=0;terminal_write("> ");
  for(;;) {
   char c=keyboard_getchar();
   if(c=='\n'){terminal_putc(c);break;}
   if(c=='\b'){if(n){n--;terminal_putc(c);}}
   else if(c>=32&&c<=126&&n<sizeof(line)-1){line[n++]=c;terminal_putc(c);}
  }
  line[n]=0;
  int argc=shell_parse(line,storage,sizeof(storage),argv,16);
  if(argc<0){terminal_write("Parse error: unmatched quote, trailing escape, or too many arguments.\n");continue;}
  if(!argc)continue;
  const char *cmd=argv[0];
  if(equal(cmd,"help")) {
   terminal_write("help | clear | about | echo TEXT | halt\nls [PATH] | pwd | cd PATH\nmkdir PATH | touch PATH | write PATH TEXT | cat PATH\nmv SOURCE DEST | cp SOURCE DEST | rm PATH\nQuote paths containing spaces: mv \"my file.txt\" /Documents/\n");
#ifdef FS_PERSISTENCE
   terminal_write("save | load (load replaces RAM files with the saved snapshot)\n");
#endif
  } else if(equal(cmd,"echo"))print_words(argc,argv,1);
  else if(equal(cmd,"clear")){if(count(argc,1))terminal_clear();}
  else if(equal(cmd,"about")){if(count(argc,1))terminal_write("Starter kernel with quoted paths and file commands.\n");}
  else if(equal(cmd,"halt")) {
   if(count(argc,1)){terminal_write("CPU halted.\n");for(;;)__asm__ volatile("cli; hlt");}
  } else if(equal(cmd,"pwd")){if(count(argc,1))fs_pwd();}
  else if(equal(cmd,"ls")) {
   if(argc<=2)fs_list(argc==2?argv[1]:".");else count(argc,2);
  }
#ifdef FS_PERSISTENCE
  else if(equal(cmd,"save")){if(count(argc,1))terminal_write(fs_save()==0?"Saved.\n":"Save failed.\n");}
  else if(equal(cmd,"load")){if(count(argc,1))terminal_write(fs_load()==0?"Loaded; current directory /.\n":"Load failed; RAM unchanged.\n");}
#endif
  else if(equal(cmd,"mv")||equal(cmd,"cp")) {
   if(count(argc,3))result(equal(cmd,"mv")?fs_move(argv[1],argv[2]):fs_copy(argv[1],argv[2]));
  } else if(equal(cmd,"write")) {
   if(argc<3){terminal_write("Usage: write PATH TEXT\n");continue;}
   size_t used=0;int overflow=0;
   for(int i=2;i<argc;i++) {
    if(i>2){if(used>=sizeof(text)-1){overflow=1;break;}text[used++]=' ';}
    for(const char *p=argv[i];*p;p++) {
     if(used>=sizeof(text)-1){overflow=1;break;}text[used++]=*p;
    }
    if(overflow)break;
   }
   if(overflow){terminal_write("Text too long.\n");continue;}
   text[used]=0;result(fs_write(argv[1],text));
  } else if(equal(cmd,"mkdir")||equal(cmd,"touch")||equal(cmd,"cd")||equal(cmd,"rm")||equal(cmd,"cat")) {
   if(!count(argc,2))continue;
   if(equal(cmd,"mkdir"))result(fs_mkdir(argv[1]));
   else if(equal(cmd,"touch"))result(fs_touch(argv[1]));
   else if(equal(cmd,"cd"))result(fs_chdir(argv[1]));
   else if(equal(cmd,"rm"))result(fs_remove(argv[1]));
   else {const char *p=fs_read(argv[1]);if(p){terminal_write(p);terminal_putc('\n');}else result(-1);}
  } else terminal_write("Unknown command. Type help.\n");
 }
}
