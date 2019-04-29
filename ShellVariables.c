#include <string.h>
#include "ShellVariables.h"

#define VAR_SIZE 10

static char ** varNames;
static int num;

void allocVarSpace();
void mallocFail();

void setVariable(char* name, char* value){

    allocVarSpace();

    setenv(name, value, 1);
    strcpy(varNames[num], name);

    num++;
}

void allocVarSpace(){
    if(num == 0){
        varNames = malloc(sizeof(char*)); //allocates memory
        mallocFail();
    }
    else{
        varNames = realloc(varNames, sizeof(char*)*(num+1)); //allocates memory
        mallocFail();
    }

    varNames[num] = malloc(sizeof(char)*(VAR_SIZE));
    mallocFail();
}

void setInitVariables() {
    num = 0;

    setVariable("PROMPT", "eggshell > ");
    setVariable("SHELL", getenv("PWD"));
    setVariable("EXITCODE", " ");

    char cwd[50];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        setVariable("CWD", cwd);
    } else {
        perror("getcwd() error");
        exit(EXIT_FAILURE);
    }

    char *tty = ttyname(STDIN_FILENO);
    if (tty != NULL) {
        setVariable("TERMINAL", tty);
    } else {
        perror("ttyname() error");
        exit(EXIT_FAILURE);
    }

    allocVarSpace();
    strcpy(varNames[num], "PATH");
    num++;

    allocVarSpace();
    strcpy(varNames[num], "USER");
    num++;

    allocVarSpace();
    strcpy(varNames[num], "HOME");
    num++;
}

//char * getValue(char * name){
//    char * var;
//    if ((var = getenv(name)) != NULL) { //compares var names
//        return var;
//    }
//
//    return NULL;
//}

void displayVariables(){
    char temp[100];
    char varName[VAR_SIZE];

    for (int i = 0; i < num; ++i) {
        strcpy(varName, varNames[i]);

        strcpy(temp, varName);
        strcat(temp, "=");
        strcat(temp, getenv(varName));

        puts(temp);
    }
}

void displayOneVariable(char **name){
    for (int i = 0; i < num; ++i) {
        if(strstr(&name[0][1], varNames[i]) != NULL){
            printf("%s", getenv(varNames[i]));
            fflush(stdout);

            if(strlen(varNames[i]) < strlen(&name[0][1])){
                //to output from variable onwards
                printf("%s", &name[0][strlen(varNames[i])+1]);
                fflush(stdout);
            }
            return;
        }
    }

    printf("--> Variable NAME does not exist  <--");
    fflush(stdout);

}

void checkSetVariables(char **args){
    //if name & value only inputted by user
    if (args[1] == NULL) {
        const char delim[2] = "=";
        char *token;

        char *temp[VAR_SIZE];


        int j = 0;

        /* get the first token */
        token = strtok(*args, delim);

        while (token != NULL) {

            temp[j] = token;

            token = strtok(NULL, delim);
            j++;
        }


        for (int i = 0; i < num; ++i) {
            //checking if variable already exists
            if (strcmp(temp[0], varNames[i]) == 0) {
                //to initialize with value of existing variable
                if (temp[1][0] == '$') {
                    setenv(temp[0], getenv(&temp[1][1]), 1); //without updating count
                }
                else {
                    setenv(temp[0], temp[1], 1); //without updating count
                }
                return;
            }
        }

        //if variable to be initialised doesn't yet exist

        //to initialize with value of existing variable
        if (temp[1][0] == '$') {
            setVariable(temp[0], getenv(&temp[1][1]));
            return;
        } else {
            setVariable(temp[0], temp[1]);
            return;
        }
    }
    else {
        puts("--> NAME=VALUE  <--");
    }
}

void mallocFail(){
    if (varNames == NULL){ //if memory request fails
        puts("-- Memory allocation failed. --\nGoodbye");
        for (int i = 0; i < num; ++i) {
            free(varNames[i]);
        }
        free(varNames);
    }
}

void freeVar(){
    for (int i = 0; i < num; ++i) {
        free(varNames[i]);
    }
    free(varNames);
}

void fail(){
    for (int i = 0; i < num; ++i) {
        free(varNames[i]);
    }
    free(varNames);
    exit(EXIT_FAILURE);
}