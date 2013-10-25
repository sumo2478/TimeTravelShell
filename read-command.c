// UCLA CS 111 Lab 1 command reading

#include "alloc.h"
#include "command.h"
#include "command-internals.h"
#include <stdlib.h>
#include <stdio.h>
#include <error.h>

int tmp_buf; // Used to store a popped element that we need later from the stream
command_t last_command; // Used to store a previous command for if there is an operator followed by a new line
int command_line; // Stores the current command number we are on

//===========================================================================
// Command Helper Functions
//===========================================================================

// Data structure for the command stream
struct command_stream
{
    command_t* commands; // Pointer to the first command
    int size; // Size of the command stream in terms of memory
    int counter; // The current number of commands stored in the stream
    int read_counter; // The current counter for the commands that have been read
};

command_stream_t create_command_stream()
{
    command_stream_t new_command_stream = checked_malloc(sizeof(struct command_stream));
    new_command_stream->size = 5;
    new_command_stream->counter = 0;
    new_command_stream->read_counter = 0;
    new_command_stream->commands = checked_malloc(new_command_stream->size*sizeof(command_t));

    return new_command_stream;
}

void increase_command_stream_size(command_stream_t stream)
{
    stream->size *=2;
    command_t* c = stream->commands;

    command_t* tmp_array = checked_realloc(c, (stream->size)*sizeof(*tmp_array));

    stream->commands = tmp_array;
}

// Data structure for storing the read arguments used to read from the file
typedef struct
{
    int (*get_next_byte) (void*);
    void *get_next_byte_argument;
}read_args;

//===========================================================================
// Stack Data structure and Functions
//===========================================================================

typedef struct
{
    int top; // Stores the index for the top of the stack
    int stack_size; // Stores the current size of the stack
    command_t* commands; // Stores the commands current on the 
}Stack;

void increase_stack_size(Stack* stack_ref)
{
    stack_ref->stack_size *= 2;
    checked_realloc(stack_ref->commands, (stack_ref->stack_size)*sizeof(command_t));
}

void push_stack(Stack* stack_ref, command_t command)
{
    stack_ref->top++;

    if (stack_ref->top >= stack_ref->stack_size) {
        increase_stack_size(stack_ref);
    }

    stack_ref->commands[stack_ref->top] = command;
}

bool stack_empty(Stack stack_ref)
{
    return (stack_ref.top < 0);
}

command_t pop_stack(Stack* stack_ref)
{
    if (stack_empty(*stack_ref)) {
        error(1, 0, "%d: Can't pop empty stack", command_line );
    }

    command_t val = stack_ref->commands[stack_ref->top];
    stack_ref->top--;
    return val;
}

command_t top_stack(Stack stack_ref)
{
    if (stack_empty(stack_ref)) {
        error(1, 0, "%d: Can't access top element of empty stack", command_line);
    }
    return stack_ref.commands[stack_ref.top];
}

Stack create_stack()
{
    Stack new_stack;
    new_stack.top = -1;
    new_stack.stack_size = 5;
    new_stack.commands = checked_malloc(new_stack.stack_size*sizeof(command_t));
    return new_stack;
}

//===========================================================================
// Word array helper functions
//===========================================================================

void increase_word_size(char** c, int *word_indexes)
{
    *word_indexes *= 2;
    *c = checked_realloc(*c, *word_indexes*sizeof(*c));
}

char** increase_word_array_size(char** c, int *word_array_indexes)
{
    *word_array_indexes *= 2; 
    c = checked_realloc(c, *word_array_indexes*sizeof(*c));
    return c;
}

//===========================================================================
// Parser Helper Functions
//===========================================================================

