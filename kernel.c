#include <stdint.h>
#include <stddef.h>
/* Standalone replacement prototype: all runtime modules in one unit. */
void *memcpy(void *d,const void *s,size_t n){unsigned char *a=d;const unsigned char *b=s;while(n--)*a++=*b++;return d;}
void *memset(void *d,int v,size_t n){unsigned char *a=d;while(n--)*a++=(unsigned char)v;return d;}
void *memmove(void *d,const void *s,size_t n){unsigned char *a=d;const unsigned char *b=s;if((uintptr_t)a<(uintptr_t)b){for(size_t i=0;i<n;i++)a[i]=b[i];}else{while(n){n--;a[n]=b[n];}}return d;}
int memcmp(const void *a,const void *b,size_t n){const unsigned char *x=a,*y=b;for(size_t i=0;i<n;i++)if(x[i]!=y[i])return x[i]-y[i];return 0;}
static int eq(const char *a,const char *b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
static int textcopy(char *d,const char *s,unsigned cap){unsigned n=0;while(s[n]){if(n>=cap-1)return -1;n++;}for(unsigned i=0;i<=n;i++)d[i]=s[i];return 0;}
static inline uint8_t in8(uint16_t p){uint8_t v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
static inline uint16_t in16(uint16_t p){uint16_t v;__asm__ volatile("inw %1,%0":"=a"(v):"Nd"(p));return v;}
static inline void out8(uint16_t p,uint8_t v){__asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p));}
static inline void out16(uint16_t p,uint16_t v){__asm__ volatile("outw %0,%1"::"a"(v),"Nd"(p));}
static volatile uint16_t *const screen=(volatile uint16_t *)0xB8000;
static unsigned row,col;
static void clear(void){for(unsigned i=0;i<2000;i++)screen[i]=0x0F20;row=col=0;}
static void put(char c){
 if(c=='\n'){col=0;row++;}
 else if(c=='\b'){if(col)col--;else if(row){row--;col=79;}else return;screen[row*80+col]=0x0F20;}
 else{screen[row*80+col]=0x0F00|(unsigned char)c;if(++col==80){col=0;row++;}}
 if(row>=25){for(unsigned i=0;i<1920;i++)screen[i]=screen[i+80];for(unsigned i=1920;i<2000;i++)screen[i]=0x0F20;row=24;}
}
static void print(const char *s){while(*s)put(*s++);}
static const char keys[128]={
 [2]='1',[3]='2',[4]='3',[5]='4',[6]='5',[7]='6',[8]='7',[9]='8',[10]='9',[11]='0',[12]='-',[13]='=',[14]='\b',
 [16]='q',[17]='w',[18]='e',[19]='r',[20]='t',[21]='y',[22]='u',[23]='i',[24]='o',[25]='p',[26]='[',[27]=']',[28]='\n',
 [30]='a',[31]='s',[32]='d',[33]='f',[34]='g',[35]='h',[36]='j',[37]='k',[38]='l',[39]=';',[40]=39,[41]='`',[43]=92,
 [44]='z',[45]='x',[46]='c',[47]='v',[48]='b',[49]='n',[50]='m',[51]=',',[52]='.',[53]='/',[57]=' '};
