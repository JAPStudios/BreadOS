#include "launcher.h"
static const struct bread_launcher_entry entries[] = {
 {"Shell","System","Kernel command interface",0},
 {"Files","System","Browse BreadFS files",1},
 {"Appearance","Personalization","Choose desktop color theme",2},
 {"System","System","View kernel and memory status",3},
 {"Notifications","System","Review alerts and notification history",4},
 {"Clipboard","Productivity","Browse and restore clipboard history",5}
};
static char fold(char c){return c>='A'&&c<='Z'?(char)(c-'A'+'a'):c;}
static int score_text(const char *s,const char *q){
 if(!q||!*q)return 0;int score=0,first=1;size_t pos=0;
 for(;*q;q++){char wanted=fold(*q);while(s[pos]&&fold(s[pos])!=wanted){pos++;score++;}
 if(!s[pos])return -1;if(first&&pos)score+=4;first=0;pos++;}return score;
}
static int score_entry(const struct bread_launcher_entry *e,const char *q){
 int best=score_text(e->name,q),v=score_text(e->category,q),w=score_text(e->description,q);
 if(v>=0&&(best<0||v+3<best))best=v+3;
 if(w>=0&&(best<0||w+8<best))best=w+8;return best;
}
const struct bread_launcher_entry *bread_launcher_entry_at(size_t i){return i<6u?&entries[i]:0;}
size_t bread_launcher_entry_count(void){return 6u;}
size_t bread_launcher_search(const char *q,size_t *out,size_t cap){
 if(!out||!cap)return 0;size_t n=0;
 for(size_t i=0;i<6u;i++){int score=score_entry(&entries[i],q);if(score<0)continue;
 size_t at=n<cap?n:cap-1u;
 while(at){int prev=score_entry(&entries[out[at-1u]],q);if(prev<=score)break;
 if(at<cap)out[at]=out[at-1u];at--;}
 out[at]=i;if(n<cap)n++;
 }return n;
}
