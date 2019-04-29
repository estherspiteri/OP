#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include "linenoise/linenoise.c"
#include "linenoise/linenoise.h"
#include "ShellVariables.c"
#include "ShellVariables.h"
#include "CommandlineInterpreter.h"
#include "CommandlineInterpreter.c"

#define ARG 100

int main(void) {

    setInitVariables();
    commandlineInterpreter();

    return 0;
}

