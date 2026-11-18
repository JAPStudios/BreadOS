#include <stddef.h>
#include "breados/syscall.h"

void _start(void) {
    static const char message[]="BreadOS native user-mode application.\n";
    if (bread_write(message,(uint32_t)(sizeof(message)-1u))<0)
        bread_exit(1);
    bread_exit(0);
}
