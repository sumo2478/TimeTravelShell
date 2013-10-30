TimeTravelShell
===============

Time Travel Shell

Project for CS 111 - Operating system, which aims to create a shell that is
capable of running multiple non-dependent commands in parallel. We determine
dependency based on the input files and output files to a command by comparing
their file names. In addition, we consider all parameters to a command as input
files even if they are not, so echo hello will consider this command as having
"hello" as an input file name.

Ex:
    1. sleep 2 && echo hello > a
    2. cat < a

    In this example we can see that the second command is dependent on the first
    command because the second command takes as input a file that the first
    command outputs and therefore cannot be run in parallel.

Ex:
    1. sleep 2 && echo world
    2. echo hello

    In this example there are no dependencies between the two and therefore when
    run in parallel these commands should output hello world

Commands:
    ./timetrash - will allow you to run shell commands normally
    ./timetrash [-p] - will allow you to print out a tree of the commands to be
                       run
    ./timetrash [-t] - wil allow you to run shell comands in parallel

