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

// Sets up the input and output file redirections for a command
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

//===========================================================================
// Command Execution Functions
//===========================================================================

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
    else 
        error(1, 0, "Forked process failed");
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
        c->status = c->u.command[0]->status;
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
            error(1, 0, "Forked process failed");
        
    }else
        error(1, 0, "Forked process failed");
}

void execute_subshell_command(command_t c)
{
    pid_t pid = fork();
    int status;

    if (pid == 0) {
        setup_io(c);
        execute_command(c->u.subshell_command);
        _exit(c->u.subshell_command->status);
    }else if (pid > 0) 
        waitpid(pid, &status, 0);
    else
        error(1, 0, "Forked process failed");

    c->status = WEXITSTATUS(status);
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

//===========================================================================
// Time Travel Functions
//===========================================================================

// Structure for a list to store the commands
typedef struct Node Node;
typedef struct FileArray FileArray;

// Data structre to store a list of file names
struct FileArray
{
    int pos;
    int size;
    char** files;
};

// Data structe that represents a command node
// Used as a node in a doubly linked list
struct Node
{
    Node* prev_node;
    Node* next_node;

    pid_t pid; // Pid for the command executed
    command_t c;

    FileArray* out_files; // Command's output files
    FileArray* in_files; // Command's input files

    bool executed; // Boolean that determines whether the command has been executed yet
};

// Linked list to store all the nodes we desire
typedef struct
{
    int counter; // Stores the number of items in the list
    Node* begin;
    Node* end;
}List;

//===========================================================================
// File Array Functions
//===========================================================================

FileArray* createFileArray()
{
    FileArray* new_file = checked_malloc(sizeof(FileArray));
    new_file->pos = 0;
    new_file->size = 5;
    new_file->files = checked_malloc(5*sizeof(char*));
    return new_file;
}

void increase_file_array(FileArray* n)
{
    n->size *=2;
    
    n->files = checked_realloc(n->files, n->size*sizeof(*n->files));
}

void insert_file_array(FileArray* array, char* word)
{
    if (array->pos >= array->size) 
        increase_file_array(array);

    array->files[array->pos] = word;
    array->pos++;
}

//===========================================================================
// List Helper Functions
//===========================================================================

List create_list()
{
    List new_list;
    new_list.begin = NULL;
    new_list.end = NULL;
    new_list.counter = 0;
    return new_list;
}

Node* create_node(command_t c)
{
    Node* new_node = checked_malloc(sizeof(Node));
    new_node->prev_node = NULL;
    new_node->next_node = NULL;
    new_node->pid = -1;
    new_node->c = c;
    
    new_node->executed = false;

    // Setup the file arrays
    new_node->out_files = createFileArray();
    new_node->in_files = createFileArray();

    return new_node;

}

void free_node(Node* n)
{
    free(n->out_files->files);
    free(n->out_files);
    free(n->in_files->files);
    free(n->in_files);
    free(n);
    n = NULL;
}

// Removes the node with the given pid from the list
void list_remove(List* list, pid_t pid)
{
    Node* iterator = list->begin;
    while (iterator != NULL) {
        if (iterator->pid == pid) {
            list->counter--;

            // If we are the beginning node then set the next node to the
            // beginning of the list
            if (iterator == list->begin) {
                list->begin = iterator->next_node;
                free_node(iterator);
                iterator = list->begin;
                return;
            }
            // Otherwise if we are the end node the set the previous node to the
            // end of the list
            else if (iterator == list->end) {
                list->end = iterator->prev_node;
                free_node(iterator);
                return; // We can return because we are at the end of the iteration
            }
            // Otherwise set the previous node to the next node and the next
            // node to the previous node
            else
            {
                iterator->prev_node->next_node = iterator->next_node;
                iterator->next_node->prev_node = iterator->prev_node;

                Node *tmp = iterator->next_node;

                free_node(iterator);
                iterator = tmp;
                return;
            }
        }else
        {
            iterator = iterator->next_node;
        }

    }
}

void list_insert(List* list, Node* node)
{
    list->counter++;

    // If the list is empty then set the beginning and end node to the node
    if (list->begin == NULL) {
        list->begin = node;
        list->end = node;
    }else
    {
        // Set the next node of the end node to this node and then set the end of
        // the list to this node
        list->end->next_node = node;

        // Set the previous node of this node the the old end node
        node->prev_node = list->end;

        list->end = node;
    }
}

//===========================================================================
// Dependency Functions
//===========================================================================

void insert_dependencies(command_t command, Node* n)
{
    if (command->input != NULL)
        insert_file_array(n->in_files, command->input);
    if (command->output != NULL)
        insert_file_array(n->out_files, command->output);
    
    if (command->type == SIMPLE_COMMAND) {
        char* word = command->u.word[1];
        int index = 1;

        while (word != NULL) {
            insert_file_array(n->in_files, word);
            index++;
            word = command->u.word[index];
        }
    }
}

void determine_dependencies(command_t command, Node* n)
{
    // Code for determining the input
    switch(command->type)
    {
        case SIMPLE_COMMAND:
            insert_dependencies(command, n);
            break;

        case AND_COMMAND:
        case OR_COMMAND:
        case PIPE_COMMAND:
        case SEQUENCE_COMMAND:
            determine_dependencies(command->u.command[0], n);
            determine_dependencies(command->u.command[1], n);
            break;

        case SUBSHELL_COMMAND:
            insert_dependencies(command, n);
            determine_dependencies(command->u.subshell_command, n);
            break;
    }
}

// Compares two character strings
bool match_word(char* w1, char* w2)
{
    char* iter1 = w1;
    char* iter2 = w2;

    while (*iter1 != '\0' && *iter2 != '\0') {
        if (*iter1 != *iter2) {
            return false;
        }

        iter1++;
        iter2++;
    }

    if (*iter1 != *iter2)
        return false;
    else
        return true;
}

// Returns whether any of the words in in_files is matched in any of the files
// in out_files
bool compare_file_arrays(FileArray* in_files, FileArray* out_files)
{
    int i, j;

    for (i = 0; i < in_files->pos; i++) 
        for (j = 0; j < out_files->pos; j++) 
            if (match_word(in_files->files[i], out_files->files[j]))
                return true;

    return false;
}

bool no_dependencies(Node* n, int index)
{
    Node* iter = n->prev_node;

    // If the node does not have input files then return true
    if (n->in_files == NULL) {
        return true;
    }

    // For each of the previous nodes in the list
    for(; index > 0; index--){
        // Compare all the outfiles of that previous node with the infiles of this
        // current node
        if (compare_file_arrays(n->in_files, iter->out_files)) {
            // If there is a match then return false
            return false;
        }

        iter = iter->prev_node;
    }

    return true;
}

void execute_time_travel(command_stream_t command_stream)
{
    command_t command;

    // Initialize array for all commands
    List command_list = create_list();

    // For each command in the command stream
    while ((command = read_command_stream(command_stream))) {
        // Initialize a new node for the command
        Node* new_node = create_node(command);

        // Determine all of its dependencies and add it to the node
        // For dependencies need to determine input files and output files
        determine_dependencies(command, new_node);

        // Push the new command node on to the array
        list_insert(&command_list, new_node);
    }

    // While there are still commands in the array
    while (command_list.counter > 0) {
        Node* iter = command_list.begin;

        int i;
        // For each command in the array
        for (i = 0; i < command_list.counter; i++)
        {
            // Check to make sure none of it's input files is equal to any of
            // the output files of the previous commands
            // If it doesn't depend on any of the previous commands then we
            // should create a fork and execute the process
            if (no_dependencies(iter, i) && !iter->executed) {
                iter->executed = true;

                pid_t pid = fork();

                // If we are in the child process
                if (pid == 0) {
                    // Execute the command
                    execute_command(iter->c);

                    // Exit the process
                    _exit(0);
                }
                // If we are the parent set that positions pid to the pid of the
                // process just created
                else if (pid > 0) {
                    iter->pid = pid;
                }else
                    error(1, 0, "Forked process Failed");
            }

            iter = iter->next_node;    
        }

        // Wait for any process to finish
        pid_t finished_pid = waitpid(-1, NULL, 0);

        // Find that process in the array and remove it from the array
        list_remove(&command_list, finished_pid);
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
