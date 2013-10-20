// UCLA CS 111 Lab 1 command execution

#include "alloc.h"
#include "command.h"
#include "command-internals.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
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

        // Set up the redirection
        if (dup2(fd_in, STD_IN) < 0)
            error(1, 0, "Error redirecting stdin");

        // Close the file when done
        if (close(fd_in) < 0)
            error(1, 0, "Error closing file");

    }

    // If the output is not null then set up a redirection for the output
    if (c->output != NULL) {

        // Set the mode for user permissions
        mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

        int fd_out = open(c->output, O_CREAT | O_WRONLY | O_TRUNC, mode);
        
        if (fd_out < 0)
            error(1, 0, "Error reading file: %s", c->output);

        if (dup2(fd_out, STD_OUT) < 0)
            error(1, 0, "Error redirecting stdout");

        if (close(fd_out) < 0)
            error(1, 0, "Error closing file");
    }
}

void execute_simple(command_t c)
{
    int status;

    pid_t pid = fork();

    // If it's the child process then execute the simple command
    if (pid == 0) {
        setup_io(c);
        execvp(c->u.word[0], c->u.word);
        error(1, 0, "Invalid command");
    }
    // If it's the parent process then wait for the child process to execute and
    // then set the commands status to the exit status
    else if (pid > 0) {
        if (waitpid(pid, &status, 0) < 0)
            abort();

        c->status = WEXITSTATUS(status);
    }
    // Otherwise return an error because the fork got an error
    else {
        error(1, 0, "Forked process failed");
    }
}

void execute_and_command(command_t c)
{
    // Execute the first command
    execute_command(c->u.command[0]);

    // If that command succeeds then execute the second command. Then set the
    // exit status to that of the run command
    if (c->u.command[0]->status == 0) {
        execute_command(c->u.command[1]);
        c->status = c->u.command[1]->status;
    }else
    {
        c->status = c->u.command[0]->status;
    }
    
}

void execute_or_command(command_t c)
{
    // Execute the first command
    execute_command(c->u.command[0]);

    // If that command fails then execute the second command
    if (c->u.command[0]->status != 0) {
        execute_command(c->u.command[1]);
        c->status = c->u.command[1]->status;
    }else {
        c->status = c->u.command[0]->status;
    }
}

void execute_pipe_command(command_t c)
{
    // Create an array of two integers to store the pipe line file descriptors
    // fd[0] = Read things in from this file descriptor
    // fd[1] = Write things out from this file descriptor
    int fd[2];

    // Establish the pipe
    if (pipe(fd) != 0)
        error(1, 0, "Failed to create pipe");

    pid_t pid = fork();

    int status;

    // If child process then close the read in portion of the pipe
    // Redirect stdout to the write descriptor of the pipe
    if (pid == 0) {
        close(fd[0]);    

        // Redirect the write section of the pipe to standard out
        if (dup2(fd[1], STD_OUT) < 0)
            error(1, 0, "Failed to redirect write to stdout");

        execute_command(c->u.command[0]);

        // Close the write portion of the pipe
        close(fd[1]);

        // Exit and return command 0's exit status
        _exit(c->u.command[0]->status);

    }else if (pid > 0) {

        pid_t pid2 = fork();

        if (pid2 == 0) {
            // Close the write portion of the pipe
            close(fd[1]);

            // Redirect stdin to the read descriptor of the pipe
            if (dup2(fd[0], STD_IN) < 0)
                error(1, 0, "Failed to redirect stdin");

            execute_command(c->u.command[1]);

            close(fd[0]);

            // Exit and return command 1's exit status
            _exit(c->u.command[1]->status);

        }else if (pid2 > 0) {

            // Close the pipe
            close(fd[0]);
            close(fd[1]);

            if (waitpid(pid2, &status, 0) < 0)
                abort();

            c->status = WEXITSTATUS(status);
        }else
        {
            error(1, 0, "Forked process failed");
        }
        
    }else{
        error(1, 0, "Forked process failed");
    }

}

void execute_subshell_command(command_t c)
{
    execute_command(c->u.subshell_command);
    c->status = c->u.subshell_command->status;
}

