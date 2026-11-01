#include "privacy.h"
static uint8_t permissions[BREAD_PRIVACY_CAPABILITY_COUNT];
void privacy_init(void){privacy_reset();}
void privacy_reset(void){for(size_t i=0;i<BREAD_PRIVACY_CAPABILITY_COUNT;i++)permissions[i]=0;}
int privacy_set(enum bread_privacy_capability c,int enabled){if((unsigned)c>=BREAD_PRIVACY_CAPABILITY_COUNT||(enabled!=0&&enabled!=1))return -1;permissions[c]=(uint8_t)enabled;return 0;}
int privacy_allows(enum bread_privacy_capability c){if((unsigned)c>=BREAD_PRIVACY_CAPABILITY_COUNT)return 0;return permissions[c]!=0;}
const char *privacy_name(enum bread_privacy_capability c){static const char *const names[BREAD_PRIVACY_CAPABILITY_COUNT]={"camera","microphone","location","network","bluetooth","usb","sensors"};if((unsigned)c>=BREAD_PRIVACY_CAPABILITY_COUNT)return "invalid";return names[c];}
static int same_name(const char *a,const char *b){if(!a||!b)return 0;while(*a&&*b){char x=*a++,y=*b++;if(x>='A'&&x<='Z')x=(char)(x-'A'+'a');if(y>='A'&&y<='Z')y=(char)(y-'A'+'a');if(x!=y)return 0;}return *a==*b;}
int privacy_find(const char *name,enum bread_privacy_capability *out){if(!name||!out)return -1;for(unsigned i=0;i<BREAD_PRIVACY_CAPABILITY_COUNT;i++)if(same_name(name,privacy_name((enum bread_privacy_capability)i))){*out=(enum bread_privacy_capability)i;return 0;}return -1;}
int privacy_serialize(char *out,size_t cap){static const char p[]="P1:";const size_t need=3u+BREAD_PRIVACY_CAPABILITY_COUNT+1u;if(!out||cap<need)return -1;size_t n=0;for(size_t i=0;i<3;i++)out[n++]=p[i];for(size_t i=0;i<BREAD_PRIVACY_CAPABILITY_COUNT;i++)out[n++]=permissions[i]?'1':'0';out[n]='\0';return (int)n;}
int privacy_deserialize(const char *in){static const char p[]="P1:";uint8_t parsed[BREAD_PRIVACY_CAPABILITY_COUNT];if(!in)return -1;for(size_t i=0;i<3;i++)if(in[i]!=p[i])return -1;in+=3;for(size_t i=0;i<BREAD_PRIVACY_CAPABILITY_COUNT;i++){if(in[i]!='0'&&in[i]!='1')return -1;parsed[i]=(uint8_t)(in[i]-'0');}if(in[BREAD_PRIVACY_CAPABILITY_COUNT]!='\0'&&in[BREAD_PRIVACY_CAPABILITY_COUNT]!='\n'&&in[BREAD_PRIVACY_CAPABILITY_COUNT]!='\r')return -1;for(size_t i=0;i<BREAD_PRIVACY_CAPABILITY_COUNT;i++)permissions[i]=parsed[i];return 0;}
