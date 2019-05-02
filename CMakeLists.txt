cmake_minimum_required(VERSION 3.13)
project(eggshell C)

set(CMAKE_C_STANDARD 99)

add_executable(eggshell eggshell.c  linenoise/linenoise.c ShellVariables.c CommandlineInterpreter.c)