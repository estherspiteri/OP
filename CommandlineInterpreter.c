#include "ShellVariables.h"
#include "CommandlineInterpreter.h"
#include "stdbool.h"
#include "linenoise/linenoise.h"

void running(char *line, char **args);
int fileExists(char *filename);
void commands(char *line);
void internal(char *line, char **args);
void redirectTo(char *line);
void redirectFrom(char *line);
void piping(char *line);
void forking(char **args);
void setExitCode(int num);

bool exitFlag = false;

void commandlineInterpreter() {
    char *line;

    linenoiseHistorySetMaxLen(20);

    /* Load history from file. The history file is just a plain text file
    * where entries are separated by newlines. */
    linenoiseHistoryLoad("history.txt"); /* Load the history at startup */


    while ((line = linenoise(getenv("PROMPT"))) != NULL) {
        exitFlag = false;

        if(strstr(line, ">")){
            redirectTo(line);
        }
        else if(strstr(line, "<")) {
            redirectFrom(line);
        }
        else if(strstr(line, "|")){
            piping(line);
        }
        else {
            commands(line);
        }
        linenoiseFree(line);

        if(!exitFlag){
            char error [50];
            //setting EXITCODE with errno
            sprintf(error, "%d", errno);

            setenv("EXITCODE", error, 1);
        }
    }
}

void commands(char *line){
    linenoiseHistoryAdd(line); /* Add to the history. */
    linenoiseHistorySave("history.txt"); /* Save the history on disk. */

    const char delim[2] = " ";
    char *token;

    char *args[ARG];


//        if(strcmp(&line[0], " ") == 0){
//            puts("yo");
//            continue;}

    int j = 0;

    /* get the first token */
    token = strtok(line, delim);

    while (token != NULL) {

        args[j] = token;

        token = strtok(NULL, delim);
        j++;
    }
    args[j] = (char *) NULL;

    running(line, args);
}

void running(char *line, char **args){
    char internalComm [6][VAR_SIZE]= {"exit", "all", "=", "print", "chdir", "source"};

    for (int i = 0; i < sizeof(internalComm)/VAR_SIZE; ++i) {
        if (strstr(args[0], internalComm[i])) {
            internal(line, args);
            return;
        }
    }
    forking(args);
}

int fileExists(char *filename){
    //tries to open file to read
    FILE *file = NULL;

    /*opens a file for reading;
      the file must exist*/
    if ((file = fopen(filename, "rb"))){
        fclose(file);
        return 1;
    }
    return 0;
}

void internal(char *line, char **args){

    if (strcmp(args[0], "exit") == 0) {
        linenoiseFree(line);
        freeVar();
        exit(0);
    }
    else if (strcmp(args[0], "all") == 0) {
        displayVariables();
    }
    else if (strstr(*args, "=") != NULL) {
        checkSetVariables(args);
    }
    else if (strcmp(args[0], "print") == 0) {
        int count = 1;

        while (args[count] != NULL) {
            if (args[count][0] == '$') {
                displayOneVariable(&args[count]);
            }
            else{
                printf("%s", args[count]);
                fflush(stdout); //to flush output stream
            }
            printf(" ");
            count++;
        }
        printf("\n");
    }
    else if (strcmp(args[0], "chdir") == 0){
        if (chdir(args[1]) == 0) {
            char cwd[50];
            if (getcwd(cwd, sizeof(cwd)) != NULL) {
                setenv("CWD", cwd, 1);
            } else {
                perror("getcwd() error");
                setExitCode(errno);
            }
        } else {
            perror("chdir() error");
            setExitCode(errno);
        }
    }
    else if (strcmp(args[0], "source") == 0) {

        if (fileExists(args[1]) == 1) {
            FILE *fp = NULL;
            char word[1024];

            //opens file
            if ((fp = fopen(args[1], "rb")) != NULL) {
                printf("-- File \"%s\" opened. --\n", args[1]);
            }

            //reading contents from buffer and executing them
            while (fgets(word, 1024, fp) != NULL) {
                //to remove extra newline
                int len = (int) strlen(word);
                if (word[len - 1] == '\n') {
                    word[len - 1] = '\0';
                }
                if(strcmp(word, "\0") != 0) {
                    commands(word);
                }
            }

            fclose(fp);
        }
        else {
            printf("--> File does not exist <--\n");
        }
    }
}

void redirectTo(char *line){
    char *command;
    char *filename;
    int saved_stdout;

    int fd;

    if (strstr(line, ">>")){
        command = strtok(line, ">>");
        filename = strtok(NULL, ">>");

        if ((fd = open(filename, O_APPEND | O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR)) == -1){
            perror("dup() failed");
            setExitCode(errno);
        }
    }
    else {
        command = strtok(line, ">");
        filename = strtok(NULL, ">");

        if ((fd = open(filename, O_RDWR | O_CREAT | O_TRUNC , S_IRUSR | S_IWUSR)) == -1){
            perror("dup() failed");
            setExitCode(errno);
        }
    }

    /* Save current stdout for use later */
    if ((saved_stdout = dup(STDOUT_FILENO)) == -1){
        perror("dup() failed");
        setExitCode(errno);
    }

    // make stdout go to file
    if ((dup2(fd, 1)) == -1){
        perror("dup2() failed");
        setExitCode(errno);
    }

    // make stderr go to file
    if ((dup2(fd, 2)) == -1){
        perror("dup2() failed");
        setExitCode(errno);
    }

    close(fd);

    commands(command);

    /* Restore stdout */
    if ((dup2(saved_stdout, STDOUT_FILENO)) == -1){
        perror("dup2() failed");
        setExitCode(errno);
    }

    close(saved_stdout);

    printf("--> Output redirected to file <--\n");
    fflush(stdout);
}

