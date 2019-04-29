#ifndef EGGSHELL_SHELLVARIABLES_H
#define EGGSHELL_SHELLVARIABLES_H

//#include "ShellVariables.c"
#include <stdlib.h>
#include <stdio.h>

#define PROMPT_VAR 20
#define ARG 100


//shell variables
void setInitVariables();
void setVariable(char *name, char *value);
void displayVariables();
void displayOneVariable(char **name);
void checkSetVariables(char **args);

void freeVar();

#endif //EGGSHELL_SHELLVARIABLES_H
