// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <error.h>

#define STD_IN 0
#define STD_OUT 1

void setup_io(command_t c)
{
    // If the input is not null then set up a redirection for the input
    if (c->input != NULL) {
        int fd_in = open(c->input, O_RDONLY);

        // If the file returned is less than 0 then return an error because the
        // file was not read
        if (fd_in < 0) {
            error(1, 0, "Error reading file: %s", c->input);
        }

        //dup2(fd_in, STD_IN);

    }

    // If the output is not null then set up a redirection for the output
    if (c->output != NULL) {
        //int fd_out = open(c->input, O_WRONLY);

        //dup2(fd_out, STD_OUT);
    }
}

void execute_simple(command_t c, bool time_travel)
{
    int status;

    //setup_io(c);

    pid_t pid = fork();

    // If it's the child process then execute the simple command
    if (pid == 0) {
        execvp(c->u.word[0], c->u.word);
        error(1, 0, "Invalid command");
    }
    // If it's the parent process then wait for the child process to execute and
    // then set the commands status to the exit status
    else if (pid > 0) {
        waitpid(pid, &status, 0);
        c->status = status;
    }
    // Otherwise return an error because the fork got an error
    else {
        error(1, 0, "Forked process failed");
    }
}

void execute_and_command(command_t c, bool time_travel)
{
    // Execute the first command
    execute_command(c->u.command[0], time_travel);

    // If that command succeeds then execute the second command. Then set the
    // exit status to that of the run command
    if (c->u.command[0]->status == 0) {
        execute_command(c->u.command[1], time_travel);
        c->status = c->u.command[1]->status;
    }else
    {
        c->status = c->u.command[0]->status;
    }
    
}

void execute_or_command(command_t c, bool time_travel)
{
    // Execute the first command
    execute_command(c->u.command[0], time_travel);

    // If that command fails then execute the second command
    if (c->u.command[0]->status != 0) {
        execute_command(c->u.command[1], time_travel);
        c->status = c->u.command[1]->status;
    }else {
        c->status = c->u.command[0]->status;
    }
}

void execute_pipe_command(command_t c, bool time_travel)
{
    error(1, 0, "Pipe command not implemented yet");
}

void execute_subshell_command(command_t c, bool time_travel)
{
    execute_command(c->u.subshell_command, time_travel);
    c->status = c->u.subshell_command->status;
}

void execute_sequence_command(command_t c, bool time_travel)
{
    execute_command(c->u.command[0], time_travel);
    execute_command(c->u.command[1], time_travel);

    // Set the exit status to the status of the last command run TODO: Check to
    // make sure this is correct
    c->status = c->u.command[1]->status;
}

int
command_status (command_t c)
{
    return c->status;
}

void
execute_command (command_t c, bool time_travel)
{
    switch(c->type)
    {
        case SIMPLE_COMMAND:
            execute_simple(c, time_travel);
            break;
        case AND_COMMAND:
            execute_and_command(c, time_travel);
            break;
        case OR_COMMAND:
            execute_or_command(c, time_travel);
            break;
        case PIPE_COMMAND:
            execute_pipe_command(c, time_travel);
            break;
        case SUBSHELL_COMMAND:
            execute_subshell_command(c, time_travel);
            break;
        case SEQUENCE_COMMAND:
            execute_sequence_command(c, time_travel);
            break;
        default:
            error(1, 0, "Invalid command type");
    }

}