static const char upper[128]={[2]='!',[3]='@',[4]='#',[5]='$',[6]='%',[7]='^',[8]='&',[9]='*',[10]='(',[11]=')',[12]='_',[13]='+',[26]='{',[27]='}',[39]=':',[40]='"',[41]='~',[43]='|',[51]='<',[52]='>',[53]='?'};
static char key(void){
 static unsigned shift,skip;
 for(;;){uint8_t s=in8(0x64);if(!(s&1))continue;uint8_t c=in8(0x60);if(s&0xE0)continue;
 if(c==0xE1){skip=5;continue;}if(skip){skip--;continue;}if(c==0xE0){skip=1;continue;}
 unsigned k=c&127;if(k==42||k==54){unsigned bit=k==42?1:2;if(c&128)shift&=~bit;else shift|=bit;continue;}
 if(c&128)continue;char v=keys[k];if(shift){if(v>='a'&&v<='z')v-=32;else if(upper[k])v=upper[k];}if(v)return v;}
}
#define NODES 64
#define NAME 32
#define CONTENT 1024
typedef struct{uint32_t used,dir,parent;char name[NAME],data[CONTENT];} Node;
static Node nodes[NODES],candidate[NODES];
static unsigned cwd;
static int child(unsigned p,const char *name){for(unsigned i=1;i<NODES;i++)if(nodes[i].used&&nodes[i].parent==p&&eq(nodes[i].name,name))return (int)i;return -1;}
static int resolve(const char *p){
 if(!p||!*p)return -1;unsigned at=*p=='/'?0:cwd;int trailing=0;
 while(*p){while(*p=='/'){p++;trailing=1;}if(!*p)break;if(!nodes[at].dir)return -1;
 char part[NAME];unsigned n=0;while(*p&&*p!='/'){if(n>=NAME-1)return -1;part[n++]=*p++;}part[n]=0;trailing=0;
 if(eq(part,"."))continue;if(eq(part,"..")){at=nodes[at].parent;continue;}
 int i=child(at,part);if(i<0)return -1;at=(unsigned)i;}
 return trailing&&!nodes[at].dir?-1:(int)at;
}
static int target(const char *path,unsigned *parent,char *name){
 char b[256];if(!path||!*path||textcopy(b,path,sizeof b)<0)return -1;
 int last=-1;unsigned n=0;while(b[n]){if(b[n]=='/')last=(int)n;n++;}
 if(last==(int)n-1||textcopy(name,b+last+1,NAME)<0||eq(name,".")||eq(name,".."))return -1;
 int p;if(last<0)p=(int)cwd;else if(last==0)p=0;else{b[last]=0;p=resolve(b);}
 if(p<0||!nodes[p].dir)return -1;*parent=(unsigned)p;return 0;
}
static int create(const char *p,int dir){
 unsigned parent;char name[NAME];if(target(p,&parent,name)<0)return -1;int old=child(parent,name);
 if(old>=0)return !dir&&!nodes[old].dir?old:-1;
 for(unsigned i=1;i<NODES;i++)if(!nodes[i].used){memset(&nodes[i],0,sizeof(Node));nodes[i].used=1;nodes[i].dir=(unsigned)dir;nodes[i].parent=parent;textcopy(nodes[i].name,name,NAME);return (int)i;}return -1;
}
static int destination(int src,const char *dst,unsigned *p,char *name){
 int d=resolve(dst);if(d>=0){if(!nodes[d].dir)return -1;*p=(unsigned)d;textcopy(name,nodes[src].name,NAME);}else if(target(dst,p,name)<0)return -1;return child(*p,name)<0?0:-1;
}
static int movefile(const char *src,const char *dst,int copying){
 int i=resolve(src);unsigned p;char name[NAME];if(i<=0||(copying&&nodes[i].dir)||destination(i,dst,&p,name)<0)return -1;
 if(copying){for(unsigned j=1;j<NODES;j++)if(!nodes[j].used){nodes[j]=nodes[i];nodes[j].parent=p;textcopy(nodes[j].name,name,NAME);return 0;}return -1;}
 for(unsigned a=p;;a=nodes[a].parent){if(a==(unsigned)i)return -1;if(!a)break;}
 nodes[i].parent=p;textcopy(nodes[i].name,name,NAME);return 0;
}
static int removefile(const char *p){int i=resolve(p);if(i<=0)return -1;
 for(unsigned a=cwd;;a=nodes[a].parent){if(a==(unsigned)i)return -1;if(!a)break;}
 for(unsigned j=1;j<NODES;j++)if(nodes[j].used&&nodes[j].parent==(unsigned)i)return -1;
 memset(&nodes[i],0,sizeof(Node));return 0;
}
static void list(const char *p){int d=resolve(p);if(d<0||!nodes[d].dir){print("Directory not found.\n");return;}
 for(unsigned i=1;i<NODES;i++)if(nodes[i].used&&nodes[i].parent==(unsigned)d){print(nodes[i].name);print(nodes[i].dir?"/\n":"\n");}}