void execute_sequence_command(command_t c)
{
    execute_command(c->u.command[0]);
    execute_command(c->u.command[1]);

    // Set the exit status to the status of the last command run TODO: Check to
    // make sure this is correct
    c->status = c->u.command[1]->status;
}

int
command_status (command_t c)
{
    return c->status;
}


// Time Travel Commands

// Structure that contains a list of out files

typedef struct
{
    pid_t pid;
    char* name;
}File;

typedef struct
{
    int m_size;
    int m_pos;
    File* files;
}List;

File create_file(pid_t pid, char* name)
{
    File new_file;
    new_file.name = name;
    new_file.pid = pid;
    return new_file;
}

List create_list()
{
    List new_list;
    new_list.m_size = 5;
    new_list.m_pos = 0;
    new_list.files = checked_malloc(5*sizeof(*new_list.files));
    return new_list;
}

void increase_list_size(List* list)
{
    list->m_size*=2;
    checked_realloc(list->files, (list->m_size*sizeof(File)));
}

void insert_list(List* list, File file)
{
    if (list->m_pos > list->m_size) {
        increase_list_size(list);
    }

    list->files[list->m_pos] = file;
    list->m_pos++;
}

void remove_from_list(List* list, int index)
{
    if (list->m_pos == 0) {
        error(1, 0, "Error list already empty");
    }

    list->m_pos--;
    list->files[index] = list->files[list->m_pos];
}

bool string_equals(char* a, char* b)
{
    while (*a != '\0' && *b != '\0') {
        if (*a != *b) {
            return false;
        }

        a++;
        b++;
    }

    if (*a != *b) {
        return false;
    }else
    {
        return true;
    }
}

int file_in_list(List list, char* file_name)
{
    int i;
    for (i = 0; i < list.m_pos; i++) {
        if (string_equals(list.files[i].name, file_name)) {
            return i;
        }
    }

    return -1;
}

void execute_time_travel(command_stream_t command_stream)
{

    // Initialize list for output file dependencies
    List out_files = create_list();
    command_t command;
    pid_t pid;

    // For each command in the command stream
    while ((command = read_command_stream(command_stream))) {
        // Create a fork
        pid = fork();

        // If it is the child process then check inside of the outfiles
        if (pid == 0) {
            // If it's input file is equal to any of the output files in that list then
            // wait for whatever process is associated to that file before executing
            // itself
            int index = -1;

            if (command->input != NULL) {
                index = file_in_list(out_files, command->input);
            }

            if (index > 0) {
                // When it's done waiting remove that file from the list and exit with it's
                // exit status
                int status;
                pid_t wait_pid = out_files.files[index].pid;
                waitpid(wait_pid, &status, 0);
                remove_from_list(&out_files, index);
            }
            // Otherwise execute the process

            execute_command(command);
            _exit(command->status);
        }else if (pid > 0) {
            // If we are the parent process then if the child has an output file add the
            // file name along with the process id to the list of output files

            pid_t parent = fork();
            if (parent == 0) {
                int index = -1; // Stores the index of the file we just added

                if (command->output != NULL) {
                    File new_file = create_file(pid, command->output); 
                    index = out_files.m_pos;
                    insert_list(&out_files, new_file); 
                }

                // Child process that will wait for the process we stored to
                // finish and then remove it from the outfiles list
                waitpid(pid, NULL, 0);

                if (index > 0) 
                    remove_from_list(&out_files, index);

                _exit(0);
            }else if (parent < 0) {
                error(1, 0, "Failed to fork process");
            }
                 
        }else
        {
            error(1, 0, "Failed to fork process");
        }
    }

}

void
execute_command (command_t c)
{
    switch(c->type)
    {
        case SIMPLE_COMMAND:
            execute_simple(c);
            break;
        case AND_COMMAND:
            execute_and_command(c);
            break;
        case OR_COMMAND:
            execute_or_command(c);
            break;
        case PIPE_COMMAND:
            execute_pipe_command(c);
            break;
        case SUBSHELL_COMMAND:
            execute_subshell_command(c);
            break;
        case SEQUENCE_COMMAND:
            execute_sequence_command(c);
            break;
        default:
            error(1, 0, "Invalid command type");
    }

}