void redirectFrom(char *line){
    char *command;
    char temp[1024];

    puts("yo");

    FILE *fp = NULL; //file pointer

    if (strstr(line, "<<<")) {
        char *hereString;

        command = strtok(line, "<<<");
        hereString = strtok(NULL, "<<<");

        if((strtok(hereString, "*")) != NULL) {
            hereString = strtok(NULL, "*");

            strcpy(temp, command);
            strcat(temp, hereString);

            if(temp[strlen(temp) - 1] != ' ') {
                strcat(temp, " ");
            }

            while(strstr(line = linenoise("*"), "*") == NULL) {
                strcat(temp, line);

                if(line[strlen(line) - 1] != ' ') {
                    strcat(temp, " ");
                }
            }

            if(strcmp(line, "*") != 0) {
                strcat(temp, strtok(line, "*"));
            }

            //to add extra space
            int lenC = (int) strlen(command);
            if (command[lenC - 1] == 0) {
                command[lenC - 1] = ' ';
            }
            commands(temp);
        }
    }
    else {
        char *filename;

        command = strtok(line, "<");
        filename = strtok(NULL, "<");

        //to remove extra space
        int len = (int) strlen(filename);
        if (filename[len - 1] == ' ') {
            filename[len - 1] = 0;
        }

        //to add extra space
        int lenC = (int) strlen(command);
        if (command[lenC - 1] == 0) {
            command[lenC - 1] = ' ';
        }

        if (fileExists(filename) == 1) {
            char word[1024];

            //opens file
            if ((fp = fopen(filename, "rb")) != NULL) {
                printf("-- File \"%s\" opened. --\n", filename);
            }

            //reading contents from buffer and executing them
            while (fgets(word, 1024, fp) != NULL) {

                //to remove extra newline
                int lenW = (int) strlen(word);
                if (word[lenW - 1] == '\n') {
                    word[lenW - 1] = '\0';
                }

                if(strcmp(word, "\0") != 0) {
                    strcpy(temp, command);
                    strcat(temp, word);

                    commands(temp);
                }
            }
            fclose(fp);
        }
        else {
            printf("--> File does not exist <--\n");
        }
    }
}

void piping(char *line){
    char *input;
    char *output;

#define READ_END	0
#define WRITE_END	1

    int saved_stdout;

    input = strtok(line, "|");
    output = strtok(NULL, "|");

    pid_t pid;
    int fd[2];

    pipe(fd);
    pid = fork();

    if(pid==0)
    {
        dup2(fd[WRITE_END], STDOUT_FILENO);
        close(fd[READ_END]);
        close(fd[WRITE_END]);
        commands(input);
        exit(1);
    }
    else
    {
        pid=fork();

        if(pid==0)
        {
            dup2(fd[READ_END], STDIN_FILENO);
            close(fd[WRITE_END]);
            close(fd[READ_END]);
            commands(output);
            exit(1);
        }
        else
        {
            int status;
            close(fd[READ_END]);
            close(fd[WRITE_END]);
            waitpid(pid, &status, 0);
        }
    }
}

void forking(char **args){
    char prompt[PROMPT_VAR];
    int status;

    time_t curtime;
    time(&curtime);

    pid_t pid = fork();

    if (pid == -1) {
        printf("%d\n", errno);
        perror("fork() failed");
        setExitCode(errno);

    } else if (pid == 0) {
        //child
        if (execvp(args[0], args)) {
            perror("execvp() failed");
            setExitCode(errno);
            return;
        }

        //dead code
        printf("This string should never get printed\n");

    }
    else {
        //parent
        do {
            //pid of child
            pid = waitpid(pid, &status, WNOHANG); //wait until a state change in the child process
            //WNOHSNG --> checks child processes without causing the caller to be suspended

            //check status of child
            if (pid > 0) {
                if (WIFEXITED(status) > 0) {
                    //returned from main(), or else called the exit() or _exit() function
                    printf("Child exited with status of %d\n", WEXITSTATUS(status));
                } else {
                    printf("Child did not exit successfully\n");
                    printf("Child is still running at %s", ctime(&curtime));
                    //ctime() --> returns a string representing the localtime based on the argument timer.
                    sleep(1);
                }
            }
            else if (pid == -1) {
                printf("%d\n", errno);
                perror("Wait failed!");
                setExitCode(errno);
            }

        } while (pid == 0);


        if (WEXITSTATUS(status) != EXIT_FAILURE) {
            memset(prompt, 0, sizeof(prompt));
            strcat(prompt, args[0]);
            strcat(prompt, " > ");
            setenv("PROMPT", prompt, 1);
        }

       setExitCode(errno);
    }
}

void setExitCode(int num){
    char error [50];
    //setting EXITCODE with errno
    sprintf(error, "%d", num);
    setenv("EXITCODE", error, 1);
    exitFlag = true;
}