static void pwd(void){unsigned chain[NODES],n=0;for(unsigned i=cwd;i;i=nodes[i].parent)chain[n++]=i;print("/");while(n){print(nodes[chain[--n]].name);if(n)print("/");}put('\n');}
/* QEMU primary-master ATA PIO only. Never attach a real disk. */
static void delay(void){for(unsigned i=0;i<4;i++)(void)in8(0x3F6);}
static int ready(int data){for(unsigned i=0;i<1000000;i++){uint8_t s=in8(0x1F7);if(!s||s==255)return -1;if(s&128)continue;if(s&0x21)return -1;if(!data||(s&8))return 0;}return -1;}
static int sector(unsigned lba,unsigned char *b,int writing){
 if(lba>=2048)return -1;out8(0x3F6,2);out8(0x1F6,0xE0);delay();if(ready(0)<0)return -1;
 out8(0x1F1,0);out8(0x1F2,1);out8(0x1F3,lba&255);out8(0x1F4,(lba>>8)&255);out8(0x1F5,0);
 out8(0x1F7,writing?0x30:0x20);delay();if(ready(1)<0)return -1;
 for(unsigned i=0;i<256;i++){if(writing)out16(0x1F0,(uint16_t)b[2*i]|((uint16_t)b[2*i+1]<<8));else{uint16_t v=in16(0x1F0);b[2*i]=v&255;b[2*i+1]=v>>8;}}
 delay();return ready(0);
}
static int flush(void){out8(0x1F6,0xE0);delay();if(ready(0)<0)return -1;out8(0x1F7,0xE7);delay();return ready(0);}
static unsigned char block[512];
static uint32_t checksum(const unsigned char *p,unsigned n){uint32_t h=2166136261U;for(unsigned i=0;i<n;i++){h^=p[i];h*=16777619U;}return h;}
static void put32(unsigned off,uint32_t n){for(unsigned i=0;i<4;i++)block[off+i]=(unsigned char)(n>>(i*8));}
static uint32_t get32(unsigned off){uint32_t n=0;for(unsigned i=0;i<4;i++)n|=(uint32_t)block[off+i]<<(i*8);return n;}
static int terminated(const char *s,unsigned n){for(unsigned i=0;i<n;i++)if(!s[i])return 1;return 0;}
static int valid(void){
 if(candidate[0].used!=1||candidate[0].dir!=1||candidate[0].parent||candidate[0].name[0])return 0;
 for(unsigned i=1;i<NODES;i++){Node *n=&candidate[i];if(n->used>1)return 0;if(!n->used)continue;
 if(n->dir>1||n->parent>=NODES||!candidate[n->parent].used||!candidate[n->parent].dir)return 0;
 if(!terminated(n->name,NAME)||!n->name[0]||eq(n->name,".")||eq(n->name,"..")||!terminated(n->data,CONTENT))return 0;
 for(unsigned j=0;n->name[j];j++)if(n->name[j]=='/')return 0;
 for(unsigned j=1;j<i;j++)if(candidate[j].used&&candidate[j].parent==n->parent&&eq(candidate[j].name,n->name))return 0;
 unsigned a=i,count=0;while(a){if(a>=NODES||!candidate[a].used||++count>=NODES)return 0;a=candidate[a].parent;}}
 return 1;
}
static int save(void){
 unsigned size=sizeof nodes,total=(size+511)/512;const unsigned char *bytes=(const unsigned char *)nodes;
 memset(block,0,512);if(sector(0,block,1)<0||flush()<0)return -1;
 for(unsigned s=0;s<total;s++){for(unsigned j=0;j<512;j++){unsigned k=s*512+j;block[j]=k<size?bytes[k]:0;}if(sector(s+1,block,1)<0)return -1;}
 if(flush()<0)return -1;memset(block,0,512);put32(0,0x32465342U);put32(4,size);put32(8,checksum(bytes,size));
 if(sector(0,block,1)<0||flush()<0)return -1;return 0;
}
static int load(void){
 if(sector(0,block,0)<0||get32(0)!=0x32465342U||get32(4)!=sizeof nodes)return -1;
 uint32_t hash=get32(8);unsigned size=sizeof nodes,total=(size+511)/512;unsigned char *bytes=(unsigned char *)candidate;
 for(unsigned s=0;s<total;s++){if(sector(s+1,block,0)<0)return -1;for(unsigned j=0;j<512;j++){unsigned k=s*512+j;if(k<size)bytes[k]=block[j];}}
 if(checksum(bytes,size)!=hash||!valid())return -1;memcpy(nodes,candidate,sizeof nodes);cwd=0;return 0;
}
static int parse(const char *line,char *buf,char **argv){
 unsigned n=0;int count=0;
 while(*line){while(*line==' ')line++;if(!*line)break;if(count==16)return -1;argv[count++]=buf+n;char quote=0;
 while(*line){char c=*line;if(!quote&&c==' ')break;line++;
 if(c=='\\'&&quote!=39){if(!*line)return -1;c=*line++;}
 else if(c==39||c=='"'){if(!quote){quote=c;continue;}if(quote==c){quote=0;continue;}}
 if(n>=255)return -1;buf[n++]=c;}
 if(quote||n>=256)return -1;buf[n++]=0;}return count;
}
static void status(int r){if(r<0)print("Operation failed; check paths, collision, limits, or disk.\n");}
static int arity(int actual,int expected){if(actual==expected)return 1;print("Wrong argument count. Type help.\n");return 0;}
void kernel_main(void){
 clear();memset(nodes,0,sizeof nodes);nodes[0].used=nodes[0].dir=1;
 print("Kernel ready. Type help. Use load to restore a saved snapshot.\n");
 char line[256],storage[256],text[256];char *args[16];
 for(;;){unsigned n=0;print("> ");for(;;){char c=key();if(c=='\n'){put(c);break;}if(c=='\b'){if(n){n--;put(c);}}else if(c>=32&&c<=126&&n<255){line[n++]=c;put(c);}}line[n]=0;
 int argc=parse(line,storage,args);if(argc<0){print("Invalid quotes, escape, or argument limit.\n");continue;}if(!argc)continue;char *cmd=args[0];
 if(eq(cmd,"help")){print("help | clear | pwd | ls [PATH] | cd PATH\nmkdir PATH | touch PATH | write PATH TEXT | cat PATH\nmv SOURCE DEST | cp SOURCE DEST | rm PATH\nsave | load | echo TEXT | halt\nQuote names containing spaces. Save/load are explicit.\n");}
 else if(eq(cmd,"clear")){if(arity(argc,1))clear();}
 else if(eq(cmd,"pwd")){if(arity(argc,1))pwd();}
 else if(eq(cmd,"ls")){if(argc<=2)list(argc==2?args[1]:".");else arity(argc,2);}
 else if(eq(cmd,"save")){if(arity(argc,1)){int r=save();if(!r)print("Saved to disk.img.\n");else status(r);}}
 else if(eq(cmd,"load")){if(arity(argc,1)){int r=load();if(!r)print("Loaded snapshot; current directory /.\n");else print("Load failed; live files unchanged.\n");}}
 else if(eq(cmd,"halt")){if(arity(argc,1)){print("Halted. Close QEMU.\n");for(;;)__asm__ volatile("cli; hlt");}}
 else if(eq(cmd,"echo")){for(int i=1;i<argc;i++){if(i>1)put(' ');print(args[i]);}put('\n');}
 else if(eq(cmd,"mv")||eq(cmd,"cp")){if(arity(argc,3))status(movefile(args[1],args[2],eq(cmd,"cp")));}
 else if(eq(cmd,"write")){if(argc<3){print("Usage: write PATH TEXT\n");continue;}int id=resolve(args[1]);if(id<0||nodes[id].dir){status(-1);continue;}
 unsigned used=0;for(int i=2;i<argc;i++){if(i>2)text[used++]=' ';for(char *p=args[i];*p&&used<255;p++)text[used++]=*p;}text[used]=0;status(textcopy(nodes[id].data,text,CONTENT));}
 else if(eq(cmd,"mkdir")||eq(cmd,"touch")||eq(cmd,"cd")||eq(cmd,"cat")||eq(cmd,"rm")){
 if(!arity(argc,2))continue;
 if(eq(cmd,"mkdir"))status(create(args[1],1));else if(eq(cmd,"touch"))status(create(args[1],0));else if(eq(cmd,"rm"))status(removefile(args[1]));
 else{int id=resolve(args[1]);if(id<0){status(-1);continue;}if(eq(cmd,"cd")){if(nodes[id].dir)cwd=(unsigned)id;else status(-1);}else{if(nodes[id].dir)status(-1);else{print(nodes[id].data);put('\n');}}}}
 else print("Unknown command. Type help.\n");
 }
}