// Determines whether the given character is a regular character
bool is_regular_char(int c)
{
    return ((c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           c == '!'               ||
           c == '%'               ||
           c == '+'               ||
           c == ','               ||
           c == '-'               ||
           c == '.'               ||
           c == '/'               ||
           c == ':'               ||
           c == '@'               ||
           c == '^'               ||
           c == '_');
}

// Returns the precedence score for the given operator used in creating the
// command tree
int get_operator_score(command_t operator)
{
    int score;

    switch (operator->type)
    {
        case SUBSHELL_COMMAND:
            score = 0;
            break;
        case SEQUENCE_COMMAND:
            score = 1;
            break;
        case OR_COMMAND:
        case AND_COMMAND:
            score = 2;
            break;
        case PIPE_COMMAND:
            score = 3;
            break;
        default:
            error(1, 0, "%d: Invalid command operator", command_line);
    }

    return score;
}

void remove_white_spaces(int *i, read_args read)
{
    // Skip any intial spaces
    while (*i == ' ' || *i == '\t') {
        *i = read.get_next_byte(read.get_next_byte_argument);
        continue;
    }

}

void remove_white_spaces_and_new_lines(int *i, read_args read)
{
    while (*i == ' ' || *i == '\t' || *i == '\n') {
        *i = read.get_next_byte(read.get_next_byte_argument);
        continue;
    }
}   

int get_next_char(read_args read)
{
    int i = read.get_next_byte(read.get_next_byte_argument);
    return i;
}

// Retrieves the next regular string from the file stream
char* get_word(int *first_char, read_args read)
{
    // If the first character is a space get the next character
    remove_white_spaces(first_char, read);

    // Otherwise if it's not a regular character return NULL
    if (!is_regular_char(*first_char)) {
        return NULL;
    }

    int counter = 0;
    int buf_size = 5;
    
    char* new_word = checked_malloc(buf_size*sizeof(char));

    // While the character is a regular character add it to the new word
    while (is_regular_char(*first_char)) {

        // If the size of the word has been reached increase the size
        if (counter >= buf_size) {
            increase_word_size(&new_word, &buf_size);
        }

        new_word[counter] = *first_char;
        counter++;
        *first_char = get_next_char(read);
    }

    // Add the null terminated byte to the end
    if (counter >= buf_size) {
        increase_word_size(&new_word, &buf_size);
    }

    new_word[counter] = '\0';

    return new_word;
}

// Enters a character into the temporary buffer
// Used for cases where we retrieve the next character but need to use it in
// another function down the line
void enter_tmp_buf(int c)
{
    if (c != ' ' && c != '\t') {
        tmp_buf = c;
    }
}

// Determines whether the command is a pipe command
bool is_pipe_command(read_args read)
{
    int i = get_next_char(read); 

    if (i == '|') {
        return false;
    }else{
        // Place the element that we read back into the stream
        enter_tmp_buf(i);
        return true;
    }
}

// Adds the input and output files for the command
void add_input_output(int *first_char, read_args read, command_t new_command)
{
    if (*first_char == '<') {
        *first_char = get_next_char(read);
        new_command->input = get_word(first_char, read);
        remove_white_spaces(first_char, read);
    }

    if (*first_char == '>') {
        *first_char = get_next_char(read);
        new_command->output = get_word(first_char, read);
    }
}

// Returns true if the command is a and, or, pipe command
bool is_command(command_t last_command)
{
    if (!last_command) {
        return false;
    }

    enum command_type type = last_command->type;

    return (type == AND_COMMAND  ||
           type == OR_COMMAND   ||
           type == PIPE_COMMAND);

}

//===========================================================================
// Command Creation Functions
//===========================================================================

command_t create_command()
{
    command_t new_command = checked_malloc(sizeof(*new_command));
    new_command->status = -1;
    new_command->input = NULL;
    new_command->output = NULL;
    return new_command;
}

command_t create_pipe_or_command(read_args read)
{
    command_t new_command = create_command();

    if (is_pipe_command(read)) {
        new_command->type = PIPE_COMMAND;
    }else{
        new_command->type = OR_COMMAND;
    }
    
    return new_command;
}

command_t create_simple_command(read_args read, int first_char)
{
    command_t new_command = create_command();

    int buffer_size = 5;
    int counter = 0;

    // Allocate memory for the array of strings
    char** words =  checked_malloc(buffer_size*sizeof(*words));
    char* new_word;
    
    // Retrieve a list of words for the simple command
    while ((new_word = get_word(&first_char, read))) {
        if (counter >= buffer_size) {
            words = increase_word_array_size(words, &buffer_size);
        }

        words[counter] = new_word;
        counter++;
    }

    // Initialize the next word to NULL to prevent accessing uninitalized item
    // in print-command.c
    if (counter >=buffer_size) {
        words = increase_word_array_size(words, &buffer_size);
    }

    words[counter] = NULL;

    // Remove white spaces
    remove_white_spaces(&first_char, read);

    // Determine if there is an input/output redirect after the simple command
    add_input_output(&first_char, read, new_command);

    // Restore the last element to use later if necessary
    enter_tmp_buf(first_char);

    new_command->u.word = words;
    new_command->type = SIMPLE_COMMAND;

    return new_command;
}

command_t create_and_command(int i, read_args read)
{
    command_t new_command = create_command();

    // Determine if it is an && statement or return error
    i = get_next_char(read);

    if (i == '&')
        new_command->type = AND_COMMAND; 
    else
        error(1, 0, "%d: Invalid & command", command_line); 
    
    return new_command;
}

command_t create_subshell_command(int i, read_args read)
{
    command_t new_command = create_command();
    new_command->type = SUBSHELL_COMMAND;

    char** word = checked_malloc(sizeof(*word));
    char* w = checked_malloc(sizeof(*w));
    w[0] = i;
    word[0] = w;

    new_command->u.word = word;

    // Determine if there is an input/output redirect after the subshell command
    if (i == ')') {
        int c;
        c = get_next_char(read);
        remove_white_spaces(&c, read);
        add_input_output(&c, read, new_command);
        
        enter_tmp_buf(c);
    }

    return new_command;
}

command_t create_sequence_command()
{
    command_t new_command = create_command();
    new_command->type = SEQUENCE_COMMAND;
    return new_command;
}

// command_before is a boolean that states whether there was a command before
// this function call
command_t get_next_command(read_args read, bool command_before)
{
    int i;

    // If there is a character in the temporary buffer use it otherwise get the
    // next character
    if (tmp_buf >= 0) {
        i = tmp_buf;
        tmp_buf = -1;
    }else{
        i = get_next_char(read);
    }

    for(;;) {

        // Skip any intial spaces
        remove_white_spaces(&i, read);

        if (i == ';') {
            return create_sequence_command();
        }

        if (i == '#')
        {
            // Ignore until we reach end of comment
            while (i != '\n') {
                i = get_next_char(read);
            }

            // If there is no command before this comment then remove all new
            // lines and white spaces after the comment
            if (!command_before) {
                remove_white_spaces_and_new_lines(&i, read);
            }

            continue;
        }

        if (i == '\n') {
            if (is_command(last_command)) {
                i = get_next_char(read);
                continue;

            }else{
                return NULL;
            }
        }

        // If i is less than zero then return because we have reached the end of
        // the stream
        if (i <=0) {
            return NULL;
        }

        if (is_regular_char(i)) {
            return create_simple_command(read, i);
        }
        else if (i == '|')
        {
            // Determine if it is a pipe or an or statement
            return create_pipe_or_command(read);
        }
        else if (i == '&')
        {
            return create_and_command(i, read);
        }
        else if (i == '(' || i == ')')
        {
            return create_subshell_command(i, read);
        }else
        {
            error(1, 0, "%d: Unrecognized symbol received - \'%c\'", command_line, i);
            return create_command();
        }
    }
}

command_t get_next_stream(read_args read, bool* valid_stream)
{
    bool in_subshell = false;
    bool command_before = false;

    command_t curr_command;

    Stack operand_stack = create_stack();
    Stack operator_stack = create_stack();

    // While we are getting new commands for the current stream
    while ((curr_command = get_next_command(read, command_before))) {
        command_before = true;
        last_command = curr_command;

        if ((curr_command->type == SIMPLE_COMMAND)) 
        {
            push_stack(&operand_stack, curr_command);
        }
        else
        {
            // If it's a sequence command and we are not in a subshell then
            // break from this loop
            if (curr_command->type == SEQUENCE_COMMAND && !in_subshell)
                break;
            
            // If the operator stack is empty push it onto the operator stack
            if (curr_command->type == SUBSHELL_COMMAND ) 
            {
                // Otherwise if the operator is a left bracket then push it onto the
                // operator stack
                char **w = curr_command->u.word;

                if (**w == '(') {
                    in_subshell = true;
                    push_stack(&operator_stack, curr_command);
                }else{
                // Otherwise if the operator is a right bracket then while we have
                // not reached the left bracket pop the top of the operator stack
                // and apply it to the two top elements of the operand stack
                    in_subshell = false;
                    command_t operator_command = pop_stack(&operator_stack);

                    while (operator_command->type != SUBSHELL_COMMAND) {
                        command_t right_command = pop_stack(&operand_stack);
                        command_t left_command = pop_stack(&operand_stack);

                        operator_command->u.command[0] = left_command;
                        operator_command->u.command[1] = right_command;

                        push_stack(&operand_stack, operator_command);

                        if (stack_empty(operator_stack)) {
                            error(1, 0, "%d: Invalid command - missing open bracket", command_line);
                        }else{
                            operator_command = pop_stack(&operator_stack);
                        }
                    }
                // Then attach the top of the operand to the SUBSHELL COMMAND
                // and place it back on to the operand stack
                    command_t operand_command = pop_stack(&operand_stack);
                    curr_command->u.subshell_command = operand_command;
                    push_stack(&operand_stack, curr_command);
                }
            }
            else if (stack_empty(operator_stack)) 
            {
                push_stack(&operator_stack, curr_command);
            }
            // Otherwise if the score of the operator is greater than the top of
            // the operator stack push it onto the operator stack
            else if(get_operator_score(curr_command) > get_operator_score(top_stack(operator_stack)))
            {
                push_stack(&operator_stack, curr_command);
            }
            // Otherwise Pop the top two elements from the operand stack and
            // apply to the top operator of the operator stack and push it onto
            // the operand stack
            // Push the new operator on to the operator stack
            else
            {
                command_t right_command = pop_stack(&operand_stack);
                command_t left_command = pop_stack(&operand_stack);
                command_t operator_command = pop_stack(&operator_stack);

                operator_command->u.command[0] = left_command;
                operator_command->u.command[1] = right_command;

                push_stack(&operand_stack, operator_command);
                push_stack(&operator_stack, curr_command);
            }
        }
    }
    // Return the top of the operand stack and if it has more than one
    // element or no element than there is an error same with if there is an
    // object inside of the operator stack
    
    while (!stack_empty(operator_stack)) {
        command_t right_command = pop_stack(&operand_stack);
        command_t left_command = pop_stack(&operand_stack);
        command_t operator_command = pop_stack(&operator_stack);

        operator_command->u.command[0] = left_command;
        operator_command->u.command[1] = right_command;

        push_stack(&operand_stack, operator_command);

    }

    if (stack_empty(operand_stack)) {
        error(1, 0, "%d: Invalid command - too many operators", command_line);
    }
    command_t new_stream = pop_stack(&operand_stack);

    if (!stack_empty(operand_stack))
    {
        error(1, 0, "%d: Invalid command - too many operators", command_line);
    }

    // Determine whether we have reached the end of not
    int i = get_next_char(read);

    while (i == ' ' || i == '\t' || i == '\n') {
        i = get_next_char(read);
    }

    if (i <= 0) {
        *valid_stream = false;
    }else{
        tmp_buf = i;
    }

    return new_stream;
}

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
    // Set up read structure
    read_args read; 
    read.get_next_byte = get_next_byte;
    read.get_next_byte_argument = get_next_byte_argument;

    // Allocate memory for the command stream
    command_stream_t new_command_stream = create_command_stream();

    command_t curr_stream;
    tmp_buf = -1;
    command_line = 1;
    bool valid_stream = true; // Determines whether we have reached the end of the stream

    // Retrieve the commands and store it into the stream
    while (valid_stream) {
        curr_stream = get_next_stream(read, &valid_stream);

        // If the size has been reached increase the size of the stream
        if (new_command_stream->counter >= new_command_stream->size) {
            increase_command_stream_size(new_command_stream);
        }

        // Add the current stream to the list of command streams
        new_command_stream->commands[new_command_stream->counter] = curr_stream;
        new_command_stream->counter++;
        command_line++;
    }

    return new_command_stream;
}

command_t
read_command_stream (command_stream_t s)
{
    if (s->read_counter < s->counter) {
        command_t val = s->commands[s->read_counter];
        s->read_counter++;
        return val;
    }else{
        return NULL;
    }
}
