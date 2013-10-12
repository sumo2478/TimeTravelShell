// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"
#include <unistd.h>
#include <sys/types.h>
#include <error.h>

void execute_simple(command_t c, bool time_travel)
{
    int status;
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

    // If that command succeeds then execute the second command
    if (c->u.command[0]->status == 0) {
        execute_command(c->u.command[1], time_travel);
    }

}

void execute_or_command(command_t c, bool time_travel)
{
    error(1, 0, "Or command not implemented yet");
}

void execute_pipe_command(command_t c, bool time_travel)
{
    error(1, 0, "Pipe command not implemented yet");
}

void execute_subshell_command(command_t c, bool time_travel)
{
    error(1, 0, "Subshell command not implemented yet");
}

void execute_sequence_command(command_t c, bool time_travel)
{
    error(1, 0, "Sequence command not implemented yet");
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
