#include "launcher.h"
#include <assert.h>
int main(void){
 size_t r[8];assert(bread_launcher_entry_count()==6u);
 size_t n=bread_launcher_search("file",r,8u);assert(n==1u);assert(bread_launcher_entry_at(r[0])->app_id==1u);
 n=bread_launcher_search("app",r,8u);assert(n>=1u);assert(bread_launcher_entry_at(r[0])->app_id==2u);
 assert(bread_launcher_search("zz-no-match",r,8u)==0u);
 assert(bread_launcher_search("",r,8u)==6u);
 assert(bread_launcher_search("system",r,2u)==2u);return 0;
}
