#include <stdint.h>
static const char keys[128]={
 [2]='1',[3]='2',[4]='3',[5]='4',[6]='5',[7]='6',[8]='7',[9]='8',[10]='9',[11]='0',[12]='-',[13]='=',[14]='\b',
 [16]='q',[17]='w',[18]='e',[19]='r',[20]='t',[21]='y',[22]='u',[23]='i',[24]='o',[25]='p',[26]='[',[27]=']',[28]='\n',
 [30]='a',[31]='s',[32]='d',[33]='f',[34]='g',[35]='h',[36]='j',[37]='k',[38]='l',[39]=';',[40]=39,[41]='`',[43]=92,
 [44]='z',[45]='x',[46]='c',[47]='v',[48]='b',[49]='n',[50]='m',[51]=',',[52]='.',[53]='/',[57]=' '};
static const char upper[128]={[2]='!',[3]='@',[4]='#',[5]='$',[6]='%',[7]='^',[8]='&',[9]='*',[10]='(',[11]=')',[12]='_',[13]='+',[26]='{',[27]='}',[39]=':',[40]='"',[41]='~',[43]='|',[51]='<',[52]='>',[53]='?'};
static inline uint8_t in8(uint16_t p){uint8_t v;__asm__ volatile("inb %1,%0":"=a"(v):"Nd"(p));return v;}
char keyboard_getchar(void){
 static unsigned shift,skip;
 for(;;){uint8_t s=in8(0x64);if(!(s&1))continue;uint8_t c=in8(0x60);if(s&0xE0)continue;
 if(c==0xE1){skip=5;continue;}if(skip){skip--;continue;}if(c==0xE0){skip=1;continue;}
 unsigned k=c&127;if(k==42||k==54){unsigned bit=k==42?1:2;if(c&128)shift&=~bit;else shift|=bit;continue;}
 if(c&128)continue;char v=keys[k];if(shift){if(v>='a'&&v<='z')v-=32;else if(upper[k])v=upper[k];}if(v)return v;}
}
