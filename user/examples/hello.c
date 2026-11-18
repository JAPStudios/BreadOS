#include <stddef.h>
#include "breados/syscall.h"

static volatile uint32_t app_state;

void _start(void) {
    app_state=1u;
    static const char message[]="BreadOS native user-mode application.\n";
    if (bread_write(message,(uint32_t)(sizeof(message)-1u))<0)
        bread_exit(1);
    bread_exit(0);
}
