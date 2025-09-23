#include "stdio.h"
#include "../kernel/logger.h"

void cmd_init(void) {
    console_init();
    //logger_log(OK, "STDIO initialized");
}