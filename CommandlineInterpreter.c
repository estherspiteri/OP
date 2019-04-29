#include "ShellVariables.h"
#include "CommandlineInterpreter.h"
#include "stdbool.h"

const char internalComm [6][VAR_SIZE]= {"exit", "all", "=", "print", "chdir", "source"};

int fileExists(char *filename);
void commands(char *line);
void internal(char *line, char **args);
void redirect1(char *line);
void forking(char **args);
void setExitCode(int num);

bool exitFlag = false;

void commandlineInterpreter() {
    char *line;

    while ((line = linenoise(getenv("PROMPT"))) != NULL) {
        exitFlag = false;

        if(strstr(line, ">")){
            redirect1(line);
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
            FILE *pipe = NULL;
            char buffer[1024];

            //copying file contents to buffer
            sprintf(buffer, "cat %s", args[1]);

            if ((pipe = popen(buffer, "r")) == NULL) {
                perror("popen");
                setExitCode(errno);
            }
            //reading contents from buffer and executing them
            while (fgets(buffer, 1024, pipe) != NULL) {
                //to remove extra newline
                int len = (int) strlen(buffer);
                if (buffer[len - 1] == '\n') {
                    buffer[len - 1] = 0;
                }
                commands(buffer);
            }
            if (pclose(pipe) == -1) {
                perror("pclose");
                setExitCode(errno);
            }
        }
        else {
            printf("--> File does not exist <--\n");
        }
    }
}

void redirect1(char *line){
    char *line1;
    char *line2;
    int saved_stdout;

    FILE *fp = NULL; //file pointer
    int fd;

    if (strstr(line, ">>")){
        line1 = strtok(line, ">>");
        line2 = strtok(NULL, ">>");

        /* Appends to a file. Writing operations, append data at the end of the file.
        * The file is created if it does not exist..*/
        if ((fp = fopen(line2, "ab+")) != NULL) {
            printf("--> File \"%s\" opened. <--\n", line2);
            fflush(stdout);
        }

        if ((fd = open(line2, O_APPEND | O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) == -1){
            perror("dup() failed");
            setExitCode(errno);
        }
    }
    else {
        line1 = strtok(line, ">");
        line2 = strtok(NULL, ">");

        /* Creates an empty file for writing. If file with the same name already exists,
         * its content is erased and the file is considered as a new empty file.*/
        if ((fp = fopen(line2, "wb")) != NULL) {
            printf("--> File \"%s\" opened. <--\n", line2);
            fflush(stdout);
        }

        if ((fd = open(line2, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)) == -1){
            perror("dup() failed");
            setExitCode(errno);
        }
    }

    if (fp == NULL){
        fprintf(stderr, "--> Can't open \"%s\" file! <--\n", line2);
        fflush(stdout);
        return;
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

    fclose(fp); //closes file
    commands(line1);

    /* Restore stdout */
    if ((dup2(saved_stdout, STDOUT_FILENO)) == -1){
        perror("dup2() failed");
        setExitCode(errno);
    }

    close(saved_stdout);

    printf("--> Output redirected to file <--\n");
    fflush(stdout);
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
        if (execvp(args[0], args)) {
            perror("execvp() failed");
            setExitCode(errno);
        }

        //dead code
        printf("This string should never get printed\n");

    } else {
        do {
            pid = waitpid(pid, &status, WNOHANG); //wait until a state change in the child process
            //WNOHSNG --> checks child processes without causing the caller to be suspended

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
//                else if (pid == 0) {
//                    printf("Status information is not available!\n");
//                }
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