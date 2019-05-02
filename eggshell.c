#include <stdio.h>
#include "linenoise/linenoise.h"
#include "ShellVariables.h"
#include "CommandlineInterpreter.h"

int main(void) {

    setInitVariables();
    commandlineInterpreter();

    return 0;
}

